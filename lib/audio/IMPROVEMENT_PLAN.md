# Improvement plan: `lib/audio`

Date: 2026-11-10.
Scope: `lib/audio/` (C++ kernels in `src/audio.cpp`, Neve wrappers in `wav.nv`/`dsp.nv`) and `audio_tests/`.
Open problems are listed in `KNOWN_ISSUES.md`; full benchmark tables are in `audio_tests/benchmarks/README.md`.

## 1. Current state

Benchmark of 2026-11-10 (Docker, clang-21, one core, median of 3 runs, mean milliseconds per call, 10 s mono 16 kHz unless stated). librosa 0.11 and torchaudio 2.5.1 (CPU) run on the same core with one thread. Bold = fastest. The last column is the fastest Python time ÷ Neve's time; above 1, Neve is faster.

| operation | Neve | librosa | torchaudio | Neve vs fastest Python |
|---|---:|---:|---:|---:|
| load PCM16, 1 s | **0.053** | 0.141 | 0.149 | 2.64× |
| load PCM16 | **0.316** | 0.511 | 0.512 | 1.62× |
| load PCM16, 60 s | **1.836** | 2.881 | 2.534 | 1.38× |
| load f32 | 0.238 | 0.184 | **0.155** | 0.65× |
| load f32, stereo 22 kHz | 0.916 | **0.236** | 0.255 | 0.26× |
| save PCM16 | **1.616** | 12.744 | 11.736 | 7.26×\* |
| spectrogram | **2.307** | 4.231 | 3.494 | 1.51× |
| spectrogram, 60 s | **13.220** | 24.289 | 26.121 | 1.84× |
| mel | **1.736** | 6.087 | 4.165 | 2.40× |
| mel, 60 s | **10.524** | 30.907 | 24.817 | 2.36× |
| MFCC | **1.992** | 7.517 | 4.379 | 2.20× |
| resample sinc 16→8 kHz | 0.649 | **0.569** | 1.339 | 0.88× |
| resample sinc 16→24 kHz | 1.748 | 1.627 | **1.267** | 0.72× |
| μ-law encode | **0.458** | 4.425 | 0.607 | 1.32× |
| time stretch 1.5× | **3.331** | 32.483 | 17.955 | 5.39× |

\* Save timings include the disk write and were noisy in this run (Python per-run means 5.8–13.8 ms; an earlier run measured librosa 5.5 ms and torchaudio 6.2 ms), so read this ratio as about 4–7×.

- **Neve leads** on PCM16 load/save, spectrogram, mel, MFCC, μ-law and time stretch.
- **Neve is behind** on f32 loads (tensor allocation, §5.1) and on sinc resample (filter length, §5.3).
- **Memory:** the Neve process peaks at ~3.2 GB of RSS (librosa/torchaudio: 250–370 MB) because the runtime never frees `tensor.data` (§5.1).
- **Accuracy:** spectrogram 133–134 dB, mel 116–122 dB, MFCC 119 dB SNR against librosa/torchaudio; resample 44.7–45.9 dB (16→8 kHz) and 40.6–41.5 dB (16→24 kHz); time stretch 52.0 dB SNR and 0.0015 spectral convergence against torchaudio; loads bit-exact against `soundfile`.

Kernel-only times (C++ harness, no tensor or FFI cost, clang-21, one core): spectrogram 1.67 ms, mel 1.55 ms, MFCC 1.93 ms, resample 0.47 ms (16→8 kHz) and 1.56 ms (16→24 kHz). On DSP the kernel is almost all of the end-to-end time; on loads, tensor allocation is a large share.

## 2. How the current code works

### 2.1 Build

`Makefile`: clang++-21, `-O3 -mavx -fno-math-errno -fno-trapping-math -fopenmp-simd`.

- `-fno-math-errno` and `-fno-trapping-math` let `sqrtf` and branch-free loops vectorize without changing results for finite inputs.
- `-ffast-math` is deliberately not used: `save_wav` relies on `std::isfinite`, and global reordering of sums would change results.
- Hot reductions use GCC/Clang vector extensions instead of plain loops, because g++ and clang vectorize plain reductions very differently.

### 2.2 WAV I/O

- **`nsk_audio_wav_info_c`:** walks the RIFF chunks; supports PCM 8/16/24/32-bit, float32 and `WAVE_FORMAT_EXTENSIBLE` with a PCM/float SubFormat. It rejects truncated or irregular `data` chunks and files whose decoded size would overflow the 32-bit Neve tensor size.
- **`nsk_audio_read_decode`:** decodes straight into the tensor in 64 KB chunks through a `thread_local` buffer, with no per-sample `%`/`/`. Mono f32 is `fread` directly into the tensor; there are dedicated paths for stereo f32 and mono PCM16. On failure the tensor is left zeroed.
- **`nsk_audio_save_wav`:** interleaves and encodes 64 KB at a time (PCM16 with clamping and NaN → 0, or f32).
- Paths of 4096 bytes or more are rejected (`c_path`), not truncated.

### 2.3 FFT and STFT

- **`CPlan`:** radix-2 complex FFT with a bit-reversal table and per-stage twiddles computed in double and stored as float. Stages 1 and 2 are fused into radix-4; the remaining stages have a contiguous, vectorizable inner loop.
- **`RFFT`:** real FFT of size N as a complex FFT of size N/2 plus a split step, and the inverse with `numpy.irfft` normalization.
- **`STFTPlan`:** an `RFFT` plus the symmetric Hann window centered in `n_fft`.
- **`PlanCache`:** plans are immutable once built and shared through `shared_ptr`; a mutex guards only creation; the cache is cleared at 32 entries. Safe for concurrent calls (checked with TSAN).
- **`power_block`:** frames are processed in blocks of 16 (framing with reflect padding, windowing and packing in one pass) and the power is written as a bin-major tile, so no full `re`/`im` matrix is ever built.
- **Sign convention:** forward transform e^{-i} (numpy/torch).
- **Limitation:** `n_fft` must be a power of 2.

### 2.4 Mel and MFCC

- **`nsk_audio_melspectrogram`:** HTK mel filter bank stored sparsely (`MelPlan`, cached per `(sr, bins, n_mels, fmin, fmax)`) and applied block by block to the power tiles; the full power spectrogram is never stored. Non-finite `fmin`/`fmax` are rejected.
- **`nsk_audio_mfcc`:** power dB via `fast_ln` (vectorizable, ~1 ulp), 80 dB floor, then an orthonormal DCT-II as a precomputed matrix.

### 2.5 Resample

- **`nsk_audio_resample_sinc`:** polyphase bandlimited sinc, 64 taps, Hann window, cutoff 0.99 × min(1, new/orig).
  - The bank of `new_sr / gcd` rows is cached per rate pair (up to 4096 phases); unusual rate pairs build each row once per output sample and use it for all channels.
  - The output position advances incrementally (`center`, `phase`), with no division per sample.
  - `dot64` uses two 8-lane vector accumulators.
  - Equal rates are an identity copy.
- **`nsk_audio_resample_linear`:** linear interpolation.

### 2.6 μ-law

- **Encode:** branch-free loop with `fast_ln`; codes are sign bit + 7-bit magnitude (0..255).
- **Decode:** 128-entry magnitude table; NaN or huge inputs map to code 0.

### 2.7 Time stretch

`nsk_audio_time_stretch` is the librosa/torchaudio phase vocoder: n_fft 1024, hop 256, symmetric Hann, reflect padding.

- Analysis frame at position `t·rate` with linear magnitude interpolation.
- Per-bin phase accumulator kept in [−π, π] with `wrap_pi` (O(1)).
- Vectorizable `fast_atan2` and `fast_sincos` (cephes polynomials, ~1e−7).
- Analysis is streamed: only the two frames in use are kept, as magnitude and phase.
- Synthesis with the inverse `RFFT`, overlap-add normalized by the sum of squared windows; output length `round(T / rate)`.
- Rates are checked in double before any int cast; `rate <= 0` or a too-small rate is rejected.

### 2.8 Other kernels and the Neve wrappers

- **Fade:** same curves as `torchaudio.transforms.Fade` (linear); overlapping fades multiply.
- **Vol:** amplitude, dB or power gain, like `torchaudio.transforms.Vol`.
- **Time/frequency masks:** zero a column or row range, clamped to the spectrogram.
- **Utilities:** `nsk_audio_tic`/`toc` (milliseconds since the last tic), `stft_bins`/`stft_frames`/`resample_len` (shape helpers returning `int`), `nsk_audio_zero`, `nsk_audio_free`.
- **Error policy of the wrappers (`dsp.nv`):** on invalid arguments they print an error and return a zero-filled tensor (or an empty one when the shape itself is invalid). `audio_fade`, `audio_vol` and the masks work in place.

## 3. Decisions in force

| id | decision | consequence |
|---|---|---|
| D1 | Time stretch is the librosa/torchaudio phase vocoder | waveforms comparable with both references (52 dB vs torchaudio) |
| D2 | FFT uses the standard e^{-i} sign | `audio_stft`/`audio_fft` match numpy/torch |
| D3 | Keep `-mavx` (no AVX2/FMA requirement) | runs on pre-Haswell CPUs; leaves some speed on the table (§5.2) |
| D4 | Keep Neve's own μ-law codes (sign + magnitude) | benchmark compares decoded audio, not codes |
| D5 | **Open:** who fixes tensor release in the runtime, and how | see §5.1 |
| D6 | In-house FFT, no new dependency | power-of-2 sizes only (§5.5) |

## 4. Verification

| check | what it covers | how to run |
|---|---|---|
| `audio_tests/benchmarks/check_kernels.py` | edge cases against float64/numpy/soundfile/librosa references: WAV variants (incl. WAVE_FORMAT_EXTENSIBLE), rejections, long paths, resample identity and multi-channel, mel NaN/inf, fade/vol, stretch rates, μ-law NaN | `NEVE_AUDIO_TEST_SO=<lib.so> python3 check_kernels.py` |
| `audio_tests/benchmarks/validate_kernels.py` | accuracy vs librosa, alias rejection, stretch vs a float64 phase vocoder (rates 0.5–6), FFT vs numpy, bit-exact loads, kernel timings | see the benchmark README |
| `audio_tests/stress_kernels.cpp` | ASAN + UBSAN (incl. float-cast-overflow) | build line in the file header |
| `audio_tests/stress_threads.cpp` | TSAN on the plan caches | build line in the file header |
| `audio_tests/test_wav.nv`, `test_dsp.nv`, `test_dsp3.nv` | Neve wrappers end to end, including invalid arguments | `neve <file>` |
| `audio_tests/benchmarks/run.sh` | timing, peak RSS and accuracy against librosa/torchaudio | `docker run --rm neve-audio-bench` |

Acceptance thresholds in `merge.py`: spectrogram/mel/MFCC SNR ≥ 110 dB; resample ≥ 40 dB (16→8 kHz) and ≥ 38 dB (16→24 kHz); time stretch SNR ≥ 40 dB vs torchaudio and ≥ 25 dB vs librosa, spectral convergence ≤ 0.01, exact length; loads bit-exact.

## 5. Next steps

Ordered by impact. Each step lists its verification.

### 5.1 Free tensor memory in the runtime (blocking; owner: Neve core, decision D5)

`class tensor` (`lib/std_lib/tensor.nv`) allocates `data` with `c_malloc64`, and the GC sweep skips objects of Neve classes, so `data` is never freed. The registered `tensor_Clean_Up` is a stub that is never called for the std_lib class. Possible paths:

- a finalizer for Neve classes, called during the sweep;
- allocating `data` through the GC pool;
- turning `tensor` into a runtime type whose `tensor_Clean_Up` frees `data`.

Verify:

- peak RSS of `bench_neve.nv` below 200 MB (today ~3.2 GB);
- f32 loads close to their kernel-only time (~0.04 ms mono, ~0.18 ms stereo 22 kHz), which would put them ahead of librosa and torchaudio.

Optional mitigation inside the lib, independent of the runtime: variants that write into an existing tensor (`audio_load_into`, `audio_spectrogram_into`) for pipelines that reuse buffers. Verify with the same RSS measurement on a loop that reuses one tensor.

### 5.2 AVX2/FMA

`-mavx2 -mfma`, or `target_clones` for runtime dispatch so D3 stays valid. Expected +15 to 45% on resample and μ-law, little on the FFT paths.
Verify: `check_kernels.py` and `validate_kernels.py` pass; timings with both builds.

### 5.3 Configurable resample filter width

Keep 64 taps as the default and add a width parameter (torchaudio's default uses 16–28 taps). It is the only way to match torchaudio at 16→24 kHz, trading quality for time.
Verify: alias rejection and SNR vs `soxr_hq` reported per width; default output unchanged.

### 5.4 torchaudio μ-law convention (changes D4)

Encode to torchaudio's 0..255 codes so the benchmark can compare codes exactly. It changes the API output.
Verify: codes equal to `torchaudio.functional.mu_law_encoding` on the benchmark signal.

### 5.5 Non-power-of-2 `n_fft`

For example 400 (Whisper). Needs Bluestein or a mixed-radix FFT (or vendoring `pocketfft`, which would change D6).
Verify: FFT against numpy for sizes 2–4096, including primes; mel/MFCC against librosa with `n_fft=400`.

### 5.6 Multi-threaded STFT

Split STFT blocks across threads (the plan cache is already thread-safe), giving Neve a defT column. Not needed to win the single-thread column.
Verify: output bit-identical to the single-thread path; TSAN clean.

### 5.7 Error reporting to Neve code

Wrappers currently print and return zeros, so callers cannot test for failure. Expose the kernel return code (for example an `ok` field or a status function).
Verify: `test_invalid_args` in `test_dsp3.nv` checks the status instead of the message.

### 5.8 Test and benchmark housekeeping

- `test_wav.nv` `sox_ground_truth` needs `/neve/sox_ref.wav`, which the Docker image does not have: generate it in the image or skip the test when it is missing.
- Rerun the 3-run benchmark and update the README table after each step above.

## 6. Risks

| risk | mitigation |
|---|---|
| Compiler-dependent vectorization (g++ and clang differ a lot on reductions) | vector extensions in hot loops; always confirm timings with clang-21, the `Makefile` compiler |
| Accuracy of `fast_ln`, `fast_atan2`, `fast_sincos` | dense sweeps in the tests; documented error ~1 ulp / ~1e−7 |
| Plan caches shared between threads | immutable plans, mutex only on creation, `shared_ptr` lifetime; TSAN stress test |
| Benchmark noise (Docker, disk writes for save) | 3 runs, core pinning, min–max ranges in `summary.csv` |
| Timings are from one host (i7-7700) | treat ratios, not absolute times, as the target |

# Known issues: `lib/audio`

Date: 2026-11-10.
Reference state: the current `src/audio.cpp` (described in `IMPROVEMENT_PLAN.md` §2) after the code review (17 findings, 15 fixed). Next steps for each item are in `IMPROVEMENT_PLAN.md` §5.
This file lists what is still wrong, incomplete or limited, grouped by where the fix has to happen.

## 1. Outside the lib (Neve runtime and compiler)

### 1.1 `tensor.data` is never freed (blocking)

- The runtime does not free tensor memory. The `bench_neve.nv` process reaches **~3.2 GB of RSS**; librosa and torchaudio stay at 250–370 MB.
- It also makes f32 loads slower: every tensor allocation lands on fresh pages and pays page faults.
- This is step 5.1 of the plan (decision D5): it needs a finalizer for Neve classes, allocating `data` through the GC pool, or turning `tensor` into a runtime type with `tensor_Clean_Up`.
- Done criterion: peak RSS of `bench_neve.nv` below 200 MB.

### 1.2 The `neve` compiler crashes on a `printl` with many arguments

- A `printl` with about 12 or more arguments makes the compiler exit with a segfault (code 139) and no message at all, even if the function is never called.
- Workaround used in `audio_tests/test_dsp3.nv`: split the message into several `printl` calls.
- Should be reported to the Neve project.

### 1.3 `c_free` missing from the `neve` binary in the Docker image

- It exists in the source code, but the binary in the `neve-audio-bench` image fails with `JIT session error: Symbols not found: [ c_free ]`.
- Workaround: the lib exports `nsk_audio_free` and the wrappers use it.

### 1.4 `include.h` is not read by the compiler

- The libs parser (`src/compiler_frontend/libs_parser.cpp`) reads only `.cpp` and `.cu` files. The declarations come from `include.nv`; `include.h` is documentation only.

## 2. Performance: where Neve still loses

Measurement of 2026-11-10 (Docker, 1 core, median of 3 runs, 10 s mono 16 kHz):

| operation | Neve | fastest in Python | Neve vs fastest | cause |
|---|---:|---:|---:|---|
| load f32 | 0.238 ms | 0.155 (torchaudio) | 0.65× | tensor allocation (§1.1) |
| load f32, stereo 22 kHz | 0.916 ms | 0.236 (librosa) | 0.26× | tensor allocation (§1.1) |
| sinc resample 16→24 kHz | 1.748 ms | 1.267 (torchaudio) | 0.72× | 64 fixed taps; torchaudio uses 16–28 |
| sinc resample 16→8 kHz | 0.649 ms | 0.569 (librosa) | 0.88× | `soxr_hq` is a different, faster algorithm in this case |

- Small margins (resample 16→8 kHz) can flip between runs; see the min–max ranges in `summary.csv`.
- Save timings include the disk write and are noisy: in the 2026-11-10 run the Python per-run means ranged 5.8–13.8 ms, so the save ratio in the README (7.26×) is closer to 4–7×.
- Performance depends on the compiler: some kernels are up to 2.6× slower with g++ 13 than with clang-21. The numbers hold for the `Makefile`'s clang-21.
- Timings come from a single host (i7-7700) and do not transfer to other machines.

## 3. Functional limitations of the lib

- **`n_fft` must be a power of 2.** `n_fft = 400` (Whisper) is rejected: the function prints an error and returns zeros. The current FFT is radix-2; other sizes need Bluestein or mixed-radix.
- **Resample with a fixed 64-tap filter.** Quality cannot be traded for time. A configurable number of taps is the only way to reach torchaudio at 16→24 kHz.
- **μ-law uses its own code convention (D4).** It differs from torchaudio's, so the benchmark compares the decoded signal (difference ≤ 0.038), not the codes.
- **Single-threaded kernels.** There is no "defT" column for Neve.
- **No AVX2/FMA.** The build uses `-mavx` (D3); `-mavx2 -mfma` or `target_clones` would give +15 to 45% on resample and μ-law.
- **Errors cannot be detected from Neve code.** With an invalid argument, the `dsp.nv` wrappers print a message and return a zero-filled tensor (or an empty one, if the shape is invalid). There is no return code or exception for the caller to test.
- **No variants that write into an existing tensor** (`audio_load_into`, `audio_spectrogram_into`). They would be the in-lib mitigation of §1.1 for pipelines that reuse buffers.

## 4. Design-decision behaviors that may surprise

- **Truncated or irregularly sized `data` chunks are rejected.** `soundfile` reads whatever is there. `validate_kernels.py` requires the rejection; accepting these files changes that contract.
- **`audio_fade`, `audio_vol`, `audio_time_mask` and `audio_freq_mask` modify their input** and return it. This is documented in the `dsp.nv` header. Returning a new tensor would double memory while §1.1 is unresolved.
- **API changes compared to the old code:**
  - `audio_stft`/`audio_fft` use the numpy/torch FFT sign (e^{-i}); the imaginary part changed sign (D2);
  - `audio_time_stretch` uses the librosa/torchaudio phase vocoder; the output changed and `rate > 4` no longer truncates (D1);
  - `audio_vol` accepts only `"amplitude"`, `"db"` and `"power"`; `audio_resample` accepts only `"sinc"` and `"linear"`;
  - `audio_time_stretch` with `rate <= 0` returns an empty `[1, 0]` tensor.

## 5. Tests, benchmark and documentation

- **`test_wav.nv` `sox_ground_truth` fails in the Docker image:** the file `/neve/sox_ref.wav` does not exist there. The test needs to generate the file or be skipped when it is missing.
- **Benchmark results are not stored in the repository:** `results/` and `results_current/` are ignored by git, so the README numbers cannot be checked from the repository.
- **Limited local validation:** on this host torchaudio is broken (undefined symbol) and there is no clang; the torchaudio comparison and the reference timings only run inside the Docker image.

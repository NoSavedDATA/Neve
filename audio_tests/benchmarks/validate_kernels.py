#!/usr/bin/env python3
"""Validate the actual C++ audio kernels without the Neve compiler.

Build first (the flags of lib/audio/Makefile; clang++-21 is the production compiler):
    g++ -DNEVE_AUDIO_STANDALONE -O3 -mavx -fopenmp-simd -fno-math-errno -fno-trapping-math \
        -std=c++17 -fPIC -shared lib/audio/src/audio.cpp -o /tmp/libneve_audio_test.so
See check_kernels.py for edge-case checks against float64/numpy/soundfile references.
Run from repository root: python3 audio_tests/benchmarks/validate_kernels.py
"""
import ctypes as ct
import os
import time

import numpy as np
import soundfile as sf


class DTStr(ct.Structure):
    _fields_ = [("str", ct.c_char_p), ("size", ct.c_int)]


def cstr(value):
    data = os.fsencode(value)
    return DTStr(data, len(data))


lib = ct.CDLL(os.environ.get("NEVE_AUDIO_TEST_SO", "/tmp/libneve_audio_test.so"))
fp = ct.POINTER(ct.c_float)
ip = ct.POINTER(ct.c_int)


def bind(name, types):
    fn = getattr(lib, "nsk_audio_" + name)
    fn.argtypes = [ct.c_void_p, *types]
    fn.restype = ct.c_int
    return fn


spec_fn = bind("spectrogram", [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int,
                                ct.c_float, fp, ct.c_int, ct.c_int])
mel_fn = bind("melspectrogram", [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int,
                                 ct.c_int, ct.c_float, ct.c_float, fp,
                                 ct.c_int, ct.c_int])
mfcc_fn = bind("mfcc", [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int,
                         ct.c_int, ct.c_int, fp, ct.c_int])
resample_fn = bind("resample_sinc", [fp, ct.c_int, ct.c_int, ct.c_int,
                                     ct.c_int, fp, ct.c_int])
info_fn = bind("wav_info_c", [DTStr, ip])
read_fn = bind("read_decode", [DTStr, fp, ct.c_int, ct.c_int, ct.c_long,
                                ct.c_int, ct.c_int])
save_fn = bind("save_wav", [DTStr, fp, ct.c_int, ct.c_int, ct.c_int, DTStr])
stretch_fn = bind("time_stretch", [fp, ct.c_int, ct.c_float, fp, ct.c_int])
mask_fn = bind("time_mask", [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int])
fft_fn = bind("fft", [fp, fp, ct.c_int, ct.c_int])
mulaw_fn = bind("mulaw_encode", [fp, fp, ct.c_int])
mulaw_dec_fn = bind("mulaw_decode", [fp, fp, ct.c_int])


def ptr(a):
    return a.ctypes.data_as(fp)


def snr(a, b):
    a = np.asarray(a, dtype=np.float64)
    b = np.asarray(b, dtype=np.float64)
    return 10 * np.log10(np.sum(b*b) / np.sum((a-b)**2))


def measured(fn, count=5):
    fn()
    samples = []
    for _ in range(count):
        start = time.perf_counter()
        fn()
        samples.append((time.perf_counter() - start) * 1000)
    return np.median(samples)


def main():
    import librosa
    from scipy.signal.windows import hann

    path = "audio_tests/benchmarks/data/sig_10s_16k_s16.wav"
    y, sr = sf.read(path, dtype="float32")
    y = np.ascontiguousarray(y)
    assert sr == 16000 and y.ndim == 1
    n_fft, hop, n_mels, n_mfcc = 1024, 256, 128, 20
    frames, bins = 1 + len(y)//hop, n_fft//2 + 1
    window = hann(n_fft, sym=True)
    def spec_ref_fn():
        return np.abs(librosa.stft(y, n_fft=n_fft, hop_length=hop,
                                   window=window, pad_mode="reflect")) ** 2
    def mel_ref_fn():
        return librosa.feature.melspectrogram(y=y, sr=sr, n_fft=n_fft,
                                               hop_length=hop, n_mels=n_mels,
                                               htk=True, norm=None, window=window,
                                               pad_mode="reflect")
    def mfcc_ref_fn():
        return librosa.feature.mfcc(S=librosa.power_to_db(mel_ref_fn(), ref=1.0,
                                                           top_db=80), n_mfcc=n_mfcc,
                                     dct_type=2, norm="ortho")

    spec = np.empty((bins, frames), np.float32)
    mel = np.empty((n_mels, frames), np.float32)
    mfcc = np.empty((n_mfcc, frames), np.float32)
    def run_spec():
        assert spec_fn(None, ptr(y), len(y), n_fft, hop, n_fft, 2.0,
                       ptr(spec), bins, frames) == 0
    def run_mel():
        assert mel_fn(None, ptr(y), len(y), sr, n_fft, hop, n_mels, 0.0, 0.0,
                      ptr(mel), bins, frames) == 0
    def run_mfcc():
        assert mfcc_fn(None, ptr(y), len(y), sr, n_mfcc, n_fft, hop, n_mels,
                       ptr(mfcc), frames) == 0
    t_spec = measured(run_spec)
    t_mel = measured(run_mel)
    t_mfcc = measured(run_mfcc)
    ref_spec = spec_ref_fn()
    ref_mel = mel_ref_fn()
    ref_mfcc = mfcc_ref_fn()
    for name, got, ref, threshold in [
        ("spectrogram", spec, ref_spec, 110),
        ("mel", mel, ref_mel, 110),
        ("mfcc", mfcc, ref_mfcc, 110),
    ]:
        value = snr(got, ref)
        print(f"{name}: SNR vs librosa with matched conventions {value:.1f} dB")
        assert value > threshold, (name, value)

    for new_sr in (8000, 24000):
        output = np.empty((int(np.ceil(len(y)*new_sr/sr)),), np.float32)
        def run_resample():
            assert resample_fn(None, ptr(y), 1, len(y), sr, new_sr,
                               ptr(output), len(output)) == 0
        t_resample = measured(run_resample)
        ref = librosa.resample(y, orig_sr=sr, target_sr=new_sr, res_type="soxr_hq")
        value = snr(output, ref)
        print(f"resample {sr}->{new_sr}: SNR vs soxr_hq {value:.1f} dB; kernel {t_resample:.2f} ms")
        assert value > (40 if new_sr == 8000 else 38), value

    # A 6 kHz component is above the 8 kHz output Nyquist and must be rejected.
    high = np.sin(2*np.pi*6000*np.arange(sr)/sr).astype(np.float32)
    low = np.empty(sr//2, np.float32)
    assert resample_fn(None, ptr(high), 1, len(high), sr, 8000,
                       ptr(low), len(low)) == 0
    alias_db = 20*np.log10(np.sqrt(np.mean(low[100:-100]**2)) /
                           np.sqrt(np.mean(high**2)))
    print(f"downsampling alias rejection: {alias_db:.1f} dB")
    assert alias_db < -80, alias_db

    # time stretch: phase vocoder (librosa/torchaudio algorithm) against a float64 implementation of it.
    # librosa itself is not the reference: it accumulates the phase unwrapped in float32 and is ~29 dB
    # from the float64 result at rate 1.5, while torchaudio and this kernel are ~52-54 dB.
    from scipy.signal.windows import hann as hann_win
    sym = hann_win(n_fft, sym=True)

    def stretch_ref64(x, rate):
        S = librosa.stft(x.astype(np.float64), n_fft=n_fft, hop_length=hop, win_length=n_fft, window=sym,
                         center=True, pad_mode="reflect", dtype=np.complex128)
        adv = hop * np.linspace(0, np.pi, S.shape[0])
        steps = np.arange(0, S.shape[-1], rate)
        Sp = np.pad(S, [(0, 0), (0, 2)])
        acc, out = np.angle(S[:, 0]), np.empty((S.shape[0], len(steps)), np.complex128)
        for t, step in enumerate(steps):
            c, alpha = Sp[:, int(step):int(step) + 2], step % 1
            out[:, t] = ((1 - alpha) * abs(c[:, 0]) + alpha * abs(c[:, 1])) * np.exp(1j * acc)
            d = np.angle(c[:, 1]) - np.angle(c[:, 0]) - adv
            acc += adv + d - 2 * np.pi * np.round(d / (2 * np.pi))
        return librosa.istft(out, hop_length=hop, win_length=n_fft, n_fft=n_fft, window=sym,
                             length=round(len(x) / rate))

    sc_kw = dict(n_fft=n_fft, hop_length=hop, window="hann")
    for rate in (0.5, 0.8, 1.0, 1.5, 2.0, 4.0, 6.0):    # rate > 4 must not truncate the content
        ref_r = stretch_ref64(y, rate)
        out_r = np.empty(round(len(y)/rate), np.float32)
        assert stretch_fn(None, ptr(y), len(y), rate, ptr(out_r), len(out_r)) == 0
        assert len(out_r) == len(ref_r)
        Sa, Sb = np.abs(librosa.stft(out_r, **sc_kw)), np.abs(librosa.stft(ref_r.astype(np.float32), **sc_kw))
        conv = np.linalg.norm(Sa - Sb) / np.linalg.norm(Sb)
        value = snr(out_r, ref_r)
        print(f"time stretch {rate}x vs float64 phase vocoder: SNR {value:.1f} dB, spectral convergence {conv:.4f}")
        assert value > 40 and conv < 0.01, (rate, value, conv)

    pure = np.sin(2*np.pi*440*np.arange(sr*2)/sr).astype(np.float32)
    stretched = np.empty(round(len(pure)/1.5), np.float32)
    assert stretch_fn(None, ptr(pure), len(pure), 1.5, ptr(stretched), len(stretched)) == 0
    segment = stretched[5000:15000]
    peak = np.argmax(np.abs(np.fft.rfft(segment))) * sr / len(segment)
    print(f"time stretch: output {len(stretched)} samples; dominant tone {peak:.1f} Hz")
    assert 430 < peak < 450, peak
    # energy outside +-100 Hz of the tone, over the steady-state part (leakage of the analysis window excluded)
    seg = stretched[4000:-4000] * np.hanning(len(stretched) - 8000)
    P = np.abs(np.fft.rfft(seg)) ** 2
    f = np.fft.rfftfreq(len(seg), 1/sr)
    out_db = 10*np.log10(P[np.abs(f - 440) > 100].sum() / P.sum())
    print(f"time stretch: out-of-band energy {out_db:.1f} dB")
    assert out_db < -90, out_db

    # FFT public API: e^{-i} forward convention like numpy, inverse normalized by 1/n
    for n in (1, 2, 4, 8, 64, 1024):
        z = np.random.default_rng(n).standard_normal(n) + 1j*np.random.default_rng(n + 1).standard_normal(n)
        re, im = z.real.astype(np.float32), z.imag.astype(np.float32)
        z32 = re.astype(np.float64) + 1j*im
        r2, i2 = re.copy(), im.copy()
        assert fft_fn(None, ptr(r2), ptr(i2), n, 0) == 0
        err = np.linalg.norm((r2 + 1j*i2) - np.fft.fft(z32)) / np.linalg.norm(np.fft.fft(z32))
        assert err < 1e-6, (n, err)
        assert fft_fn(None, ptr(r2), ptr(i2), n, 1) == 0
        assert np.linalg.norm((r2 + 1j*i2) - z32) / np.linalg.norm(z32) < 1e-6, n
    print("fft vs numpy (forward sign, inverse, n=1..1024): OK")

    # WAV decode must reproduce soundfile's float32 samples bit for bit (all benchmark signals)
    data_dir = "audio_tests/benchmarks/data"
    for name in sorted(os.listdir(data_dir)):
        if not name.endswith(".wav"):
            continue
        p = os.path.join(data_dir, name)
        ref, _ = sf.read(p, dtype="float32", always_2d=True)       # [T, C]
        info = np.empty(8, np.int32)
        assert info_fn(None, cstr(p), info.ctypes.data_as(ip)) == 0
        _, C, bits, fmt, _, T, _, off = [int(v) for v in info]
        got = np.empty((C, T), np.float32)
        assert read_fn(None, cstr(p), ptr(got), C, T, off, fmt, bits) == 0
        assert np.array_equal(got.T, ref), name
    print("load vs soundfile (bit-exact, all benchmark signals): OK")

    # mu-law: decoded error vs librosa and encode/decode round trip
    xs = np.linspace(-1, 1, 100001, dtype=np.float32)
    codes, dec = np.empty_like(xs), np.empty_like(xs)
    assert mulaw_fn(None, ptr(xs), ptr(codes), len(xs)) == 0
    assert mulaw_dec_fn(None, ptr(codes), ptr(dec), len(xs)) == 0
    err = np.abs(dec - xs).max()
    print(f"mu-law encode->decode max error {err:.4f}")
    assert err < 0.04, err

    # Negative near-zero samples exposed an asymmetric PCM16 rounding bug.
    values = np.array([[-1.0, -1/32768, 0.0, 1/32768, 1.0]], np.float32)
    tmp_path = "/tmp/neve_audio_roundtrip.wav"
    assert save_fn(None, cstr(tmp_path), ptr(values), 1, 5, sr, cstr("s16")) == 0
    got, _ = sf.read(tmp_path, dtype="float32")
    assert np.array_equal(got, np.array([-1, -1/32768, 0, 1/32768, 32767/32768], np.float32))
    info = np.empty(8, np.int32)
    assert info_fn(None, cstr(tmp_path), info.ctypes.data_as(ip)) == 0
    with open(tmp_path, "rb") as f:
        raw = f.read()[:-1]
    with open(tmp_path, "wb") as f:
        f.write(raw)
    assert info_fn(None, cstr(tmp_path), info.ctypes.data_as(ip)) != 0
    print("WAV PCM16 and truncated-file checks: OK")

    matrix = np.ones((3, 4), np.float32)
    assert mask_fn(None, ptr(matrix), 3, 4, 10, -1) == 0
    assert not matrix.any()
    real = np.ones(5, np.float32)
    imag = np.zeros(5, np.float32)
    assert fft_fn(None, ptr(real), ptr(imag), 5, 0) != 0
    print("mask bounds and invalid FFT length: OK")

    print(f"kernel times: spectrogram={t_spec:.2f} ms mel={t_mel:.2f} ms mfcc={t_mfcc:.2f} ms")
    print(f"librosa times: spectrogram={measured(spec_ref_fn):.2f} ms "
          f"mel={measured(mel_ref_fn):.2f} ms "
          f"mfcc={measured(mfcc_ref_fn):.2f} ms")


if __name__ == "__main__":
    main()

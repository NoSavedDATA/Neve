#!/usr/bin/env python3
"""Check the lib/audio C++ kernels against independent float64 / numpy / soundfile / librosa references.

Covers the edge cases that the benchmark signals do not: tiny signals (multiple reflections), n_fft 2..4096,
win < n_fft, odd hops, many resample ratios (including > 4096 phases and several channels), WAV formats
(u8/s16/s24/s32/f32, 1-3 channels, chunk boundaries), non-finite input to save/mu-law.

  load/save/mulaw_decode : bit-exact against soundfile / the closed-form definition
  STFT/spectrogram/mel/MFCC/resample : SNR (dB) against a float64 computation
  mulaw_encode : at most one code of difference (float32 rounding at code boundaries)
  fft : against numpy.fft (forward sign e^-i, inverse normalized by 1/n)
Time stretch is checked in validate_kernels.py (float64 phase vocoder).

Build (the Makefile flags; clang++-21 is the production compiler):
  g++ -DNEVE_AUDIO_STANDALONE -O3 -mavx -fopenmp-simd -fno-math-errno -fno-trapping-math \\
      -std=c++17 -fPIC -shared lib/audio/src/audio.cpp -o /tmp/libneve_audio_test.so
Run from the repository root:
  NEVE_AUDIO_TEST_SO=/tmp/libneve_audio_test.so python3 audio_tests/benchmarks/check_kernels.py
"""
import ctypes as ct
import os
import struct
import sys
import tempfile

import numpy as np
import soundfile as sf

fp = ct.POINTER(ct.c_float)
ip = ct.POINTER(ct.c_int)
DATA = os.path.join(os.path.dirname(os.path.abspath(__file__)), "data")


class DTStr(ct.Structure):
    _fields_ = [("str", ct.c_char_p), ("size", ct.c_int)]


def cstr(value):
    data = os.fsencode(value)
    return DTStr(data, len(data))


def ptr(a):
    return a.ctypes.data_as(fp)


SIGS = {
    "spectrogram": [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_float, fp, ct.c_int, ct.c_int],
    "stft": [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int, fp, fp, ct.c_int, ct.c_int],
    "melspectrogram": [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_float, ct.c_float,
                       fp, ct.c_int, ct.c_int],
    "mfcc": [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, fp, ct.c_int],
    "resample_sinc": [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int, fp, ct.c_int],
    "wav_info_c": [DTStr, ip],
    "read_decode": [DTStr, fp, ct.c_int, ct.c_int, ct.c_long, ct.c_int, ct.c_int],
    "save_wav": [DTStr, fp, ct.c_int, ct.c_int, ct.c_int, DTStr],
    "time_stretch": [fp, ct.c_int, ct.c_float, fp, ct.c_int],
    "mulaw_encode": [fp, fp, ct.c_int],
    "mulaw_decode": [fp, fp, ct.c_int],
    "fft": [fp, fp, ct.c_int, ct.c_int],
    "fade": [fp, ct.c_int, ct.c_int, ct.c_int, ct.c_int],
    "vol": [fp, ct.c_int, ct.c_float, ct.c_int],
}


class Lib:
    def __init__(self, path):
        self.lib = ct.CDLL(path)
        for name, types in SIGS.items():
            fn = getattr(self.lib, "nsk_audio_" + name)
            fn.argtypes = [ct.c_void_p, *types]
            fn.restype = ct.c_int
            setattr(self, name, fn)

    def info(self, path):
        out = np.zeros(8, np.int32)
        rc = self.wav_info_c(None, cstr(path), out.ctypes.data_as(ip))
        return rc, out

    def load(self, path):
        rc, i = self.info(path)
        assert rc == 0, path
        sr, C, bits, fmt, _, T, _, off = [int(v) for v in i]
        out = np.empty((C, T), np.float32)
        assert self.read_decode(None, cstr(path), ptr(out), C, T, off, fmt, bits) == 0
        return out


def snr(a, ref):
    a = np.asarray(a)
    ref = np.asarray(ref)
    if not np.iscomplexobj(a) and not np.iscomplexobj(ref):
        a, ref = a.astype(np.float64), ref.astype(np.float64)
    err = np.sum(np.abs(a - ref) ** 2)
    sig = np.sum(np.abs(ref) ** 2)
    return 999.0 if err == 0 or sig == 0 else 10 * np.log10(sig / err)


def ref_stft64(y, n_fft, hop, win):
    """float64 STFT (e^-i, centered, reflect padded); None when np.pad cannot reflect that far."""
    pad = n_fft // 2
    if len(y) < pad + 1:
        return None
    w = np.zeros(n_fft)
    left = (n_fft - win) // 2
    w[left:left + win] = 1.0 if win == 1 else np.hanning(win)
    yp = np.pad(y.astype(np.float64), pad, mode="reflect")
    frames = 1 + len(y) // hop
    idx = np.arange(n_fft)[None, :] + hop * np.arange(frames)[:, None]
    return np.fft.rfft(yp[idx] * w, axis=1).T      # [bins, frames]


failures = []
rows = []


def check(name, ok, detail):
    print(f"{'OK  ' if ok else 'FAIL'} {name}: {detail}")
    if not ok:
        failures.append(name)


def reflect_idx(pos, T):
    if T == 1:
        return np.zeros_like(pos)
    period = 2 * (T - 1)
    pos = np.mod(pos, period)
    return np.where(pos >= T, period - pos, pos)


def ref_stft64(y, n_fft, hop, win):
    """float64 STFT, e^-i, centered, reflect padded (any number of reflections), win centered in n_fft."""
    w = np.zeros(n_fft)
    left = (n_fft - win) // 2
    w[left:left + win] = 1.0 if win == 1 else np.hanning(win)
    frames = 1 + len(y) // hop
    idx = reflect_idx(np.arange(n_fft)[None, :] + hop * np.arange(frames)[:, None] - n_fft // 2, len(y))
    return np.fft.rfft(y.astype(np.float64)[idx] * w, axis=1).T      # [bins, frames]


def ref_resample64(x, orig_sr, new_sr, taps=64):
    """float64 polyphase windowed-sinc, same definition as the kernel (cutoff 0.99, Hann window, unit-sum rows)."""
    T = len(x)
    To = int((np.int64(T) * new_sr + orig_sr - 1) // orig_sr)
    cutoff = 0.99 * min(1.0, new_sr / orig_sr)
    t = np.arange(To, dtype=np.int64)
    num = t * orig_sr
    center, frac = num // new_sr, (num % new_sr) / new_sr
    out = np.empty(To)
    for s in range(0, To, 4096):
        c, fr = center[s:s + 4096, None], frac[s:s + 4096, None]
        j = np.arange(taps)[None, :]
        d = fr - (j - taps // 2 + 1)
        xx = cutoff * d
        sinc = np.where(np.abs(xx) < 1e-12, 1.0, np.sinc(xx))
        w = cutoff * sinc * 0.5 * (1.0 + np.cos(np.pi * d / (taps // 2)))
        w = w.astype(np.float32).astype(np.float64)
        w = (w / w.sum(axis=1, keepdims=True)).astype(np.float32).astype(np.float64)
        k = np.clip(c + j - taps // 2 + 1, 0, T - 1)
        out[s:s + 4096] = (x.astype(np.float64)[k] * w).sum(axis=1)
    return out


def main():
    import librosa
    from scipy.fft import dct

    lib = Lib(os.environ.get("NEVE_AUDIO_TEST_SO", "/tmp/libneve_audio_test.so"))
    tmp = tempfile.mkdtemp(prefix="neve_audio_chk_")
    rng = np.random.default_rng(7)

    # ------------------------------------------------------------------ load: bit-exact vs soundfile
    files = [os.path.join(DATA, f) for f in sorted(os.listdir(DATA)) if f.endswith(".wav")]
    x = (rng.standard_normal((3, 5000)) * 0.3).astype(np.float32)
    for name, ch, sub in [("u8_mono", 1, "PCM_U8"), ("s24_stereo", 2, "PCM_24"), ("s32_stereo", 2, "PCM_32"),
                          ("s16_3ch", 3, "PCM_16"), ("f32_3ch", 3, "FLOAT"), ("s16_stereo", 2, "PCM_16"),
                          ("f32_mono", 1, "FLOAT"), ("f32_stereo", 2, "FLOAT")]:
        p = os.path.join(tmp, name + ".wav")
        sf.write(p, x[:ch].T, 22050, subtype=sub, format="WAV")
        files.append(p)
    # WAVE_FORMAT_EXTENSIBLE (what ffmpeg writes for > 16 bits, > 2 channels or > 48 kHz)
    for name, ch, sub in [("wavex_s16", 1, "PCM_16"), ("wavex_s24_3ch", 3, "PCM_24"), ("wavex_f32", 2, "FLOAT"),
                          ("wavex_s32", 2, "PCM_32"), ("wavex_u8", 1, "PCM_U8")]:
        p = os.path.join(tmp, name + ".wav")
        sf.write(p, x[:ch].T, 96000, subtype=sub, format="WAVEX")
        files.append(p)
    for p in files:
        ref, _ = sf.read(p, dtype="float32", always_2d=True)
        got = lib.load(p)
        check("load " + os.path.basename(p), np.array_equal(got.T, ref), f"shape {got.shape}")
    # an extensible header whose SubFormat is not PCM/float is still rejected
    with open(os.path.join(tmp, "wavex_s16.wav"), "rb") as f:
        raw = bytearray(f.read())
    i = raw.find(b"fmt ") + 8 + 24                     # SubFormat GUID
    raw[i:i + 2] = b"\x02\x00"                         # ADPCM
    p = os.path.join(tmp, "wavex_adpcm.wav")
    with open(p, "wb") as f:
        f.write(raw)
    check("wav_info rejects extensible ADPCM", lib.info(p)[0] != 0, "")

    def wav_header(channels, sr, bits, data_bytes):
        ba = channels * bits // 8
        return (b"RIFF" + struct.pack("<I", 36 + data_bytes) + b"WAVEfmt " +
                struct.pack("<IHHIIHH", 16, 1, channels, sr, sr * ba, ba, bits) +
                b"data" + struct.pack("<I", data_bytes))
    # the decoded tensor (4 bytes/sample, 32-bit size in Neve) must fit: a sparse 1.2 GB 8-bit file is refused
    p = os.path.join(tmp, "huge_u8.wav")
    with open(p, "wb") as f:
        f.write(wav_header(1, 8000, 8, 1_200_000_000))
        f.truncate(44 + 1_200_000_000)
    check("wav_info rejects tensor > INT_MAX bytes", lib.info(p)[0] != 0, "8-bit, 1.2e9 samples")
    os.unlink(p)
    p = os.path.join(tmp, "ok_u8.wav")
    with open(p, "wb") as f:
        f.write(wav_header(1, 8000, 8, 1000) + bytes(1000))
    check("wav_info accepts small 8-bit file", lib.info(p)[0] == 0, "")
    # a frame larger than the decoder's chunk buffer is refused, not overflowed
    big = np.empty(20000, np.float32)
    rc = lib.read_decode(None, cstr(files[0]), ptr(big), 20000, 1, 0, 3, 32)
    check("read_decode rejects 20000 channels", rc != 0, f"rc {rc}")
    # paths that do not fit are refused, not truncated
    long_path = os.path.join(tmp, "x" * 5000 + ".wav")
    one = np.zeros((1, 4), np.float32)
    check("save_wav rejects a 5000-byte path", lib.save_wav(None, cstr(long_path), ptr(one), 1, 4, 16000, cstr("s16")) != 0, "")
    check("wav_info rejects a 5000-byte path", lib.info(long_path)[0] != 0, "")
    # load on a truncated file must fail and leave a zeroed tensor
    p = os.path.join(tmp, "trunc.wav")
    with open(files[1], "rb") as f:
        raw = f.read()
    rc, info = lib.info(files[1])
    sr, C, bits, fmt, _, T, _, off = [int(v) for v in info]
    with open(p, "wb") as f:
        f.write(raw[:off + 1000])
    out = np.full((C, T), 7.0, np.float32)
    rc = lib.read_decode(None, cstr(p), ptr(out), C, T, off, fmt, bits)
    check("load truncated file", rc != 0 and not out.any(), f"rc {rc}")

    # ------------------------------------------------------------------ save: bit-exact vs closed form
    vals = rng.uniform(-1.3, 1.3, (3, 40000)).astype(np.float32)
    vals[0, :6] = [np.nan, np.inf, -np.inf, 1.0, -1.0, 0.0]
    for C in (1, 2, 3):
        for fmt in ("s16", "f32"):
            for T in (0, 1, 7, 4099, 16383, 16384, 16385, 40000):
                d = np.ascontiguousarray(vals[:C, :T])
                pa = os.path.join(tmp, "a.wav")
                ra = lib.save_wav(None, cstr(pa), ptr(d), C, T, 16000, cstr(fmt))
                if ra != 0:
                    check(f"save C={C} {fmt} T={T}", False, f"rc {ra}")
                    continue
                if T == 0:
                    check(f"save C={C} {fmt} T={T}", os.path.getsize(pa) == 44, "header only")
                    continue
                got, _ = sf.read(pa, dtype="float32", always_2d=True)
                if fmt == "f32":
                    want = np.where(np.isfinite(d), d, d)          # raw bytes, nan/inf preserved
                    ok = np.array_equal(got.T, want, equal_nan=True)
                else:
                    v = np.where(np.isfinite(d), d, 0.0).clip(-1.0, 1.0).astype(np.float32)
                    q = np.clip(np.rint(v.astype(np.float64) * 32768.0), -32768, 32767)
                    ok = np.array_equal(got.T, (q / 32768.0).astype(np.float32))
                check(f"save C={C} {fmt} T={T}", ok, "")

    # ------------------------------------------------------------------ signals
    y10, _ = sf.read(os.path.join(DATA, "sig_10s_16k_s16.wav"), dtype="float32")
    y1, _ = sf.read(os.path.join(DATA, "sig_1s_16k_s16.wav"), dtype="float32")
    sigs = {"10s": y10, "1s": y1, "tiny": y10[:300].copy(), "T=1000": y10[:1000].copy(), "T=3": y10[:3].copy(),
            "T=1": y10[:1].copy()}

    # ------------------------------------------------------------------ stft / spectrogram
    configs = [(1024, 256, 1024), (512, 128, 512), (1024, 256, 400), (256, 64, 256), (2048, 512, 2048),
               (4, 1, 4), (2, 1, 2), (8, 3, 8), (64, 16, 33), (4096, 1024, 4096)]
    for sname, y in sigs.items():
        for n_fft, hop, win in configs:
            bins, frames = n_fft // 2 + 1, 1 + len(y) // hop
            if frames * bins > 4_000_000:
                continue
            S = ref_stft64(y, n_fft, hop, win)
            for power in (2.0, 1.0, 0.5):
                a = np.empty((bins, frames), np.float32)
                ra = lib.spectrogram(None, ptr(y), len(y), n_fft, hop, win, power, ptr(a), bins, frames)
                v = snr(a, np.abs(S) ** power) if ra == 0 else 0.0
                # |X|^0.5 turns float32 rounding noise in near-empty bins into relative error: 55 dB is its floor
                limit = 100 if power >= 1.0 else 55
                check(f"spectrogram {sname} n_fft={n_fft} hop={hop} win={win} p={power}",
                      ra == 0 and (v > limit or np.abs(S).max() < 1e-12), f"SNR vs float64 {v:.1f} dB")
            ar, ai = np.empty((bins, frames), np.float32), np.empty((bins, frames), np.float32)
            rc = lib.stft(None, ptr(y), len(y), n_fft, hop, win, ptr(ar), ptr(ai), bins, frames)
            v = snr(ar.astype(np.float64) + 1j * ai, S)
            check(f"stft {sname} n_fft={n_fft} hop={hop} win={win}", rc == 0 and v > 100, f"SNR {v:.1f} dB")

    # invalid arguments are rejected
    a = np.empty((513, 10), np.float32)
    for n_fft, hop, win in [(1000, 256, 1000), (1024, 0, 1024), (1024, 256, 2048), (1024, 256, 0)]:
        r = lib.spectrogram(None, ptr(y10), len(y10), n_fft, hop, win, 2.0, ptr(a), 513, 10)
        check(f"spectrogram rejects n_fft={n_fft} hop={hop} win={win}", r != 0, f"rc {r}")
    r = lib.spectrogram(None, ptr(y10), len(y10), 1024, 256, 1024, float("nan"), ptr(a), 513, 626)
    check("spectrogram rejects nan power", r != 0, f"rc {r}")

    # ------------------------------------------------------------------ mel / mfcc vs float64 + librosa filter bank
    for sname in ("10s", "1s", "T=1000"):
        y = sigs[sname]
        for (sr, n_fft, hop, n_mels, fmin, fmax) in [(16000, 1024, 256, 128, 0.0, 0.0), (16000, 512, 160, 40, 50.0, 7000.0),
                                                     (22050, 2048, 512, 80, 0.0, 8000.0), (8000, 256, 80, 23, 0.0, 0.0)]:
            bins, frames = n_fft // 2 + 1, 1 + len(y) // hop
            fb = librosa.filters.mel(sr=sr, n_fft=n_fft, n_mels=n_mels, fmin=fmin, fmax=fmax or sr / 2,
                                     htk=True, norm=None)
            P = np.abs(ref_stft64(y, n_fft, hop, n_fft)) ** 2
            ref_mel = fb @ P
            a = np.empty((n_mels, frames), np.float32)
            ra = lib.melspectrogram(None, ptr(y), len(y), sr, n_fft, hop, n_mels, fmin, fmax, ptr(a), bins, frames)
            v = snr(a, ref_mel) if ra == 0 else 0.0
            check(f"mel {sname} sr={sr} n_fft={n_fft} n_mels={n_mels}", ra == 0 and v > 100, f"SNR {v:.1f} dB")
            n_mfcc = min(20, n_mels)
            # nsk_audio_mfcc has no fmin/fmax: it always uses the full 0..sr/2 filter bank
            fb0 = librosa.filters.mel(sr=sr, n_fft=n_fft, n_mels=n_mels, fmin=0.0, fmax=sr / 2, htk=True, norm=None)
            db = 10 * np.log10(np.maximum(fb0 @ P, 1e-10))
            db = np.maximum(db, max(db.max(), -100.0) - 80.0)
            ref_mfcc = dct(db, type=2, axis=0, norm="ortho")[:n_mfcc]
            a = np.empty((n_mfcc, frames), np.float32)
            ra = lib.mfcc(None, ptr(y), len(y), sr, n_mfcc, n_fft, hop, n_mels, ptr(a), frames)
            v = snr(a, ref_mfcc) if ra == 0 else 0.0
            check(f"mfcc {sname} sr={sr} n_fft={n_fft} n_mels={n_mels}", ra == 0 and v > 90, f"SNR {v:.1f} dB")

    # ------------------------------------------------------------------ resample vs float64
    st = np.ascontiguousarray(np.stack([y10[:48000], y10[100:48100] * 0.5]))
    for (osr, nsr, sig) in [(16000, 8000, y10[:48000]), (16000, 24000, y10[:48000]), (44100, 16000, y10[:48000]),
                            (16000, 44100, y10[:48000]), (48000, 44100, y10[:48000]), (22050, 16000, y10[:48000]),
                            (16000, 8000, y10[:40]), (16000, 24000, y10[:1]),
                            (16000, 8000, y10[:63]), (16000, 48000, st), (44101, 16001, y10[:20000]),
                            (44101, 16001, np.ascontiguousarray(st[:, :20000]))]:
        C = sig.shape[0] if sig.ndim == 2 else 1
        T = sig.shape[-1]
        To = int((np.int64(T) * nsr + osr - 1) // osr)
        a = np.empty((C, To), np.float32)
        sig = np.ascontiguousarray(sig)
        ra = lib.resample_sinc(None, ptr(sig), C, T, osr, nsr, ptr(a), To)
        ref = np.stack([ref_resample64(c, osr, nsr) for c in sig.reshape(C, T)])
        v = snr(a, ref) if ra == 0 else 0.0
        check(f"resample {osr}->{nsr} C={C} T={T}", ra == 0 and v > 110, f"SNR vs float64 {v:.1f} dB")

    for C, T in ((1, len(y10)), (2, 48000)):
        sig = np.ascontiguousarray(y10[:C * T].reshape(C, T))
        a = np.empty_like(sig)
        rc = lib.resample_sinc(None, ptr(sig), C, T, 16000, 16000, ptr(a), T)
        check(f"resample same rate is identity C={C}", rc == 0 and np.array_equal(a, sig), "")

    # ------------------------------------------------------------------ mel arguments
    for fmin, fmax in ((float("nan"), 0.0), (0.0, float("nan")), (float("inf"), 0.0), (0.0, float("inf"))):
        y = sigs["1s"]
        frames = 1 + len(y) // 256
        a = np.empty((40, frames), np.float32)
        rc = lib.melspectrogram(None, ptr(y), len(y), 16000, 1024, 256, 40, fmin, fmax, ptr(a), 513, frames)
        check(f"mel rejects fmin={fmin} fmax={fmax}", rc != 0, f"rc {rc}")
    y = sigs["1s"]
    frames = 1 + len(y) // 256
    a = np.empty((40, frames), np.float32)
    rc = lib.melspectrogram(None, ptr(y), len(y), 16000, 1024, 256, 40, 0.0, 4000.0, ptr(a), 513, frames)
    fb = librosa.filters.mel(sr=16000, n_fft=1024, n_mels=40, fmin=0.0, fmax=4000.0, htk=True, norm=None)
    v = snr(a, fb @ (np.abs(ref_stft64(y, 1024, 256, 1024)) ** 2))
    check("mel after rejected NaN calls", rc == 0 and v > 100, f"SNR {v:.1f} dB")

    # ------------------------------------------------------------------ fade / vol (torchaudio semantics)
    for T, fi, fo in ((10, 4, 4), (10, 0, 0), (10, 1, 1), (10, 7, 6), (1000, 400, 300), (5, 10, 10)):
        d = np.ones((2, T), np.float32)
        rc = lib.fade(None, ptr(d), 2, T, fi, fo)
        g = np.ones(T)
        g[:min(fi, T)] *= np.linspace(0, 1, fi)[:min(fi, T)] if fi > 0 else 1
        if fo > 0:
            k = np.arange(T) - (T - fo)
            g[k >= 0] *= np.linspace(1, 0, fo)[k[k >= 0]]
        check(f"fade T={T} in={fi} out={fo}", rc == 0 and np.allclose(d, g[None], atol=1e-6), "")
    d = np.ones(4, np.float32)
    check("fade rejects negative length", lib.fade(None, ptr(d), 1, 4, -1, 0) != 0, "")
    for gt, gain, want in ((0, 2.0, 2.0), (1, 6.0, 10 ** (6 / 20)), (2, 2.0, np.sqrt(2.0))):
        d = np.full(5, 0.25, np.float32)
        rc = lib.vol(None, ptr(d), 5, gain, gt)
        check(f"vol gain_type={gt}", rc == 0 and np.allclose(d, 0.25 * want, rtol=1e-6), f"{d[0]:.6f}")
    for gt, gain in ((3, 2.0), (2, -1.0), (0, float("nan"))):
        d = np.full(5, 0.25, np.float32)
        rc = lib.vol(None, ptr(d), 5, gain, gt)
        check(f"vol rejects gain_type={gt} gain={gain}", rc != 0 and (d == 0.25).all(), "")

    # ------------------------------------------------------------------ time stretch arguments
    o = np.empty(16, np.float32)
    for rate in (1e-30, 0.0, -1.0, float("nan"), float("inf")):
        rc = lib.time_stretch(None, ptr(y10), len(y10), rate, ptr(o), 16)
        check(f"time_stretch rejects rate={rate}", rc != 0, f"rc {rc}")

    # ------------------------------------------------------------------ mulaw
    def mulaw64(v):
        x = np.clip(v.astype(np.float64), -1, 1)
        mag = np.minimum(127, np.floor(np.log10(1 + 255 * np.abs(x)) / np.log10(256.0) * 127 + 0.5))
        return np.where(x < 0, 128, 0) + mag
    a = np.empty_like(y10)
    lib.mulaw_encode(None, ptr(y10), ptr(a), len(y10))
    check("mulaw_encode signal", np.abs(a - mulaw64(y10)).max() <= 1 and (a != mulaw64(y10)).sum() < 20,
          f"{int((a != mulaw64(y10)).sum())} codes differ from float64")
    xs = np.linspace(-1.2, 1.2, 2_000_001, dtype=np.float32)
    a = np.empty_like(xs)
    lib.mulaw_encode(None, ptr(xs), ptr(a), len(xs))
    d = np.abs(a - mulaw64(xs))
    check("mulaw_encode dense sweep", d.max() <= 1 and (d > 0).sum() < 200,
          f"{int((d > 0).sum())} of {len(xs)} differ, max {d.max():.0f}")
    nan = np.array([np.nan, np.inf, -np.inf], np.float32)
    o = np.empty(3, np.float32)
    lib.mulaw_encode(None, ptr(nan), ptr(o), 3)
    check("mulaw_encode non-finite", o[0] == 0 and o[1] == 127 and o[2] == 255, f"{o}")
    codes = np.arange(-300, 600, dtype=np.float32)
    a = np.empty_like(codes)
    lib.mulaw_decode(None, ptr(codes), ptr(a), len(codes))
    c = codes.astype(np.int64)
    ax = ((256.0) ** ((c & 127) / 127.0) - 1) / 255.0
    ref = np.where(((c >> 7) & 1) == 1, -ax, ax)
    check("mulaw_decode all codes", np.abs(a - ref).max() < 1e-6, f"max abs err {np.abs(a - ref).max():.1e}")
    bad = np.array([np.nan, np.inf, -np.inf, 3e9], np.float32)
    o = np.empty_like(bad)
    lib.mulaw_decode(None, ptr(bad), ptr(o), len(bad))
    check("mulaw_decode non-finite / huge codes", (o == 0).all(), f"{o}")

    # ------------------------------------------------------------------ FFT public API
    for n in [1, 2, 4, 8, 16, 64, 1024, 4096]:
        z = rng.standard_normal(n) + 1j * rng.standard_normal(n)
        re, im = z.real.astype(np.float32), z.imag.astype(np.float32)
        z32 = re.astype(np.float64) + 1j * im
        r2, i2 = re.copy(), im.copy()
        assert lib.fft(None, ptr(r2), ptr(i2), n, 0) == 0
        got = r2.astype(np.float64) + 1j * i2
        ref = np.fft.fft(z32)
        e = np.linalg.norm(got - ref) / max(np.linalg.norm(ref), 1e-30)
        r3, i3 = r2.copy(), i2.copy()
        assert lib.fft(None, ptr(r3), ptr(i3), n, 1) == 0
        back = np.linalg.norm((r3 + 1j * i3) - z32) / np.linalg.norm(z32)
        parseval = abs(np.sum(np.abs(got) ** 2) / n / np.sum(np.abs(z32) ** 2) - 1)
        check(f"fft n={n}", e < 1e-6 and back < 1e-6 and parseval < 1e-5,
              f"fwd err {e:.1e} (vs numpy e^-i), roundtrip {back:.1e}, parseval {parseval:.1e}")
    for n in (0, 3, 5, 6, 100):
        r2, i2 = np.zeros(max(n, 1), np.float32), np.zeros(max(n, 1), np.float32)
        check(f"fft rejects n={n}", lib.fft(None, ptr(r2), ptr(i2), n, 0) != 0, "")

    print()
    if failures:
        print(f"{len(failures)} FAILED: {failures[:10]}")
        sys.exit(1)
    print("all checks passed")


if __name__ == "__main__":
    main()

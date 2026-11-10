// Neve audio lib — C kernels (FFI pattern: lib.so).
// Includes the Neve runtime as documented
// (neve-lang.dev/cpp_docs, Extern Functions section).
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <tuple>
#include <limits>
#include <climits>
#include <numeric>
#include <vector>

#ifdef NEVE_AUDIO_STANDALONE
struct Scope_Struct;
struct DT_str { char *str; int size; };
#else
#include "../../../src/nsk_cpp.h"
#endif

// Copies a Neve string into a NUL-terminated path. Fails (instead of truncating)
// when the path does not fit.
static bool c_path(DT_str path, char (&p)[4096]) {
    if (!path.str || path.size <= 0 || path.size >= int(sizeof(p))) return false;
    memcpy(p, path.str, size_t(path.size));
    p[path.size] = 0;
    return true;
}

extern "C" void *nsk_audio_fopen(Scope_Struct *scope_struct, DT_str path, DT_str mode) {
    char p[4096];
    if (!c_path(path, p) || !mode.str || mode.size <= 0) return nullptr;
    char m[8];
    int m_size = mode.size < 7 ? mode.size : 7;
    memcpy(m, mode.str, m_size);
    m[m_size] = 0;
    return (void *)fopen(p, m);
}

// fread/fwrite never touch more than the Neve string's own dst.size/src.size bytes.
extern "C" long nsk_audio_fread(Scope_Struct *scope_struct, DT_str dst, long size, long nmemb, void *f) {
    if (!f || !dst.str || size <= 0 || nmemb <= 0 || dst.size <= 0) return 0;
    return fread(dst.str, size, std::min(nmemb, long(dst.size) / size), (FILE *)f);
}

extern "C" long nsk_audio_fwrite(Scope_Struct *scope_struct, DT_str src, long size, long nmemb, void *f) {
    if (!f || !src.str || size <= 0 || nmemb <= 0 || src.size <= 0) return 0;
    return fwrite(src.str, size, std::min(nmemb, long(src.size) / size), (FILE *)f);
}

extern "C" int nsk_audio_fseek(Scope_Struct *scope_struct, void *f, long offset, int whence) {
    return f ? fseek((FILE *)f, offset, whence) : -1;
}

extern "C" long nsk_audio_ftell(Scope_Struct *scope_struct, void *f) {
    return f ? ftell((FILE *)f) : -1;
}

extern "C" int nsk_audio_fclose(Scope_Struct *scope_struct, void *f) {
    return f ? fclose((FILE *)f) : EOF;
}

extern "C" float nsk_audio_sin(Scope_Struct *scope_struct, float x) { return sinf(x); }
extern "C" float nsk_audio_cos(Scope_Struct *scope_struct, float x) { return cosf(x); }
extern "C" float nsk_audio_exp(Scope_Struct *scope_struct, float x) { return expf(x); }
extern "C" float nsk_audio_log(Scope_Struct *scope_struct, float x) { return logf(x); }
extern "C" float nsk_audio_log10(Scope_Struct *scope_struct, float x) { return log10f(x); }
extern "C" float nsk_audio_fabs(Scope_Struct *scope_struct, float x) { return fabsf(x); }
extern "C" float nsk_audio_atan(Scope_Struct *scope_struct, float x) { return atanf(x); }

extern "C" int nsk_audio_f2i(Scope_Struct *scope_struct, float x) { return (int)lrintf(x); }

// Interval timer: tic() marks the start, toc() returns milliseconds since the last tic().
// The value stays small, so float keeps full resolution however long the process has been running.
static thread_local std::chrono::steady_clock::time_point g_tic;
extern "C" int nsk_audio_tic(Scope_Struct *scope_struct) {
    g_tic = std::chrono::steady_clock::now();
    return 0;
}
extern "C" float nsk_audio_toc(Scope_Struct *scope_struct) {
    using namespace std::chrono;
    return (float)duration<double, std::milli>(steady_clock::now() - g_tic).count();
}

extern "C" bool nsk_audio_isnull(Scope_Struct *scope_struct, void *p) { return p == nullptr; }

// Releases a scratch buffer obtained with c_malloc64 (aligned_alloc) in a wrapper.
extern "C" void nsk_audio_free(Scope_Struct *scope_struct, void *p) { free(p); }

// Zeroes n floats: the wrappers use it so a rejected call never returns uninitialized memory.
extern "C" int nsk_audio_zero(Scope_Struct *scope_struct, float *data, int n) {
    if (data && n > 0) std::memset(data, 0, sizeof(float) * size_t(n));
    return 0;
}

// Decodes a WAV data chunk into a [C, T] float tensor, 64 KB at a time
// (no whole-file staging buffer, no per-sample div/mod).
static int wav_read_decode(DT_str path, float *out, int C, int T, long data_offset, int format, int bits) {
    // A WAV header holds at most 65535 channels, so a frame is at most 65535 bytes (block_align).
    if (!out || C <= 0 || C > 65535 || T < 0 || data_offset < 0 ||
        !((format == 1 && (bits == 8 || bits == 16 || bits == 24 || bits == 32)) ||
          (format == 3 && bits == 32))) return 1;
    char p[4096];
    if (!c_path(path, p)) return 1;
    FILE *f = fopen(p, "rb");
    if (!f) return 1;
    if (fseek(f, data_offset, SEEK_SET) != 0) { fclose(f); return 1; }
    const int bps = bits / 8, frame_bytes = bps * C;
    if (format == 3 && C == 1) {   // little-endian host: read straight into the tensor
        size_t got = fread(out, 4, size_t(T), f);
        fclose(f);
        return got == size_t(T) ? 0 : 1;
    }
    alignas(64) static thread_local unsigned char buf[1 << 16];
    if (frame_bytes > int(sizeof(buf))) { fclose(f); return 1; }   // not a valid WAV frame
    const int per_chunk = int(sizeof(buf)) / frame_bytes;
    for (int t0 = 0; t0 < T; t0 += per_chunk) {
        const int nf = std::min(per_chunk, T - t0);
        if (fread(buf, size_t(frame_bytes), size_t(nf), f) != size_t(nf)) { fclose(f); return 1; }
        const unsigned char *b = buf;
        if (format == 3 && C == 2) {
            float *o0 = out + t0, *o1 = out + size_t(T) + t0;
            for (int t = 0; t < nf; t++) { std::memcpy(o0 + t, b + 8 * t, 4); std::memcpy(o1 + t, b + 8 * t + 4, 4); }
        } else if (format == 3) {
            for (int t = 0; t < nf; t++)
                for (int c = 0; c < C; c++)
                    std::memcpy(&out[size_t(c) * T + t0 + t], b + 4 * (size_t(t) * C + c), 4);
        } else if (bits == 16 && C == 1) {
            const int16_t *s = reinterpret_cast<const int16_t *>(b);
            float *o = out + t0;
            for (int t = 0; t < nf; t++) o[t] = float(s[t]) * (1.0f / 32768.0f);
        } else if (bits == 16) {
            for (int t = 0; t < nf; t++)
                for (int c = 0; c < C; c++) {
                    int16_t v;
                    std::memcpy(&v, b + 2 * (size_t(t) * C + c), 2);
                    out[size_t(c) * T + t0 + t] = float(v) * (1.0f / 32768.0f);
                }
        } else {
            for (int t = 0; t < nf; t++)
                for (int c = 0; c < C; c++) {
                    const unsigned char *q = b + size_t(bps) * (size_t(t) * C + c);
                    float v;
                    if (bits == 8) v = (q[0] - 128) / 128.0f;
                    else if (bits == 24) {
                        int x = int(q[0]) | (int(q[1]) << 8) | (int(q[2]) << 16);
                        if (x >= 8388608) x -= 16777216;
                        v = x / 8388608.0f;
                    } else {
                        int32_t x;
                        std::memcpy(&x, q, 4);
                        v = x / 2147483648.0f;
                    }
                    out[size_t(c) * T + t0 + t] = v;
                }
        }
    }
    fclose(f);
    return 0;
}

// The caller hands us an uninitialized tensor: on failure leave it zeroed, not garbage.
extern "C" int nsk_audio_read_decode(Scope_Struct *scope_struct, DT_str path, float *out,
                                     int C, int T, long data_offset, int format, int bits) {
    int rc = wav_read_decode(path, out, C, T, data_offset, format, bits);
    if (rc != 0 && out && C > 0 && T > 0) std::memset(out, 0, sizeof(float) * size_t(C) * size_t(T));
    return rc;
}

// Scans the RIFF/WAVE header. Fills out[0..7]:
// sr, channels, bits, format, block_align, frames, data_bytes, data_offset.
extern "C" int nsk_audio_wav_info_c(Scope_Struct *scope_struct, DT_str path, int *out) {
    if (!out) return 1;
    for (int i = 0; i < 8; i++) out[i] = 0;
    char p[4096];
    if (!c_path(path, p)) return 1;
    FILE *f = fopen(p, "rb");
    if (!f) return 1;

    unsigned char hdr[12];
    if (fread(hdr, 1, 12, f) < 12) { fclose(f); return 1; }
    if (memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4)) { fclose(f); return 1; }

    bool have_fmt = false, have_data = false;
    while (true) {
        unsigned char chdr[8];
        if (fread(chdr, 1, 8, f) < 8) break;
        uint32_t csize = uint32_t(chdr[4]) | uint32_t(chdr[5]) << 8 |
                         uint32_t(chdr[6]) << 16 | uint32_t(chdr[7]) << 24;
        long chunk_start = ftell(f);
        if (chunk_start < 0 || csize > LONG_MAX - chunk_start - 1) break;
        if (!memcmp(chdr, "fmt ", 4)) {
            if (csize < 16) break;
            unsigned char fmtb[16];
            if (fread(fmtb, 1, 16, f) < 16) break;
            out[3] = fmtb[0] | (fmtb[1] << 8);
            out[1] = fmtb[2] | (fmtb[3] << 8);
            out[0] = (int)((unsigned)fmtb[4] | ((unsigned)fmtb[5] << 8) |
                           ((unsigned)fmtb[6] << 16) | ((unsigned)fmtb[7] << 24));
            out[4] = fmtb[12] | (fmtb[13] << 8);
            out[2] = fmtb[14] | (fmtb[15] << 8);
            if (out[3] == 0xFFFE) {
                // WAVE_FORMAT_EXTENSIBLE: the real format is the first 2 bytes of the SubFormat
                // GUID, which must be the KSDATAFORMAT base {xxxxxxxx-0000-0010-8000-00AA00389B71}.
                static const unsigned char tail[14] = {0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80,
                                                       0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71};
                unsigned char ext[24];   // cbSize, wValidBitsPerSample, dwChannelMask, SubFormat
                if (csize < 40 || fread(ext, 1, 24, f) < 24) break;
                out[3] = memcmp(ext + 10, tail, 14) == 0 ? (ext[8] | (ext[9] << 8)) : 0;
            }
            have_fmt = true;
        } else {
            if (!memcmp(chdr, "data", 4)) {
                if (chunk_start > INT_MAX || csize > INT_MAX) break;
                out[7] = int(chunk_start);
                out[6] = int(csize);
                have_data = true;
            }
        }
        if (have_fmt && have_data) break;
        if (fseek(f, chunk_start + long(csize) + long(csize & 1), SEEK_SET) != 0) break;
    }
    long file_end = -1;
    if (fseek(f, 0, SEEK_END) == 0) file_end = ftell(f);
    fclose(f);
    bool supported = (out[3] == 1 && (out[2] == 8 || out[2] == 16 ||
                       out[2] == 24 || out[2] == 32)) || (out[3] == 3 && out[2] == 32);
    if (!have_fmt || !have_data || !supported || out[0] <= 0 || out[1] <= 0 ||
        out[4] != out[1] * (out[2] / 8) || out[6] % out[4] != 0 ||
        file_end < 0 || int64_t(out[7]) + out[6] > file_end) return 1;
    out[5] = out[6] / out[4];
    // The Neve tensor computes its byte size (4 * C * T) in a 32-bit int: refuse files whose
    // decoded size would overflow it (only reachable with 8-bit files above ~1 GB).
    if (int64_t(out[5]) * out[1] > INT_MAX / 4) return 1;
    return 0;
}

static void wav_write_header(unsigned char *h, int fmt_tag, int channels,
                             int sample_rate, int bits, uint32_t data_bytes) {
    int block_align = channels * (bits / 8);
    uint32_t byte_rate = uint32_t(uint64_t(sample_rate) * block_align);
    auto wr32 = [&](int o, unsigned int v) {
        h[o] = v & 255; h[o+1] = (v >> 8) & 255; h[o+2] = (v >> 16) & 255; h[o+3] = (v >> 24) & 255;
    };
    auto wr16 = [&](int o, unsigned int v) { h[o] = v & 255; h[o+1] = (v >> 8) & 255; };
    wr32(0, 0x46464952u);           // "RIFF"
    wr32(4, 36u + (unsigned int)data_bytes);
    wr32(8, 0x45564157u);           // "WAVE"
    wr32(12, 0x20746D66u);          // "fmt "
    wr32(16, 16);
    wr16(20, (unsigned int)fmt_tag);
    wr16(22, (unsigned int)channels);
    wr32(24, (unsigned int)sample_rate);
    wr32(28, (unsigned int)byte_rate);
    wr16(32, (unsigned int)block_align);
    wr16(34, (unsigned int)bits);
    wr32(36, 0x61746164u);          // "data"
    wr32(40, (unsigned int)data_bytes);
}

// Saves a float [C, T] tensor as WAV. fmt: "s16" or "f32".
extern "C" int nsk_audio_save_wav(Scope_Struct *scope_struct, DT_str path, float *data,
                                  int C, int T, int sample_rate, DT_str fmt) {
    if (!data || C <= 0 || C > 65535 || T < 0 || sample_rate <= 0) return 1;
    char p[4096];
    if (!c_path(path, p)) return 1;
    int bits = 16, fmt_tag = 1;
    if (fmt.size == 3 && memcmp(fmt.str, "f32", 3) == 0) { bits = 32; fmt_tag = 3; }
    else if (fmt.size != 3 || memcmp(fmt.str, "s16", 3) != 0) return 1;

    uint64_t block_align = uint64_t(C) * uint64_t(bits / 8);
    uint64_t data_bytes = uint64_t(T) * block_align;
    if (block_align > 65535 || data_bytes > UINT32_MAX - 36 ||
        uint64_t(sample_rate) * block_align > UINT32_MAX) return 1;

    FILE *f = fopen(p, "wb");
    if (!f) return 1;

    unsigned char hdr[44];
    wav_write_header(hdr, fmt_tag, (int)C, (int)sample_rate, bits, uint32_t(data_bytes));
    if (fwrite(hdr, 1, 44, f) != 44) { fclose(f); return 1; }

    // Interleave and encode 64 KB at a time (little-endian host).
    alignas(64) static thread_local unsigned char buf[1 << 16];
    const int bps = bits / 8, frame_bytes = bps * C;
    const int per_chunk = std::max(1, int(sizeof(buf)) / frame_bytes);
    bool ok = true;
    for (int t0 = 0; t0 < T && ok; t0 += per_chunk) {
        const int nf = std::min(per_chunk, T - t0);
        for (int c = 0; c < C; c++) {
            const float *src = data + size_t(c) * T + t0;
            if (bits == 16) {
                for (int t = 0; t < nf; t++) {
                    float v = src[t];
                    if (!std::isfinite(v)) v = 0.0f;
                    v = std::max(-1.0f, std::min(v, 1.0f));
                    int16_t s = (int16_t)std::max(-32768, std::min(32767, int(lrintf(v * 32768.0f))));
                    std::memcpy(buf + 2 * (size_t(t) * C + c), &s, 2);
                }
            } else {
                for (int t = 0; t < nf; t++) std::memcpy(buf + 4 * (size_t(t) * C + c), src + t, 4);
            }
        }
        ok = fwrite(buf, size_t(frame_bytes), size_t(nf), f) == size_t(nf);
    }
    if (fclose(f) != 0) ok = false;
    return ok ? 0 : 1;
}

// ---------------- DSP (torchaudio.transforms style) ----------------
//
// FFT convention: forward transform is e^{-i...} (numpy/torch), so STFT/FFT
// imaginary parts have the standard sign.

namespace {

// Complex FFT plan, N = M a power of two (>= 1). Input must be bit-reversed
// (see `rev`); the transform then runs in place with contiguous twiddle tables.
struct CPlan {
    int M;
    std::vector<int> rev;
    std::vector<float> twr, twi;   // per-stage twiddles, stage h at offset h-1
    explicit CPlan(int m) : M(m), rev(m), twr(m), twi(m) {
        int lg = 0;
        while ((1 << lg) < M) lg++;
        for (int i = 0; i < M; i++) {
            int r = 0;
            for (int b = 0; b < lg; b++) if (i & (1 << b)) r |= 1 << (lg - 1 - b);
            rev[i] = r;
        }
        for (int h = 1; h < M; h <<= 1)
            for (int j = 0; j < h; j++) {
                double a = -M_PI * j / h;   // twiddles in double, stored as float
                twr[h - 1 + j] = float(std::cos(a));
                twi[h - 1 + j] = float(std::sin(a));
            }
    }
    void run(float *__restrict zr, float *__restrict zi) const {
        int h = 1;
        if (M >= 4) {   // stages h=1 and h=2 fused (radix-4)
            for (int i = 0; i < M; i += 4) {
                float a0r = zr[i], a0i = zi[i], a1r = zr[i + 1], a1i = zi[i + 1];
                float a2r = zr[i + 2], a2i = zi[i + 2], a3r = zr[i + 3], a3i = zi[i + 3];
                float b0r = a0r + a1r, b0i = a0i + a1i, b1r = a0r - a1r, b1i = a0i - a1i;
                float b2r = a2r + a3r, b2i = a2i + a3i, b3r = a2r - a3r, b3i = a2i - a3i;
                zr[i] = b0r + b2r;     zi[i] = b0i + b2i;
                zr[i + 2] = b0r - b2r; zi[i + 2] = b0i - b2i;
                zr[i + 1] = b1r + b3i; zi[i + 1] = b1i - b3r;
                zr[i + 3] = b1r - b3i; zi[i + 3] = b1i + b3r;
            }
            h = 4;
        }
        for (; h < M; h <<= 1) {
            const float *wr = twr.data() + h - 1, *wi = twi.data() + h - 1;
            for (int i = 0; i < M; i += 2 * h) {
                float *ar = zr + i, *ai = zi + i, *br = zr + i + h, *bi = zi + i + h;
#pragma omp simd
                for (int j = 0; j < h; j++) {
                    float tr = br[j] * wr[j] - bi[j] * wi[j];
                    float ti = br[j] * wi[j] + bi[j] * wr[j];
                    br[j] = ar[j] - tr; bi[j] = ai[j] - ti;
                    ar[j] += tr;        ai[j] += ti;
                }
            }
        }
    }
};

// Real FFT of size N (power of two, >= 2) as a complex FFT of size N/2 plus a
// split step.
struct RFFT {
    int N, M;
    CPlan c;
    std::vector<float> rtr, rti;   // W_N^k, k in [0, M)
    explicit RFFT(int n) : N(n), M(n / 2), c(n / 2), rtr(n / 2), rti(n / 2) {
        for (int k = 0; k < M; k++) {
            double a = -2.0 * M_PI * k / N;
            rtr[k] = float(std::cos(a));
            rti[k] = float(std::sin(a));
        }
    }
    // Windowed real frame -> packed, bit-reversed complex input.
    void pack(const float *x, const float *w, float *zr, float *zi) const {
        for (int n = 0; n < M; n++) {
            int r = c.rev[n];
            zr[r] = x[2 * n] * w[2 * n];
            zi[r] = x[2 * n + 1] * w[2 * n + 1];
        }
    }
    // Z (size M) -> X[k], k = 0..M.
    void post(const float *zr, const float *zi, float *Xr, float *Xi) const {
        Xr[0] = zr[0] + zi[0]; Xi[0] = 0.0f;
        Xr[M] = zr[0] - zi[0]; Xi[M] = 0.0f;
        for (int k = 1; k < M; k++) {
            float ar = zr[k], ai = zi[k], br = zr[M - k], bi = -zi[M - k];
            float er = 0.5f * (ar + br), ei = 0.5f * (ai + bi);
            float dr = 0.5f * (ar - br), di = 0.5f * (ai - bi);
            float o_r = di, o_i = -dr;
            Xr[k] = er + (o_r * rtr[k] - o_i * rti[k]);
            Xi[k] = ei + (o_r * rti[k] + o_i * rtr[k]);
        }
    }
    // X[k], k = 0..M -> x[0..N) (numpy.irfft normalization). zr/zi: scratch of size M.
    void inverse(const float *Xr, const float *Xi, float *x, float *zr, float *zi) const {
        for (int k = 0; k < M; k++) {
            float ar = Xr[k], ai = Xi[k], br = Xr[M - k], bi = -Xi[M - k];
            float er = 0.5f * (ar + br), ei = 0.5f * (ai + bi);
            float dr = 0.5f * (ar - br), di = 0.5f * (ai - bi);
            float wr = rtr[k], wi = -rti[k];
            float o_r = dr * wr - di * wi, o_i = dr * wi + di * wr;
            int r = c.rev[k];
            zr[r] = er - o_i;
            zi[r] = -(ei + o_r);
        }
        c.run(zr, zi);
        float s = 1.0f / float(M);
        for (int n = 0; n < M; n++) { x[2 * n] = zr[n] * s; x[2 * n + 1] = -zi[n] * s; }
    }
};

struct STFTPlan {
    RFFT fft;
    std::vector<float> win;   // symmetric Hann of length `win`, centered in n_fft
    STFTPlan(int n_fft, int w) : fft(n_fft), win(n_fft, 0.0f) {
        const float PI = 3.14159265358979f;   // same float formula as nsk_audio_hann
        int left = (n_fft - w) / 2;
        for (int i = 0; i < w; i++)
            win[left + i] = w == 1 ? 1.0f : 0.5f * (1.0f - cosf(2.0f * PI * float(i) / float(w - 1)));
    }
};

// Plans are immutable once built; the mutex only guards creation. A returned
// shared_ptr stays valid even if the (bounded) cache is cleared meanwhile.
template <class K, class V>
struct PlanCache {
    std::mutex mu;
    std::map<K, std::shared_ptr<const V>> m;
    template <class F>
    std::shared_ptr<const V> get(const K &k, F make) {
        std::lock_guard<std::mutex> lk(mu);
        auto it = m.find(k);
        if (it != m.end()) return it->second;
        if (m.size() >= 32) m.clear();
        std::shared_ptr<const V> p(new V(make()));
        m[k] = p;
        return p;
    }
};

std::shared_ptr<const STFTPlan> stft_plan(int n_fft, int win) {
    static auto *cache = new PlanCache<std::pair<int, int>, STFTPlan>;
    return cache->get({n_fft, win}, [&] { return STFTPlan(n_fft, win); });
}

std::shared_ptr<const CPlan> cfft_plan(int n) {
    static auto *cache = new PlanCache<int, CPlan>;
    return cache->get(n, [&] { return CPlan(n); });
}

inline int reflect(int pos, int T) {
    if (pos >= 0 && pos < T) return pos;
    if (T == 1) return 0;
    int period = 2 * (T - 1);
    pos %= period;
    if (pos < 0) pos += period;
    if (pos >= T) pos = period - pos;
    return pos;
}

// Frame t of the centered, reflect-padded signal: a pointer into wf when the
// frame is interior, otherwise a reflected copy in `frame`.
inline const float *frame_ptr(const float *wf, int T, int n_fft, int hop, int t, float *frame) {
    int start = t * hop - n_fft / 2;
    if (start >= 0 && start + n_fft <= T) return wf + start;
    for (int i = 0; i < n_fft; i++) frame[i] = wf[reflect(start + i, T)];
    return frame;
}

// Natural log for normal y > 0, ~1 ulp; branch-free (musl-style reduction) so loops vectorize.
inline float fast_ln(float y) {
    uint32_t ix;
    std::memcpy(&ix, &y, 4);
    ix += 0x3F800000u - 0x3F3504F3u;               // reduce mantissa to [sqrt(.5), sqrt(2))
    int k = int(ix >> 23) - 0x7F;
    ix = (ix & 0x007FFFFFu) + 0x3F3504F3u;
    float m;
    std::memcpy(&m, &ix, 4);
    float s = (m - 1.0f) / (m + 1.0f), s2 = s * s;
    float p = 2.0f * s * (1.0f + s2 * (0.33333333f + s2 * (0.2f + s2 * (0.14285714f + s2 * 0.11111111f))));
    return float(k) * 0.69314718f + p;
}

// atan2 with the cephes atanf polynomial (~1e-7 rad); selects instead of branches.
inline float fast_atan2(float y, float x) {
    const float PI = 3.14159265358979f, PI_2 = 1.57079632679f, PI_4 = 0.785398163397f;
    float ax = std::fabs(x), ay = std::fabs(y);
    float mx = std::max(ax, ay), mn = std::min(ax, ay);
    float a = mn / (mx > 0.0f ? mx : 1.0f);            // [0, 1]
    bool red = a > 0.414213562f;
    float z = red ? (a - 1.0f) / (a + 1.0f) : a;
    float z2 = z * z;
    float r = (((8.05374449538e-2f * z2 - 1.38776856032e-1f) * z2 + 1.99777106478e-1f) * z2 - 3.33329491539e-1f) * z2 * z + z;
    r = red ? r + PI_4 : r;
    r = ay > ax ? PI_2 - r : r;
    r = x < 0.0f ? PI - r : r;
    return y < 0.0f ? -r : r;
}

// sin/cos for |t| <= ~4 (callers wrap to [-pi, pi]); cephes polynomials on [-pi/4, pi/4].
inline void fast_sincos(float t, float *s, float *c) {
    float q = rintf(t * 0.636619772f);                  // quadrant
    float r = (t - q * 1.5703125f) - q * 4.83826794897e-4f;
    float r2 = r * r;
    float sp = ((-1.9515295891e-4f * r2 + 8.3321608736e-3f) * r2 - 1.6666654611e-1f) * r2 * r + r;
    float cp = ((2.443315711809948e-5f * r2 - 1.388731625493765e-3f) * r2 + 4.166664568298827e-2f) * r2 * r2 - 0.5f * r2 + 1.0f;
    int qi = int(q) & 3;
    float ss = (qi & 1) ? cp : sp, cc = (qi & 1) ? sp : cp;
    *s = (qi & 2) ? -ss : ss;
    *c = ((qi + 1) & 2) ? -cc : cc;
}

inline float wrap_pi(float x) {
    return x - 6.28318530717958647f * rintf(x * 0.15915494309189535f);
}

constexpr int kBlock = 16;   // STFT frames processed together

// Working memory for the blocked power-spectrum pipeline.
struct Scratch {
    std::vector<float> v;
    float *frame, *zr, *zi, *Xr, *Xi, *powT;
    Scratch(int n_fft, int B)
        : v(size_t(n_fft) + n_fft + 2 * (n_fft / 2 + 1) + size_t(n_fft / 2 + 1) * B) {
        frame = v.data(); zr = frame + n_fft; zi = zr + n_fft / 2;
        Xr = zi + n_fft / 2; Xi = Xr + n_fft / 2 + 1; powT = Xi + n_fft / 2 + 1;
    }
};

// Power spectrum of `nb` consecutive frames from t0 into powT[k*B + j] (bin-major block).
void power_block(const STFTPlan &P, const float *wf, int T, int hop, int t0, int nb, Scratch &s) {
    const int n_fft = P.fft.N, M = P.fft.M, B = kBlock;
    for (int j = 0; j < nb; j++) {
        const float *x = frame_ptr(wf, T, n_fft, hop, t0 + j, s.frame);
        P.fft.pack(x, P.win.data(), s.zr, s.zi);
        P.fft.c.run(s.zr, s.zi);
        P.fft.post(s.zr, s.zi, s.Xr, s.Xi);
        for (int k = 0; k <= M; k++) s.powT[k * B + j] = s.Xr[k] * s.Xr[k] + s.Xi[k] * s.Xi[k];
    }
}

bool stft_args_ok(const float *wf, int T, int n_fft, int hop, int win, int bins, int frames) {
    return wf && T > 0 && hop > 0 && n_fft >= 2 && !(n_fft & (n_fft - 1)) && win > 0 &&
           win <= n_fft && bins == n_fft / 2 + 1 && frames == 1 + T / hop;
}

}  // namespace

// In-place complex FFT on re/im buffers of size n (power of 2); inverse is
// normalized by 1/n.
extern "C" int nsk_audio_fft(Scope_Struct *scope_struct, float *re, float *im, int n, int inverse) {
    if (!re || !im || n <= 0 || (n & (n - 1))) return 1;
    auto plan = cfft_plan(n);
    std::vector<float> zr(n), zi(n);
    const float sgn = inverse ? -1.0f : 1.0f;     // IFFT(x) = conj(FFT(conj(x))) / n
    for (int i = 0; i < n; i++) { zr[plan->rev[i]] = re[i]; zi[plan->rev[i]] = sgn * im[i]; }
    plan->run(zr.data(), zi.data());
    const float s = inverse ? 1.0f / float(n) : 1.0f;
    for (int i = 0; i < n; i++) { re[i] = zr[i] * s; im[i] = sgn * zi[i] * s; }
    return 0;
}

// Symmetric Hann: win[i] = 0.5*(1-cos(2*pi*i/(n-1))).
extern "C" int nsk_audio_hann(Scope_Struct *scope_struct, float *win, int n) {
    if (!win || n <= 0) return 1;
    const float PI = 3.14159265358979f;
    if (n == 1) { win[0] = 1.0f; return 0; }
    for (int i = 0; i < n; i++) win[i] = 0.5f * (1.0f - cosf(2.0f*PI*(float)i/(float)(n-1)));
    return 0;
}

// Single-channel STFT: hann window, reflection center padding.
// wf has T samples; re/im receive [bins x frames], bins = n_fft/2+1.
extern "C" int nsk_audio_stft(Scope_Struct *scope_struct, float *wf, int T,
                              int n_fft, int hop, int win,
                              float *re, float *im, int bins, int frames) {
    if (!re || !im || !stft_args_ok(wf, T, n_fft, hop, win, bins, frames)) return 1;
    auto P = stft_plan(n_fft, win);
    const int M = n_fft / 2;
    std::vector<float> frame(n_fft), zr(M), zi(M), Xr(M + 1), Xi(M + 1);
    for (int t = 0; t < frames; t++) {
        const float *x = frame_ptr(wf, T, n_fft, hop, t, frame.data());
        P->fft.pack(x, P->win.data(), zr.data(), zi.data());
        P->fft.c.run(zr.data(), zi.data());
        P->fft.post(zr.data(), zi.data(), Xr.data(), Xi.data());
        for (int b = 0; b < bins; b++) {
            re[size_t(b) * frames + t] = Xr[b];
            im[size_t(b) * frames + t] = Xi[b];
        }
    }
    return 0;
}

// Spectrogram: |STFT|^power -> out [bins x frames]. Frames are processed in
// blocks, so no complex intermediate matrix is ever materialized.
extern "C" int nsk_audio_spectrogram(Scope_Struct *scope_struct, float *wf, int T,
                                     int n_fft, int hop, int win, float power,
                                     float *out, int bins, int frames) {
    if (!out || !std::isfinite(power) || power < 0.0f || bins <= 0 || frames <= 0 ||
        !stft_args_ok(wf, T, n_fft, hop, win, bins, frames)) return 1;
    auto P = stft_plan(n_fft, win);
    Scratch s(n_fft, kBlock);
    for (int t0 = 0; t0 < frames; t0 += kBlock) {
        int nb = std::min(kBlock, frames - t0);
        power_block(*P, wf, T, hop, t0, nb, s);
        for (int k = 0; k < bins; k++) {
            float *dst = out + size_t(k) * frames + t0;
            const float *src = s.powT + k * kBlock;
            if (power == 2.0f) std::memcpy(dst, src, nb * sizeof(float));
            else if (power == 1.0f) for (int j = 0; j < nb; j++) dst[j] = sqrtf(src[j]);
            else for (int j = 0; j < nb; j++) dst[j] = powf(src[j], power * 0.5f);
        }
    }
    return 0;
}

// STFT shape helpers for the Neve wrappers (0 on invalid arguments).
extern "C" int nsk_audio_stft_bins(Scope_Struct *scope_struct, int n_fft) {
    return (n_fft < 2 || (n_fft & (n_fft - 1))) ? 0 : n_fft / 2 + 1;
}
extern "C" int nsk_audio_stft_frames(Scope_Struct *scope_struct, int T, int hop) {
    return (T <= 0 || hop <= 0 || 1LL + T / hop > INT_MAX) ? 0 : 1 + T / hop;
}
extern "C" int nsk_audio_resample_len(Scope_Struct *scope_struct, int T, int orig_sr, int new_sr) {
    if (T < 0 || orig_sr <= 0 || new_sr <= 0 ||
        (int64_t(T) * new_sr + orig_sr - 1) / orig_sr > INT_MAX) return 0;
    return int((int64_t(T) * new_sr + orig_sr - 1) / orig_sr);
}

// amplitude -> dB (ref=1.0): 20*log10(max(x, 1e-10)).
extern "C" int nsk_audio_amplitude_to_db(Scope_Struct *scope_struct, float *in, float *out, int n) {
    for (int i = 0; i < n; i++) {
        float v = in[i] > 1e-10f ? in[i] : 1e-10f;
        out[i] = 20.0f * log10f(v);
    }
    return 0;
}

// dB -> amplitude (ref=1.0): 10^(x/20).
extern "C" int nsk_audio_db_to_amplitude(Scope_Struct *scope_struct, float *in, float *out, int n) {
    for (int i = 0; i < n; i++) out[i] = powf(10.0f, in[i] / 20.0f);
    return 0;
}

// ---------------- Advanced DSP ----------------

// Per-channel linear resampling. in [C x T], out [C x T_out].
extern "C" int nsk_audio_resample_linear(Scope_Struct *scope_struct, float *in, int C, int T,
                                         int orig_sr, int new_sr, float *out, int T_out) {
    if (!in || !out || C <= 0 || T <= 0 || orig_sr <= 0 || new_sr <= 0 || T_out < 0) return 1;
    double ratio = double(orig_sr) / double(new_sr);
    for (int c = 0; c < C; c++) {
        for (int t = 0; t < T_out; t++) {
            double pos = double(t) * ratio;
            int p0 = pos >= T - 1 ? T - 1 : int(pos);
            float frac = float(pos - p0);
            if (p0 >= T-1) out[c*T_out + t] = in[c*T + T-1];
            else out[c*T_out + t] = in[c*T + p0]*(1.0f-frac) + in[c*T + p0+1]*frac;
        }
    }
    return 0;
}

namespace {
constexpr int kTaps = 64;

// One row of the polyphase bandlimited-sinc bank: kTaps weights for fractional
// phase `phase`/`phases`, normalized to unit sum.
void sinc_row(int orig_sr, int new_sr, int phases, int phase, float *weights) {
    constexpr double PI = 3.14159265358979323846;
    double cutoff = 0.99 * std::min(1.0, double(new_sr) / orig_sr);
    double fraction = double(phase) / phases, sum = 0.0;
    for (int j = 0; j < kTaps; ++j) {
        double d = fraction - (j - kTaps / 2 + 1);
        double x = cutoff * d;
        double sinc = std::abs(x) < 1e-12 ? 1.0 : std::sin(PI * x) / (PI * x);
        double window = 0.5 * (1.0 + std::cos(PI * d / (kTaps / 2)));
        weights[j] = float(cutoff * sinc * window);
        sum += weights[j];
    }
    for (int j = 0; j < kTaps; ++j) weights[j] = float(weights[j] / sum);
}

// All rows for one (orig_sr, new_sr) pair; cached across calls.
std::vector<float> build_sinc_bank(int orig_sr, int new_sr) {
    int phases = new_sr / std::gcd(orig_sr, new_sr);
    std::vector<float> bank(size_t(phases) * kTaps);
    for (int p = 0; p < phases; ++p) sinc_row(orig_sr, new_sr, phases, p, bank.data() + size_t(p) * kTaps);
    return bank;
}

// 64-tap dot product: two 8-lane accumulators (GCC/Clang vector extensions,
// which both compilers lower well; plain loops differ a lot between them).
inline float dot64(const float *x, const float *w) {
    typedef float v8f __attribute__((vector_size(32), aligned(4)));
    v8f s0 = {0}, s1 = {0};
    for (int j = 0; j < kTaps; j += 16) {
        v8f a0, b0, a1, b1;
        std::memcpy(&a0, x + j, 32);     std::memcpy(&b0, w + j, 32);
        std::memcpy(&a1, x + j + 8, 32); std::memcpy(&b1, w + j + 8, 32);
        s0 += a0 * b0; s1 += a1 * b1;
    }
    s0 += s1;
    return ((s0[0] + s0[4]) + (s0[1] + s0[5])) + ((s0[2] + s0[6]) + (s0[3] + s0[7]));
}
}  // namespace

// Per-channel polyphase bandlimited sinc (64 taps). in [C x T], out [C x T_out].
extern "C" int nsk_audio_resample_sinc(Scope_Struct *scope_struct, float *in, int C, int T,
                                       int orig_sr, int new_sr, float *out, int T_out) {
    if (!in || !out || C <= 0 || T <= 0 || orig_sr <= 0 || new_sr <= 0 || T_out < 0) return 1;
    if (orig_sr == new_sr) {   // identity (like torchaudio), not a low-pass
        for (int c = 0; c < C; c++)
            for (int t = 0; t < T_out; t++) out[size_t(c) * T_out + t] = in[size_t(c) * T + std::min(t, T - 1)];
        return 0;
    }
    const int divisor = std::gcd(orig_sr, new_sr);
    const int phases = new_sr / divisor, step = orig_sr / divisor;
    // Position of output t is t*step/phases: advance (center, phase) incrementally.
    const int cstep = step / phases, pstep = step % phases;
    auto tap = [&](const float *channel, int center, const float *w) {
        const int first = center - kTaps / 2 + 1;
        if (first >= 0 && first <= T - kTaps) return dot64(channel + first, w);
        float acc = 0.0f;
        for (int j = 0; j < kTaps; ++j) acc += channel[std::max(0, std::min(T - 1, first + j))] * w[j];
        return acc;
    };
    if (phases <= 4096) {
        static auto *cache = new PlanCache<std::pair<int, int>, std::vector<float>>;
        auto bank = cache->get({orig_sr, new_sr}, [&] { return build_sinc_bank(orig_sr, new_sr); });
        for (int c = 0; c < C; c++) {
            const float *channel = in + size_t(c) * T;
            int center = 0, phase = 0;
            for (int t = 0; t < T_out; t++) {
                out[size_t(c) * T_out + t] = tap(channel, center, bank->data() + size_t(phase) * kTaps);
                center += cstep;
                phase += pstep;
                if (phase >= phases) { phase -= phases; center++; }
            }
        }
        return 0;
    }
    // Unusual rate pairs: no table (it would be unbounded); build each row once and use it
    // for every channel.
    std::vector<float> row(kTaps);
    int center = 0;
    int64_t phase = 0;
    for (int t = 0; t < T_out; t++) {
        sinc_row(orig_sr, new_sr, phases, int(phase), row.data());
        for (int c = 0; c < C; c++) out[size_t(c) * T_out + t] = tap(in + size_t(c) * T, center, row.data());
        center += cstep;
        phase += pstep;
        if (phase >= phases) { phase -= phases; center++; }
    }
    return 0;
}

namespace {
float hz_to_mel(float f) { return 2595.0f * log10f(1.0f + f / 700.0f); }
float mel_to_hz(float m) { return 700.0f * (powf(10.0f, m / 2595.0f) - 1.0f); }

// Sparse triangular (HTK) filter bank: filter m covers bins [b0[m], b0[m]+len[m])
// with weights w[off[m] ...].
struct MelPlan {
    std::vector<int> b0, len, off;
    std::vector<float> w;
};

MelPlan build_mel_plan(int sr, int bins, int n_mels, float fmin, float fmax) {
    MelPlan p;
    float mlo = hz_to_mel(fmin), mhi = hz_to_mel(fmax);
    int n_fft_full = (bins - 1) * 2;
    for (int m = 0; m < n_mels; m++) {
        float l = mlo + (mhi - mlo) * (float)m / (float)(n_mels + 1);
        float c = mlo + (mhi - mlo) * (float)(m + 1) / (float)(n_mels + 1);
        float r = mlo + (mhi - mlo) * (float)(m + 2) / (float)(n_mels + 1);
        float fl = mel_to_hz(l), fc = mel_to_hz(c), fr = mel_to_hz(r);
        int b0 = std::max(0, int(std::ceil(double(fl) * n_fft_full / sr)));
        int b1 = std::min(bins - 1, int(std::floor(double(fr) * n_fft_full / sr)));
        p.b0.push_back(b0);
        p.off.push_back(int(p.w.size()));
        p.len.push_back(std::max(0, b1 - b0 + 1));
        for (int b = b0; b <= b1; b++) {
            float f = (float)b * (float)sr / (float)n_fft_full, v = 0.0f;
            if (f >= fl && f <= fc && fc > fl) v = (f - fl) / (fc - fl);
            else if (f > fc && f <= fr && fr > fc) v = (fr - f) / (fr - fc);
            p.w.push_back(v);
        }
    }
    return p;
}
}  // namespace

// (HTK) mel spectrogram of a single channel. out [n_mels x frames]. The power
// spectrum is filtered block by block and never stored in full.
extern "C" int nsk_audio_melspectrogram(Scope_Struct *scope_struct, float *wf, int T, int sr,
                                        int n_fft, int hop, int n_mels,
                                        float fmin, float fmax,
                                        float *out, int bins, int frames) {
    if (!wf || !out || sr <= 0 || n_mels <= 0 || bins <= 1 || frames <= 0 ||
        !std::isfinite(fmin) || !std::isfinite(fmax) || fmin < 0.0f || fmin >= sr/2.0f ||
        !stft_args_ok(wf, T, n_fft, hop, n_fft, bins, frames)) return 1;
    float nyq = (float)sr / 2.0f;
    if (fmax <= 0.0f || fmax > nyq) fmax = nyq;
    if (fmax <= fmin) return 1;
    auto P = stft_plan(n_fft, n_fft);
    static auto *cache = new PlanCache<std::tuple<int, int, int, float, float>, MelPlan>;
    auto F = cache->get({sr, bins, n_mels, fmin, fmax}, [&] { return build_mel_plan(sr, bins, n_mels, fmin, fmax); });
    Scratch s(n_fft, kBlock);
    for (int t0 = 0; t0 < frames; t0 += kBlock) {
        int nb = std::min(kBlock, frames - t0);
        power_block(*P, wf, T, hop, t0, nb, s);
        for (int m = 0; m < n_mels; m++) {
            float acc[kBlock] = {0};
            const float *w = F->w.data() + F->off[m];
            for (int i = 0; i < F->len[m]; i++) {
                const float *src = s.powT + (F->b0[m] + i) * kBlock;
                for (int j = 0; j < kBlock; j++) acc[j] += w[i] * src[j];
            }
            std::memcpy(out + size_t(m) * frames + t0, acc, nb * sizeof(float));
        }
    }
    return 0;
}

// MFCC: log-mel -> DCT-II. out [n_mfcc x frames].
extern "C" int nsk_audio_mfcc(Scope_Struct *scope_struct, float *wf, int T, int sr,
                              int n_mfcc, int n_fft, int hop, int n_mels,
                              float *out, int frames) {
    if (!wf || !out || n_mfcc <= 0 || n_mfcc > n_mels || n_mels <= 0 || frames <= 0) return 1;
    int bins = n_fft/2 + 1;
    std::vector<float> lm(size_t(n_mels) * frames);
    if (nsk_audio_melspectrogram(scope_struct, wf, T, sr, n_fft, hop, n_mels, 0.0f, 0.0f, lm.data(), bins, frames))
        return 1;
    const size_t n = lm.size();
    const float k10 = 10.0f / 2.30258509f;   // 10*log10(x) = (10/ln 10) * ln(x)
    for (size_t i = 0; i < n; i++) lm[i] = k10 * fast_ln(std::max(lm[i], 1e-10f));
    float max_db = -100.0f;
    for (size_t i = 0; i < n; i++) max_db = std::max(max_db, lm[i]);
    const float floor_db = max_db - 80.0f;
    for (size_t i = 0; i < n; i++) lm[i] = std::max(lm[i], floor_db);
    std::vector<float> dct(size_t(n_mfcc) * n_mels);
    const float PI = 3.14159265358979f;
    for (int m = 0; m < n_mfcc; m++) {
        float scale = sqrtf((m == 0 ? 1.0f : 2.0f) / float(n_mels));
        for (int k = 0; k < n_mels; k++)
            dct[size_t(m) * n_mels + k] = scale * cosf(PI * float(m) * (float(k) + 0.5f) / float(n_mels));
    }
    for (int m = 0; m < n_mfcc; m++) {
        float *dst = out + size_t(m) * frames;
        std::fill(dst, dst + frames, 0.0f);
        for (int k = 0; k < n_mels; k++) {
            const float c = dct[size_t(m) * n_mels + k];
            const float *src = lm.data() + size_t(k) * frames;
            for (int t = 0; t < frames; t++) dst[t] += c * src[t];
        }
    }
    return 0;
}

// MuLaw: in [n] float in [-1,1] -> out [n] float with codes 0..255
// (bit7 = sign, bits 0..6 = magnitude 0..127).
extern "C" int nsk_audio_mulaw_encode(Scope_Struct *scope_struct, float *in, float *out, int n) {
    const float scale = 127.0f / 5.54517744f;   // 127 / ln(256)
#pragma omp simd
    for (int i = 0; i < n; i++) {
        float v = in[i];
        float x = v > 1.0f ? 1.0f : (v < -1.0f ? -1.0f : v);
        float ax = std::fabs(x);
        ax = ax == ax ? ax : 0.0f;               // NaN -> code 0
        int mag = int(fast_ln(1.0f + 255.0f * ax) * scale + 0.5f);
        mag = mag > 127 ? 127 : mag;
        out[i] = (x < 0.0f ? 128.0f : 0.0f) + float(mag);
    }
    return 0;
}

extern "C" int nsk_audio_mulaw_decode(Scope_Struct *scope_struct, float *in, float *out, int n) {
    static const std::vector<float> table = [] {   // magnitude -> amplitude
        const float MU = 255.0f;
        std::vector<float> t(128);
        for (int mag = 0; mag < 128; mag++) t[mag] = (powf(1.0f + MU, (float)mag / 127.0f) - 1.0f) / MU;
        return t;
    }();
    for (int i = 0; i < n; i++) {
        const float v = in[i];
        const int c = (v == v && std::fabs(v) < 2.0e9f) ? (int)v : 0;   // NaN/huge: no UB, code 0
        float ax = table[c & 127];
        out[i] = ((c >> 7) & 1) == 1 ? -ax : ax;
    }
    return 0;
}

// Linear fade in/out (in samples), per channel.
extern "C" int nsk_audio_fade(Scope_Struct *scope_struct, float *data, int C, int T,
                              int fade_in, int fade_out) {
    // Same curves as torchaudio.transforms.Fade (linear): the first fade_in gains are
    // linspace(0, 1, fade_in), the last fade_out are linspace(1, 0, fade_out); overlapping
    // fades multiply.
    if (!data || C < 0 || T < 0 || fade_in < 0 || fade_out < 0) return 1;
    for (int c = 0; c < C; c++)
        for (int t = 0; t < T; t++) {
            float g = 1.0f;
            if (t < fade_in) g = fade_in == 1 ? 0.0f : (float)t / (float)(fade_in - 1);
            const int k = t - (T - fade_out);   // position inside the fade-out
            if (k >= 0) g *= fade_out == 1 ? 1.0f : 1.0f - (float)k / (float)(fade_out - 1);
            data[size_t(c) * T + t] *= g;
        }
    return 0;
}

// gain_type: 0 = amplitude, 1 = dB, 2 = power (like torchaudio.transforms.Vol).
extern "C" int nsk_audio_vol(Scope_Struct *scope_struct, float *data, int n, float gain, int gain_type) {
    if (!data || n < 0 || !std::isfinite(gain) || gain_type < 0 || gain_type > 2 ||
        (gain_type == 2 && gain < 0.0f)) return 1;
    const float g = gain_type == 1 ? powf(10.0f, gain / 20.0f) : gain_type == 2 ? sqrtf(gain) : gain;
    for (int i = 0; i < n; i++) data[i] *= g;
    return 0;
}

// Time masking: zeroes columns [t0, t0+width) of spec [bins x frames].
extern "C" int nsk_audio_time_mask(Scope_Struct *scope_struct, float *spec, int bins, int frames,
                                   int width, int t0) {
    if (!spec || bins < 0 || frames < 0 || width < 0) return 1;
    width = std::min(width, frames);
    if (t0 < 0) t0 = frames - width;
    t0 = std::min(t0, frames);
    for (int b = 0; b < bins; b++)
        for (int t = t0; t < t0 + width && t < frames; t++) spec[b*frames + t] = 0.0f;
    return 0;
}

// Frequency masking: zeroes rows [f0, f0+width).
extern "C" int nsk_audio_freq_mask(Scope_Struct *scope_struct, float *spec, int bins, int frames,
                                   int width, int f0) {
    if (!spec || bins < 0 || frames < 0 || width < 0) return 1;
    width = std::min(width, bins);
    if (f0 < 0) f0 = bins - width;
    f0 = std::min(f0, bins);
    for (int m = f0; m < f0 + width && m < bins; m++)
        for (int t = 0; t < frames; t++) spec[m*frames + t] = 0.0f;
    return 0;
}

// Time stretch: phase vocoder (same algorithm as librosa.phase_vocoder /
// torchaudio TimeStretch): n_fft=1024, hop=256, symmetric Hann, fixed hop,
// analysis frames at t*rate with magnitude interpolation, per-bin phase
// accumulator kept in [-pi, pi], overlap-add normalized by the window sum of
// squares. out has T_out ~ T / rate samples.
extern "C" int nsk_audio_time_stretch(Scope_Struct *scope_struct, float *wf, int T,
                                      float rate, float *out, int T_out) {
    if (!wf || !out || T <= 1 || !std::isfinite(rate) || rate <= 0.0f || T_out < 0) return 1;
    const int n_fft = 1024, hop = 256, M = n_fft / 2, bins = M + 1;
    const int frames_a = 1 + T / hop;
    const double n_out_d = std::ceil(double(frames_a) / rate);   // checked before any int cast
    if (!(n_out_d >= 1.0) || (n_out_d - 1.0) * hop + n_fft > double(INT_MAX)) return 1;
    const int n_out = int(n_out_d);
    auto P = stft_plan(n_fft, n_fft);
    const float PI = 3.14159265358979f;
    std::vector<float> frame(n_fft), zr(M), zi(M), y(n_fft), Xr(bins), Xi(bins);
    // Analysis frames are consumed in order (i0 = floor(t*rate) never decreases), so only the
    // pair (i0, i0+1) is kept, as magnitude and phase. Frames past the end are zero, like the
    // two frames of padding librosa adds.
    std::vector<float> magA(bins), phA(bins), magB(bins), phB(bins);
    auto analyze = [&](int t, float *mag, float *ph) {
        if (t >= frames_a) {
            std::fill(mag, mag + bins, 0.0f);
            std::fill(ph, ph + bins, 0.0f);
            return;
        }
        const float *x = frame_ptr(wf, T, n_fft, hop, t, frame.data());
        P->fft.pack(x, P->win.data(), zr.data(), zi.data());
        P->fft.c.run(zr.data(), zi.data());
        P->fft.post(zr.data(), zi.data(), Xr.data(), Xi.data());
#pragma omp simd
        for (int b = 0; b < bins; b++) {
            mag[b] = sqrtf(Xr[b] * Xr[b] + Xi[b] * Xi[b]);
            ph[b] = fast_atan2(Xi[b], Xr[b]);
        }
    };
    int k0 = 0;   // analysis frame held in the A slot (B holds k0 + 1)
    analyze(0, magA.data(), phA.data());
    analyze(1, magB.data(), phB.data());
    std::vector<float> adv(bins), acc(bins), Yr(bins), Yi(bins);
    for (int b = 0; b < bins; b++) {
        adv[b] = PI * float(hop) * float(b) / float(bins - 1);   // expected phase advance per hop
        acc[b] = phA[b];
    }
    const int full = n_fft + hop * (n_out - 1);                  // istft buffer before center trim
    std::vector<float> ybuf(full, 0.0f), wss(full, 0.0f);
    for (int t = 0; t < n_out; t++) {
        const double step = double(t) * rate;
        const int i0 = int(step);
        const float alpha = float(step - i0);
        if (i0 == k0 + 1) {
            std::swap(magA, magB);
            std::swap(phA, phB);
            analyze(i0 + 1, magB.data(), phB.data());
        } else if (i0 != k0) {
            analyze(i0, magA.data(), phA.data());
            analyze(i0 + 1, magB.data(), phB.data());
        }
        k0 = i0;
        const float *m0 = magA.data(), *p0 = phA.data(), *m1 = magB.data(), *p1 = phB.data();
#pragma omp simd
        for (int b = 0; b < bins; b++) {
            float mag = (1.0f - alpha) * m0[b] + alpha * m1[b];
            float s, c;
            fast_sincos(acc[b], &s, &c);
            Yr[b] = mag * c;
            Yi[b] = mag * s;
            float dphase = wrap_pi(p1[b] - p0[b] - adv[b]);
            acc[b] = wrap_pi(acc[b] + adv[b] + dphase);
        }
        Yi[0] = 0.0f; Yi[M] = 0.0f;
        P->fft.inverse(Yr.data(), Yi.data(), y.data(), zr.data(), zi.data());
        const float *w = P->win.data();
        float *dst = &ybuf[size_t(t) * hop], *ws = &wss[size_t(t) * hop];
        for (int i = 0; i < n_fft; i++) { dst[i] += y[i] * w[i]; ws[i] += w[i] * w[i]; }
    }
    for (int i = 0; i < T_out; i++) {
        int64_t k = int64_t(i) + n_fft / 2;
        out[i] = (k < full && wss[k] > 1e-10f) ? ybuf[k] / wss[k] : 0.0f;
    }
    return 0;
}

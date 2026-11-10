// Edge-case stress for the lib/audio kernels: odd sizes, tiny signals, extreme rates, chunk boundaries.
// Build and run under ASAN + UBSAN from the repository root:
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -fopenmp-simd \
//       -fno-math-errno -fno-trapping-math -mavx -DNEVE_AUDIO_STANDALONE \
//       audio_tests/stress_kernels.cpp lib/audio/src/audio.cpp -o /tmp/stress_kernels && /tmp/stress_kernels
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <random>
#include <string>
struct Scope_Struct; struct DT_str { char *str; int size; };
#define F(name, ...) extern "C" int nsk_audio_##name(Scope_Struct *, __VA_ARGS__);
F(spectrogram, float *, int, int, int, int, float, float *, int, int)
F(stft, float *, int, int, int, int, float *, float *, int, int)
F(melspectrogram, float *, int, int, int, int, int, float, float, float *, int, int)
F(mfcc, float *, int, int, int, int, int, int, float *, int)
F(resample_sinc, float *, int, int, int, int, float *, int)
F(time_stretch, float *, int, float, float *, int)
F(mulaw_encode, float *, float *, int)
F(mulaw_decode, float *, float *, int)
F(fft, float *, float *, int, int)
F(save_wav, DT_str, float *, int, int, int, DT_str)
F(read_decode, DT_str, float *, int, int, long, int, int)
F(wav_info_c, DT_str, int *)
extern "C" void *nsk_audio_fopen(Scope_Struct *, DT_str, DT_str);
extern "C" long nsk_audio_fread(Scope_Struct *, DT_str, long, long, void *);
extern "C" int nsk_audio_fclose(Scope_Struct *, void *);
int main() {
    std::mt19937 g(1);
    std::normal_distribution<float> nd(0, 0.3f);
    int calls = 0;
    for (int T : {1, 2, 3, 5, 63, 64, 65, 255, 1000, 4097, 20000}) {
        std::vector<float> y(T);
        for (auto &v : y) v = nd(g);
        for (int n_fft : {2, 4, 8, 16, 64, 256, 1024, 4096}) for (int hop : {1, 3, 64, 256, 5000}) for (int win : {1, 2, n_fft / 2 + 1, n_fft}) {
            if (win <= 0 || win > n_fft) continue;
            int bins = n_fft / 2 + 1, frames = 1 + T / hop;
            if ((long)bins * frames > 3000000) continue;
            std::vector<float> a((size_t)bins * frames), b(a.size());
            nsk_audio_spectrogram(nullptr, y.data(), T, n_fft, hop, win, 2.0f, a.data(), bins, frames);
            nsk_audio_stft(nullptr, y.data(), T, n_fft, hop, win, a.data(), b.data(), bins, frames);
            calls += 2;
        }
        for (int n_fft : {8, 256, 1024}) {
            int hop = 100, bins = n_fft / 2 + 1, frames = 1 + T / hop;
            std::vector<float> m(size_t(40) * frames), f(size_t(13) * frames);
            nsk_audio_melspectrogram(nullptr, y.data(), T, 16000, n_fft, hop, 40, 0, 0, m.data(), bins, frames);
            nsk_audio_mfcc(nullptr, y.data(), T, 16000, 13, n_fft, hop, 40, f.data(), frames);
            calls += 2;
        }
        for (float rate : {0.01f, 0.1f, 0.5f, 1.0f, 1.5f, 3.9f, 4.1f, 17.0f, 1e4f}) {
            if (T <= 1) continue;
            long To = std::lround(T / rate); if (To > 5000000) continue;
            std::vector<float> o(To + 8);
            int rc = nsk_audio_time_stretch(nullptr, y.data(), T, rate, o.data(), (int)To);
            for (int i = 0; i < To; i++) if (!std::isfinite(o[i])) { printf("non-finite stretch T=%d rate=%g\n", T, rate); return 1; }
            calls++; (void)rc;
        }
        for (auto [os, ns] : std::vector<std::pair<int,int>>{{16000,8000},{16000,16000},{8000,48000},{44100,16000},{44101,16001},{48000,44100},{1,3},{3,1},{65537,2}}) {
            for (int C : {1, 3}) {
                long To = ((long)T * ns + os - 1) / os; if (To > 3000000) continue;
                std::vector<float> in((size_t)C * T), o((size_t)C * To + 8);
                for (auto &v : in) v = nd(g);
                nsk_audio_resample_sinc(nullptr, in.data(), C, T, os, ns, o.data(), (int)To);
                calls++;
            }
        }
        std::vector<float> e(T), d(T);
        nsk_audio_mulaw_encode(nullptr, y.data(), e.data(), T);
        nsk_audio_mulaw_decode(nullptr, e.data(), d.data(), T);
    }
    for (int n : {1, 2, 4, 1024}) { std::vector<float> r(n, 1.f), i(n, 0.f); nsk_audio_fft(nullptr, r.data(), i.data(), n, 0); nsk_audio_fft(nullptr, r.data(), i.data(), n, 1); }
    // wav round trip with odd chunk boundaries
    for (int C : {1, 2, 3}) for (int T : {0, 1, 16383, 16384, 16385, 40000}) for (const char *fm : {"s16", "f32"}) {
        std::vector<float> v((size_t)C * T + 1);
        for (auto &x : v) x = nd(g);
        char path[] = "/tmp/stress_rt.wav"; DT_str p{path, (int)strlen(path)}; DT_str f{(char *)fm, 3};
        if (nsk_audio_save_wav(nullptr, p, v.data(), C, T, 8000, f) != 0) { printf("save failed\n"); return 1; }
        int info[8];
        if (T > 0) {
            if (nsk_audio_wav_info_c(nullptr, p, info)) { printf("info failed C=%d T=%d\n", C, T); return 1; }
            std::vector<float> back((size_t)C * T);
            if (nsk_audio_read_decode(nullptr, p, back.data(), C, T, info[7], info[3], info[2])) { printf("read failed\n"); return 1; }
            if (fm[0] == 'f') for (size_t i = 0; i < back.size(); i++) if (back[i] != v[i]) { printf("rt mismatch C=%d T=%d\n", C, T); return 1; }
        }
    }
    // memory-safety regressions: none of these may write out of bounds or hit UB
    {
        std::vector<float> big(20000);
        char path[] = "/tmp/stress_rt.wav"; DT_str p{path, (int)strlen(path)};
        if (nsk_audio_read_decode(nullptr, p, big.data(), 20000, 1, 0, 3, 32) == 0) { printf("read_decode accepted 20000 ch\n"); return 1; }
        std::string longp(5000, 'x'); DT_str lp{&longp[0], (int)longp.size()}; DT_str f{(char *)"s16", 3};
        if (nsk_audio_save_wav(nullptr, lp, big.data(), 1, 4, 8000, f) == 0) { printf("save accepted a 5000-byte path\n"); return 1; }
        char small[16]; DT_str dst{small, 16};
        void *fh = nsk_audio_fopen(nullptr, p, DT_str{(char *)"rb", 2});
        long got = nsk_audio_fread(nullptr, dst, 1, 4096, fh);
        nsk_audio_fclose(nullptr, fh);
        if (got > 16) { printf("fread past dst\n"); return 1; }
        float codes[3] = {NAN, INFINITY, 3e9f}, dec[3];
        nsk_audio_mulaw_decode(nullptr, codes, dec, 3);
        std::vector<float> y(4000, 0.1f), o(16);
        if (nsk_audio_time_stretch(nullptr, y.data(), 4000, 1e-30f, o.data(), 16) == 0) { printf("tiny rate accepted\n"); return 1; }
    }
    printf("stress ok, %d calls\n", calls);
}

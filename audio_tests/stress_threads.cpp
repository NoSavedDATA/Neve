// Concurrent calls into the cached plans (STFT, mel, resample, time stretch, fft). Build with
//   (TSAN needs a fixed address layout on some kernels: run it as `setarch $(uname -m) -R /tmp/stress_threads`)
//   g++ -std=c++17 -O1 -g -fsanitize=thread -fopenmp-simd -mavx -DNEVE_AUDIO_STANDALONE \
//       audio_tests/stress_threads.cpp lib/audio/src/audio.cpp -o /tmp/stress_threads -lpthread && /tmp/stress_threads
#include <cstdio>
#include <thread>
#include <vector>
#include <random>
struct Scope_Struct;
extern "C" int nsk_audio_spectrogram(Scope_Struct *, float *, int, int, int, int, float, float *, int, int);
extern "C" int nsk_audio_melspectrogram(Scope_Struct *, float *, int, int, int, int, int, float, float, float *, int, int);
extern "C" int nsk_audio_resample_sinc(Scope_Struct *, float *, int, int, int, int, float *, int);
extern "C" int nsk_audio_time_stretch(Scope_Struct *, float *, int, float, float *, int);
extern "C" int nsk_audio_fft(Scope_Struct *, float *, float *, int, int);
int main() {
    std::vector<float> y(30000); std::mt19937 g(2); std::normal_distribution<float> nd(0, .3f); for (auto &v : y) v = nd(g);
    std::vector<std::thread> th;
    for (int k = 0; k < 8; k++) th.emplace_back([&, k] {
        for (int it = 0; it < 40; it++) {
            int n_fft = 64 << ((k + it) % 5), hop = 100 + 7 * k, bins = n_fft / 2 + 1, frames = 1 + 30000 / hop;
            std::vector<float> a((size_t)bins * frames), m((size_t)20 * frames);
            nsk_audio_spectrogram(nullptr, y.data(), 30000, n_fft, hop, n_fft, 2.0f, a.data(), bins, frames);
            nsk_audio_melspectrogram(nullptr, y.data(), 30000, 16000 + 1000 * (it % 40), n_fft, hop, 20, 0, 0, m.data(), bins, frames);
            int nsr = 8000 + 100 * ((k * 40 + it) % 50), To = (int)(((long)30000 * nsr + 15999) / 16000);
            std::vector<float> o(To); nsk_audio_resample_sinc(nullptr, y.data(), 1, 30000, 16000, nsr, o.data(), To);
            std::vector<float> s(20000); nsk_audio_time_stretch(nullptr, y.data(), 30000, 1.5f, s.data(), 20000);
            std::vector<float> r(n_fft, 1.f), i(n_fft, 0.f); nsk_audio_fft(nullptr, r.data(), i.data(), n_fft, 0);
        }
    });
    for (auto &t : th) t.join();
    puts("tsan ok");
}

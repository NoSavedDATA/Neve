#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <sys/resource.h>

struct Scope_Struct;

namespace
{
    struct MemSnapshot
    {
        uint64_t rss_bytes = 0;
    };

    struct MonitorState
    {
        std::atomic<bool> running{false};

        std::atomic<uint64_t> peak_bytes{0};
        std::atomic<uint64_t> accumulated_bytes{0};
        std::atomic<uint64_t> samples{0};

        std::thread worker;
        std::mutex mutex;
    };

    MonitorState g_monitor;

    // Sampling interval
    constexpr std::chrono::milliseconds SAMPLE_INTERVAL(100);

    uint64_t get_rss_bytes()
    {
        std::ifstream statm("/proc/self/statm");
        if (!statm.is_open())
            return 0;

        uint64_t size_pages = 0;
        uint64_t resident_pages = 0;

        statm >> size_pages >> resident_pages;

        long page_size = sysconf(_SC_PAGESIZE);

        return resident_pages * static_cast<uint64_t>(page_size);
    }

    void sampler_thread()
    {
        while (g_monitor.running.load(std::memory_order_acquire))
        {
            uint64_t rss = get_rss_bytes();

            g_monitor.accumulated_bytes.fetch_add(
                rss,
                std::memory_order_relaxed
            );

            g_monitor.samples.fetch_add(
                1,
                std::memory_order_relaxed
            );

            uint64_t current_peak =
                g_monitor.peak_bytes.load(std::memory_order_relaxed);

            while (rss > current_peak &&
                   !g_monitor.peak_bytes.compare_exchange_weak(
                       current_peak,
                       rss,
                       std::memory_order_relaxed))
            {
            }

            std::this_thread::sleep_for(SAMPLE_INTERVAL);
        }
    }

    double bytes_to_mb(uint64_t bytes)
    {
        return static_cast<double>(bytes) / (1024.0 * 1024.0);
    }
}

extern "C"
int memp_start(Scope_Struct *ctx)
{

    std::lock_guard<std::mutex> lock(g_monitor.mutex);

    if (g_monitor.running.load(std::memory_order_acquire))
        return 0;

    g_monitor.peak_bytes.store(0, std::memory_order_release);
    g_monitor.accumulated_bytes.store(0, std::memory_order_release);
    g_monitor.samples.store(0, std::memory_order_release);

    g_monitor.running.store(true, std::memory_order_release);

    g_monitor.worker = std::thread(sampler_thread);
    return 0;
}

extern "C"
int memp_end(Scope_Struct *ctx)
{
    {
        std::lock_guard<std::mutex> lock(g_monitor.mutex);

        if (!g_monitor.running.load(std::memory_order_acquire))
            return 0;

        g_monitor.running.store(false, std::memory_order_release);
    }

    if (g_monitor.worker.joinable())
        g_monitor.worker.join();

    // TRUE OS peak RSS
    rusage usage{};
    getrusage(RUSAGE_SELF, &usage);

    // Linux: ru_maxrss is KB
    uint64_t peak =
        static_cast<uint64_t>(usage.ru_maxrss) * 1024;

    uint64_t total =
        g_monitor.accumulated_bytes.load(std::memory_order_relaxed);

    uint64_t samples =
        g_monitor.samples.load(std::memory_order_relaxed);

    double average =
        samples == 0
            ? 0.0
            : static_cast<double>(total) /
              static_cast<double>(samples);

    std::cout
        << "Peak RSS (OS): "
        << bytes_to_mb(peak) << " MB\n"

        << "Average RSS (sampled): "
        << bytes_to_mb(static_cast<uint64_t>(average))
        << " MB\n";

    return 0;
}

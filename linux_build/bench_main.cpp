/****************************************************************************
 *  bench_main.cpp — Portable benchmark for np_alloc vs malloc
 ***/
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <iomanip>
#include <cstring>

#include "../src/np_common.h"
#include "../src/np_alloc.h"

using std::chrono::system_clock;

class stopwatch {
public:
    stopwatch() { start(); }
    std::chrono::duration<double> check() { return system_clock::now() - start_; }
    void start() { start_ = system_clock::now(); }
    system_clock::time_point start_;
};

struct malloc_API {
    inline static void* alloc(size_t sz) { return ::malloc(sz); }
    inline static void free(void* ptr) { ::free(ptr); }
};

struct np_alloc_API {
    inline static void* alloc(size_t sz) { return np_alloc(sz); }
    inline static void free(void* ptr) { np_free(ptr); }
};

template <typename API>
double run_bench(int thread_count, int iteration_count, int max_alloc, int min_size, int max_size) {
    std::vector<std::thread> threads(thread_count);

    stopwatch sw;
    for (auto& t : threads) {
        t = std::thread([&]() {
            std::mt19937 gen(std::random_device{}());
            std::uniform_int_distribution<int> size_dist(min_size, max_size);
            std::uniform_int_distribution<int> op_dist(0, 1);

            std::vector<void*> allocated;
            allocated.reserve(max_alloc);

            // Pre-fill some
            int prefill = gen() % max_alloc;
            for (int i = 0; i < prefill; ++i) {
                allocated.push_back(API::alloc(size_dist(gen)));
            }

            bool allocating = true;
            for (int i = 0; i < iteration_count; ++i) {
                if (allocating) {
                    if ((int)allocated.size() < max_alloc) {
                        allocated.push_back(API::alloc(size_dist(gen)));
                    } else {
                        allocating = false;
                    }
                } else {
                    if (allocated.size() > 0) {
                        API::free(allocated.back());
                        allocated.pop_back();
                    } else {
                        allocating = true;
                    }
                }
            }

            for (auto* p : allocated) API::free(p);
        });
    }

    for (auto& t : threads) t.join();
    return sw.check().count();
}

struct BenchResult {
    const char* name;
    double malloc_time;
    double np_time;
};

int main() {
    std::cout << "=== np_alloc Benchmark (macOS/ARM64) ===" << std::endl;
    std::cout << "Platform: ";
#ifdef __aarch64__
    std::cout << "ARM64";
#elif defined(__x86_64__)
    std::cout << "x86-64";
#else
    std::cout << "unknown";
#endif
    std::cout << std::endl;

    // Warm up global pool
    void* warmup = np_alloc(64);
    np_free(warmup);

    const int thread_count = 4;
    const int single_iter = 500000;
    const int multi_iter = 50000;
    const int max_alloc = 2000;

    struct TestDef {
        const char* name;
        int threads;
        int iter;
        int min_size;
        int max_size;
    };

    TestDef tests[] = {
        {"random_single  (100-7100B)", 1, single_iter, 100, 7100},
        {"random_multi   (100-7100B)", thread_count, multi_iter, 100, 7100},
        {"small_single   (50-300B)",   1, single_iter, 50, 300},
        {"small_multi    (50-300B)",   thread_count, multi_iter, 50, 300},
        {"big_single     (5000-7500B)",1, single_iter, 5000, 7500},
        {"big_multi      (5000-7500B)",thread_count, multi_iter, 5000, 7500},
    };

    std::cout << "\nConfig: single=" << single_iter << " iter, multi=" << thread_count
              << " threads × " << multi_iter << " iter, max_alloc=" << max_alloc << "\n" << std::endl;

    std::cout << std::left << std::setw(32) << "Test"
              << std::right << std::setw(12) << "malloc"
              << std::setw(12) << "np_alloc"
              << std::setw(10) << "speedup" << std::endl;
    std::cout << std::string(66, '-') << std::endl;

    for (auto& t : tests) {
        double malloc_t = run_bench<malloc_API>(t.threads, t.iter, max_alloc, t.min_size, t.max_size);
        double np_t = run_bench<np_alloc_API>(t.threads, t.iter, max_alloc, t.min_size, t.max_size);
        double speedup = malloc_t / np_t;

        std::cout << std::left << std::setw(32) << t.name
                  << std::right << std::fixed << std::setprecision(4)
                  << std::setw(10) << malloc_t << "s"
                  << std::setw(10) << np_t << "s"
                  << std::setw(8) << std::setprecision(2) << speedup << "x" << std::endl;
    }

    std::cout << std::endl;
    np_debug_print();

    return 0;
}

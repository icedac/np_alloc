/****************************************************************************
 *  bench.cpp — cross-platform np_alloc benchmark
 *
 *  Compares malloc vs np_alloc across single/multi-thread workloads.
 *  Build via CMake: cmake --build build --target np_bench
 ***/
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>
#include <iomanip>
#include <cstdio>
#include <cstdlib>

#include "np_alloc.h"

using Clock = std::chrono::high_resolution_clock;

// ── Allocator API wrappers ──────────────────────────────────────────────

struct malloc_API {
    static const char* name() { return "malloc"; }
    static void* alloc(size_t sz) { return ::malloc(sz); }
    static void  free(void* ptr)  { ::free(ptr); }
};

struct np_alloc_API {
    static const char* name() { return "np_alloc"; }
    static void* alloc(size_t sz) { return np_alloc(sz); }
    static void  free(void* ptr)  { np_free(ptr); }
};

// ── Single-thread benchmark ─────────────────────────────────────────────

template <typename API>
double run_single(int iterations, int max_alloc, int min_sz, int max_sz) {
    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(min_sz, max_sz);

    std::vector<void*> allocs;
    allocs.reserve(max_alloc);

    auto start = Clock::now();

    bool filling = true;
    for (int i = 0; i < iterations; ++i) {
        if (filling) {
            if ((int)allocs.size() < max_alloc) {
                allocs.push_back(API::alloc(dist(gen)));
            } else filling = false;
        } else {
            if (!allocs.empty()) {
                API::free(allocs.back());
                allocs.pop_back();
            } else filling = true;
        }
    }

    for (auto* p : allocs) API::free(p);

    auto end = Clock::now();
    return std::chrono::duration<double>(end - start).count();
}

// ── Multi-thread benchmark ──────────────────────────────────────────────

template <typename API>
double run_multi(int nthreads, int iterations, int max_alloc, int min_sz, int max_sz) {
    std::vector<std::thread> threads;
    auto start = Clock::now();

    for (int t = 0; t < nthreads; ++t) {
        threads.emplace_back([=]() {
            std::mt19937 gen(42 + t);
            std::uniform_int_distribution<int> dist(min_sz, max_sz);
            std::vector<void*> allocs;
            allocs.reserve(max_alloc);

            bool filling = true;
            for (int i = 0; i < iterations; ++i) {
                if (filling) {
                    if ((int)allocs.size() < max_alloc) {
                        allocs.push_back(API::alloc(dist(gen)));
                    } else filling = false;
                } else {
                    if (!allocs.empty()) {
                        API::free(allocs.back());
                        allocs.pop_back();
                    } else filling = true;
                }
            }

            for (auto* p : allocs) API::free(p);
        });
    }

    for (auto& t : threads) t.join();
    auto end = Clock::now();
    return std::chrono::duration<double>(end - start).count();
}

// ── Main ────────────────────────────────────────────────────────────────

int main() {
    printf("=== np_alloc Benchmark ===\n");

    // Warm up: ensure global pool is initialized
    void* w = np_alloc(64);
    if (!w) {
        fprintf(stderr, "FATAL: np_alloc(64) returned null\n");
        return 1;
    }
    np_free(w);

    const int S_ITER    = 500000;
    const int M_THREADS = 4;
    const int M_ITER    = 100000;
    const int MAX_ALLOC = 2000;

    struct TestCase {
        const char* name;
        int thr;
        int iter;
        int lo;
        int hi;
    };

    TestCase tests[] = {
        {"random_single  (100-7100)",  1,         S_ITER, 100,  7100},
        {"small_single   (50-300)",    1,         S_ITER, 50,   300},
        {"big_single     (5000-7500)", 1,         S_ITER, 5000, 7500},
        {"random_multi   (100-7100)",  M_THREADS, M_ITER, 100,  7100},
        {"small_multi    (50-300)",    M_THREADS, M_ITER, 50,   300},
        {"big_multi      (5000-7500)", M_THREADS, M_ITER, 5000, 7500},
    };

    printf("\n%-32s %10s %10s %8s\n", "Test", "malloc", "np_alloc", "speedup");
    printf("--------------------------------------------------------------\n");

    for (auto& t : tests) {
        double mt, nt;
        if (t.thr == 1) {
            mt = run_single<malloc_API>(t.iter, MAX_ALLOC, t.lo, t.hi);
            nt = run_single<np_alloc_API>(t.iter, MAX_ALLOC, t.lo, t.hi);
        } else {
            mt = run_multi<malloc_API>(t.thr, t.iter, MAX_ALLOC, t.lo, t.hi);
            nt = run_multi<np_alloc_API>(t.thr, t.iter, MAX_ALLOC, t.lo, t.hi);
        }
        printf("%-32s %8.4fs %8.4fs %6.2fx\n", t.name, mt, nt, mt / nt);
    }

    printf("\n");
    np_debug_print();
    return 0;
}

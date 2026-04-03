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
#include <csignal>
#include <cstdio>
#include <cstdlib>

#include "np_alloc.h"

using Clock = std::chrono::high_resolution_clock;

#ifdef _MSC_VER
#include <windows.h>
// Windows SEH: catch access violations that signal() cannot
static LONG WINAPI seh_handler(EXCEPTION_POINTERS* ep) {
    fprintf(stderr, "SEH exception 0x%08lX at address %p\n",
        ep->ExceptionRecord->ExceptionCode,
        ep->ExceptionRecord->ExceptionAddress);
    fflush(stderr);
    ExitProcess(2);
    return EXCEPTION_EXECUTE_HANDLER;
}
#else
#include <unistd.h>
#endif

static void signal_handler(int sig) {
    fprintf(stderr, "Signal %d received!\n", sig);
    fflush(stderr);
#ifdef _MSC_VER
    exit(sig);
#else
    _exit(sig);
#endif
}

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
                void* p = API::alloc(dist(gen));
                if (!p) {
                    fprintf(stderr, "FATAL: %s alloc returned null at iter %d\n", API::name(), i);
                    fflush(stderr);
                    exit(3);
                }
                allocs.push_back(p);
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
#ifdef _MSC_VER
    SetUnhandledExceptionFilter(seh_handler);
#endif
    signal(SIGSEGV, signal_handler);
#ifndef _MSC_VER
    signal(SIGBUS, signal_handler);
#endif

    fprintf(stderr, "[DEBUG] main() entered\n"); fflush(stderr);
    printf("=== np_alloc Benchmark ===\n");
    fflush(stdout);

    // Warm up: ensure global pool is initialized
    void* w = np_alloc(64);
    if (!w) {
        fprintf(stderr, "FATAL: np_alloc(64) returned null — global pool init failed\n");
        return 1;
    }
    np_free(w);
    printf("Warmup done.\n");
    fflush(stdout);

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
    fflush(stdout);

    for (auto& t : tests) {
        double mt, nt;
        fprintf(stderr, "[DEBUG] test='%s' threads=%d\n", t.name, t.thr); fflush(stderr);

        if (t.thr == 1) {
            fprintf(stderr, "[DEBUG]   malloc start\n"); fflush(stderr);
            mt = run_single<malloc_API>(t.iter, MAX_ALLOC, t.lo, t.hi);
            fprintf(stderr, "[DEBUG]   malloc done: %.4fs\n", mt); fflush(stderr);

            fprintf(stderr, "[DEBUG]   np_alloc start\n"); fflush(stderr);
            nt = run_single<np_alloc_API>(t.iter, MAX_ALLOC, t.lo, t.hi);
            fprintf(stderr, "[DEBUG]   np_alloc done: %.4fs\n", nt); fflush(stderr);
        } else {
            fprintf(stderr, "[DEBUG]   malloc multi start\n"); fflush(stderr);
            mt = run_multi<malloc_API>(t.thr, t.iter, MAX_ALLOC, t.lo, t.hi);
            fprintf(stderr, "[DEBUG]   malloc multi done: %.4fs\n", mt); fflush(stderr);

            fprintf(stderr, "[DEBUG]   np_alloc multi start\n"); fflush(stderr);
            nt = run_multi<np_alloc_API>(t.thr, t.iter, MAX_ALLOC, t.lo, t.hi);
            fprintf(stderr, "[DEBUG]   np_alloc multi done: %.4fs\n", nt); fflush(stderr);
        }
        printf("%-32s %8.4fs %8.4fs %6.2fx\n", t.name, mt, nt, mt / nt);
        fflush(stdout);
    }

    printf("\n");
    np_debug_print();
    fprintf(stderr, "[DEBUG] main() returning 0\n"); fflush(stderr);
    return 0;
}

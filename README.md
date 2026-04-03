[![CI](https://github.com/icedac/np_alloc/actions/workflows/ci.yml/badge.svg)](https://github.com/icedac/np_alloc/actions/workflows/ci.yml)
[![GitHub license](https://img.shields.io/github/license/icedac/potio.svg?style=flat-square)](./LICENSE)

# np_alloc

A high-performance, per-thread, lock-free memory allocator for C++17.

Written from scratch in 2009, inspired by ideas from jemalloc and tcmalloc. Designed to eliminate lock contention in multi-threaded, allocation-heavy workloads.

## Architecture

4-layer allocation pipeline:

```
np_alloc() → thread_local_pool (TLS)
               → tls_ps_pool (per-size, ~32 buckets)
                  → global_pool (lock-free Treiber stack)
                     → mmap / VirtualAlloc (4 GB reserved)
```

- **Thread-local pools** — each thread owns its pool; zero locks on the hot path
- **Per-size buckets** — O(1) size-class lookup via precomputed mapping table
- **Lock-free global pool** — ABA-safe Treiber stack with 128-bit CAS (`cmpxchg16b` / `ldxp/stxp`)
- **L1 cache-line alignment** — 64-byte aligned allocations for optimal cache behavior
- **Platform abstraction** — unified source via `#ifdef _MSC_VER` (Windows FLS / POSIX `pthread_key`)

## Build

Cross-platform CMake build. Tested on MSVC 2022, GCC 13+, and Apple Clang.

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/np_bench          # run benchmark
```

## API

```cpp
void*    np_alloc(size_t bytes);
void*    np_alloc(size_t bytes, const char file[], int line);
void     np_free(void* ptr);
void     np_debug_print();   // dump mmap stats to stdout
```

## Features

- Lock-free, per-size, per-thread memory pool
- Wait-free on the fast path; contention only when fetching from the global pool
- Debug mode with memory fence padding (`0xbeefbabe19771218`, `0x5ca1ab1e20180721`)
- Thread-exit cleanup via FLS (Windows) / `pthread_key` (POSIX)

## Trade-offs

| | |
|---|---|
| **+** | Zero-lock allocation — thread-local free lists serve most requests in O(1) |
| **+** | Cache-friendly — pre-allocated pages per thread improve spatial locality |
| **−** | Higher memory footprint — tunable via pool size constants |

## Benchmarks

### Linux x64 — GitHub Actions CI (GCC 13, Ubuntu, 2-core, 2026)

500K iterations single-thread / 100K iterations × 4 threads, 2000 max live allocations.

| Test | malloc | np_alloc | Speedup |
|------|--------|----------|---------|
| random_single (100–7100 B) | 0.0223 s | 0.0092 s | **2.4×** |
| small_single (50–300 B) | 0.0120 s | 0.0046 s | **2.6×** |
| big_single (5000–7500 B) | 0.3340 s | 0.0096 s | **34.8×** |
| random_multi (100–7100 B) | 0.0194 s | 0.0077 s | **2.5×** |
| small_multi (50–300 B) | 0.0058 s | 0.0023 s | **2.5×** |
| big_multi (5000–7500 B) | 0.4138 s | 0.0062 s | **66.5×** |

> `big_*` speedup is extreme because glibc `malloc` falls back to `mmap`/`munmap` per allocation for sizes > ~128 KB threshold, while np_alloc serves from pre-reserved pool.

### Windows x64 — GitHub Actions CI (MSVC 2022, Windows Server 2025, 4-core, 2026)

Same workload parameters as Linux.

| Test | malloc | np_alloc | Speedup |
|------|--------|----------|---------|
| random_single (100–7100 B) | 0.2397 s | 0.0105 s | **22.8×** |
| small_single (50–300 B) | 0.0161 s | 0.0058 s | **2.8×** |
| big_single (5000–7500 B) | 0.1507 s | 0.0108 s | **13.9×** |
| random_multi (100–7100 B) | 0.0203 s | 0.0135 s | **1.5×** |
| small_multi (50–300 B) | 0.0124 s | 0.0046 s | **2.7×** |
| big_multi (5000–7500 B) | 0.0453 s | 0.0103 s | **4.4×** |

> Windows `malloc` (UCRT heap) is significantly slower in single-threaded random/big workloads. Multi-thread gains are modest on CI runners due to limited vCPU count.

### Windows x64 — Xeon X5570, 8-core / 16-thread (2009, Original)

50 threads × 500K iterations, 10K max live allocations.

<details>
<summary>Raw output</summary>

```
thread [28b0]: global pool created. pool=[2042de1a4b0]
malloc {
         random_multithread (500000) - 5.11385s
         random_singlethread (5000000) - 1.43155s
         small_multithread (500000) - 0.733916s
         small_singlethread (5000000) - 0.281047s
         big_multithread (500000) - 3.08501s
         big_singlethread (5000000) - 1.76247s
}
np_alloc {
         random_multithread (500000) - 1.2951s
         random_singlethread (5000000) - 0.229591s
         small_multithread (500000) - 0.681932s
         small_singlethread (5000000) - 0.0977364s
         big_multithread (500000) - 0.826968s
         big_singlethread (5000000) - 0.232066s
}
```

</details>

| | random (50–7100 B) | small (50–300 B) | big (5000–7500 B) |
|---|---|---|---|
| **Multi-thread** | **3.95×** | **1.08×** | **3.73×** |
| **Single-thread** | **6.24×** | **2.88×** | **7.59×** |

## TODO

- **Global pool GC** — return idle thread-local chunks to the global pool for cross-thread reuse; saves memory at the cost of some cache locality.

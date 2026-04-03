[![GitHub license](https://img.shields.io/github/license/icedac/potio.svg?style=flat-square)](./LICENSE)

# np_alloc

A high-performance, per-thread, lock-free memory allocator for C++.

Originally written from scratch in 2009, inspired by ideas from commercial production code. Designed to eliminate lock contention in multi-threaded allocation-heavy workloads.

## Architecture

4-layer allocation pipeline:

```
np_alloc() → thread_local_pool (TLS)
               → tls_ps_pool (per-size, ~32 buckets)
                  → global_pool (lock-free Treiber stack)
                     → mmap / VirtualAlloc
```

- **Thread-local pools**: Each thread owns its own pool — no locks on the hot path
- **Per-size buckets**: O(1) size-class lookup via precomputed mapping table
- **Lock-free global pool**: ABA-safe Treiber stack with 128-bit CAS (`cmpxchg16b` / `ldxp/stxp`)
- **L1 cache line alignment**: 64-byte aligned allocations for optimal cache behavior

## Toolset

- C++14 / GSL / x64
- POSIX (Linux / macOS) support via `linux_build/`

## Features

- Lock-free, per-size, per-thread memory pool
- Wait-free on the fast path; only contends when fetching from the global pool
- Debug mode with memory fence padding (`0xbeefbabe19771218`, `0x5ca1ab1e20180721`)

## API

```cpp
void*    np_alloc(size_t bytes);
void*    np_alloc(size_t bytes, const char file[], int line);
void     np_free(void* ptr);
```

## Usage

- Unit tests: [`test/unittest/UnitTest.cpp`](https://github.com/icedac/np_alloc/blob/master/test/unittest/UnitTest.cpp)
- Core source: [`src/`](https://github.com/icedac/np_alloc/blob/master/src/)
- POSIX build: [`linux_build/`](https://github.com/icedac/np_alloc/tree/master/linux_build)

## Pros / Cons

| | |
|---|---|
| **+** | Zero-lock allocation — thread-local free lists serve most requests in O(1) |
| **+** | Improved cache locality — memory pages are pre-allocated per thread |
| **-** | Higher memory footprint — tunable per project via pool size constants |

## TODO

- **Global pool garbage collection**: Return unused thread-local chunks to the global pool. Saves memory at the cost of some cache locality due to cross-thread memory reuse.

## Benchmarks

### Windows x64 (Xeon X5570, 8-core / 16-thread, 2009)

Original benchmarks. Source: [`np_alloc_test/test.cpp`](https://github.com/icedac/np_alloc/blob/master/np_alloc_test/test.cpp)

Method: Allocate up to max count, deallocate all, repeat.

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

**Multi-thread** (50 threads, 500K iterations, 10K max allocations per thread):

| | random (50-7100B) | small (50-300B) | big (5000-7500B) |
|---|---|---|---|
| malloc | 5.114s | 0.734s | 3.085s |
| np_alloc | 1.295s | 0.682s | 0.827s |
| **speedup** | **3.95x** | **1.08x** | **3.73x** |

**Single-thread** (1 thread, 5M iterations, 10K max allocations):

| | random (50-7100B) | small (50-300B) | big (5000-7500B) |
|---|---|---|---|
| malloc | 1.432s | 0.281s | 1.763s |
| np_alloc | 0.230s | 0.098s | 0.232s |
| **speedup** | **6.24x** | **2.88x** | **7.59x** |

### macOS / ARM64 (Apple Silicon, 2026)

After bugfixes and POSIX port. Source: [`linux_build/bench_v2.cpp`](https://github.com/icedac/np_alloc/blob/fix/codex-review-bugfixes/linux_build/bench_v2.cpp)

- `clang++ -std=c++17 -O2` / macOS 15 / Apple Silicon (ARM64)
- 4 threads / 500K iterations / 10K max allocations per thread

| test | malloc | np_alloc | speedup |
|------|--------|----------|---------|
| random_single (100-7100B) | 0.015s | 0.004s | **3.6x** |
| random_multi (100-7100B) | 0.005s | 0.001s | **3.7x** |
| small_single (50-300B) | 0.009s | 0.003s | **2.7x** |
| small_multi (50-300B) | 0.004s | 0.001s | **8.4x** |
| big_single (5000-7500B) | 0.012s | 0.006s | **2.1x** |
| big_multi (5000-7500B) | 0.006s | 0.001s | **4.6x** |

**Key observations:**
- Single-thread: **2.1-3.6x** faster — TLS per-size pool + free list O(1) allocation outperforms malloc's general-purpose codepath
- Multi-thread: **3.7-8.4x** faster — zero lock contention, performance scales linearly with thread count
- `small_multi` at **8.4x** demonstrates the sweet spot where lock-free per-thread pools dominate


[![GitHub license](https://img.shields.io/github/license/icedac/potio.svg?style=flat-square)](./LICENSE)


# [WORK-IN-PROGRESS] np_alloc
- yet-another, per-thread memory alloc/pool for c++
- code from scratch, some idea from written commerical code by myself in 2009.
  
## toolset
- c++14/gsl/x64
- POSIX (Linux/macOS) support via `linux_build/`
 

## feature
- lock-free, per-size, per-thread memory pool
- wait-free most of times except fetching from global pool
  
## api
```
void*    np_alloc(size_t bytes);
void*    np_alloc(size_t bytes, const char file[], int line);
void     np_free(void * ptr);
```

## usage & test code
- see [unittest.cpp](https://github.com/icedac/np_alloc/blob/master/test/unittest/UnitTest.cpp)
- see core [source](https://github.com/icedac/np_alloc/blob/master/src/)
 
## pros / cons
- (+) no lock to get new memory so fast
- (+) improved cache locality due to pre-allocated by memory page for local thread
- (-) using lots of memory but can be optimizing per project
 

## todo
### garbage collecting process from local thread pool to global pool
- it will saves memory but worse cache localtity due to spreading out memory fragments across threads

## benchmark
- just for initial lame benchmarks. code are [here](https://github.com/icedac/np_alloc/blob/master/np_alloc_test/test.cpp)
- methods: allocate up to max allocate count and deallocate until all freed and repeat this

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
thread [3a84]: global pool destroying. pool=[2042de1a4b0]
[3a84] ~global_pool() this=[2042de1a4b0]
Press any key to continue . . .
```
- windows 10 x64 / xeon x5570 2 processor / 8 core 16 thread
- env|50 threads|500k iteration per thread|10000 max alloction per thread
 
_ |random alloc(50-7100) | small(50-300) | big(5000-7500)
---|---|---|---
malloc|5.1139 s|0.7339 s|3.0850 s
np_alloc|1.2951 s|0.6819 s|0.8270 s
faster x|3.95 |1.08|3.73

- env|1 thread|5000k iteration per thread|10000 max alloction per thread

_ | random alloc(50-7100) | small(50-300) | big(5000-7500)
---|---|---|---
malloc | 1.4316 s| 0.2810 s| 1.7625 s
np_alloc | 0.2296 s | 0.0977 s| 0.2321 s
faster x|6.24|2.88|7.59

### macOS/ARM64 (Apple Silicon M-series, 2026)

After bugfixes and POSIX port. Benchmark code: [`linux_build/bench_v2.cpp`](https://github.com/icedac/np_alloc/blob/fix/codex-review-bugfixes/linux_build/bench_v2.cpp)

- clang++ -std=c++17 -O2 / macOS 15 / Apple Silicon (ARM64)
- 4 threads / 500k iterations per thread / 10000 max allocation per thread

| test | malloc | np_alloc | speedup |
|------|--------|----------|---------|
| random_single (100-7100B) | 0.015s | 0.004s | **3.6x** |
| random_multi (100-7100B) | 0.005s | 0.001s | **3.7x** |
| small_single (50-300B) | 0.009s | 0.003s | **2.7x** |
| small_multi (50-300B) | 0.004s | 0.001s | **8.4x** |
| big_single (5000-7500B) | 0.012s | 0.006s | **2.1x** |
| big_multi (5000-7500B) | 0.006s | 0.001s | **4.6x** |

Key observations:
- Single-thread: **2.1~3.6x** faster — TLS per-size pool + free list O(1) alloc beats malloc's general-purpose path
- Multi-thread: **3.7~8.4x** faster — zero lock contention scales linearly with thread count
- small_multi **8.4x** shows the sweet spot where lock-free per-thread pools dominate


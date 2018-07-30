/****************************************************************************
 *
 *  test.cpp
 *      ($C:\_repo\np_alloc\np_alloc_test)
 *
 *  by icedac 
 *     (2018/7/22)
 *
 ***/
#include "stdafx.h"
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

// #define HAS_JE_MALLOC


#ifdef HAS_JE_MALLOC
#include <jemalloc/jemalloc.h>
#endif

#include "../src/np_common.h"
#include "../src/np_alloc.h"

using std::chrono::system_clock;

class stopwatch {
public:
    stopwatch() {
        start();
    }

    system_clock::duration check() {
        return system_clock::now() - start_;
    }

    void start() {
        start_ = system_clock::now();
    }


public:
    system_clock::time_point start_;
};

np::uint64 bench_instrinc() {
    using namespace np;

#ifdef _DEBUG
    const auto kTestCount = 5000000;
#else
    const auto kTestCount = 500000000;
#endif

    uint64 result = 1;

    std::chrono::duration<double> diff;
    stopwatch sw;

    for (auto i = 0; i < kTestCount; ++i) {
        result += np::popcnt((np::uint64)i);
    }

    diff = sw.check();
    std::cout << "popcnt() * " << kTestCount << " = " << diff.count() << "s\n";

    sw.start();
    for (auto i = 0; i < kTestCount; ++i) {
        result += np::clz((np::uint64)i); // use bsr
    }
    diff = sw.check();
    std::cout << "clz() * " << kTestCount << " = " << diff.count() << "s\n";

    return result;
}

struct bench_summary {
	typedef std::chrono::duration<double> diff_t;



	np::string	name;

	enum {
		OP_RANODM_MULTI,
		OP_RANDOM_SINGLE,
		OP_SMALL_MULTI,
		OP_SMALL_SINGLE,
		OP_BIG_MULTI,
		OP_BIG_SINGLE,
		OP_MAX_SIZE
	};

	double		diff[OP_MAX_SIZE];
};

class determined_rand {
private:
	static const auto kPrecalculatedRandCount = 100000;
	np::uint32	precalculated_rand_[kPrecalculatedRandCount];
	mutable np::uint32	rand_index_ = 0;

public:
	determined_rand() {
		std::random_device rd;  //Will be used to obtain a seed for the random number engine
		std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
		std::uniform_int_distribution<np::uint32> dis( 0, UINT32_MAX);

		for (auto i = 0; i < kPrecalculatedRandCount; ++i)
			precalculated_rand_[i] = dis(gen);
	}

	inline np::uint32 rand() const {
		return precalculated_rand_[(rand_index_++%kPrecalculatedRandCount)];
	}

	np::uint32 operator () () const {
		return rand();
	}

	inline void reset() const {
		rand_index_ = 0;
	}
};


template < typename API, int MaxAllocCount = 1000 >
class alloc_bench {
private:
	std::vector< std::thread > ts_;
	const determined_rand& rnd_;

public:
	alloc_bench( const int max_thread_count, const determined_rand& rnd ) : rnd_(rnd) 
	{
		ts_.reserve(max_thread_count);
	}


	double random_alloc( const int thread_count, const int iteration_count ) {
		using namespace np;
		ts_.resize(thread_count);

		enum e_op {
			OP_FREE,
			OP_ALLOC
		};

		stopwatch sw;

		for (auto& t : ts_) {
			t = std::thread([&]() {
				vector< void* > allocated;
				allocated.reserve(MaxAllocCount);

				auto ss = rnd_() % MaxAllocCount;
				for (uint32 i = 0; i < ss; ++i)
				{
					auto alloc_size = 100 + rnd_() % 7000; // 100 ~ 7100
					allocated.push_back(API::alloc(alloc_size));
				}

				e_op op = OP_ALLOC;
				for (auto i = 0; i < iteration_count; ++i) {
					switch (op) {
					case OP_ALLOC:
						if (allocated.size() < MaxAllocCount) {
							auto alloc_size = 100 + rnd_() % 8000; // 100 ~ 7100
							allocated.push_back(API::alloc(alloc_size));
						}
						else
							op = OP_FREE;
						break;
					case OP_FREE:
						if (allocated.size() > 0) {
							auto* ptr = allocated.back();
							allocated.pop_back();
							API::free(ptr);
						}
						else
							op = OP_ALLOC;
						break;
					}
				}

				for (auto* ptr : allocated) API::free(ptr);
				allocated.clear();
			});
		}

		for (auto& t : ts_) {
			t.join();
		}


		std::chrono::duration<double> diff = sw.check();
		return diff.count();
	}

	template < typename GetAllocFun >
	double t_alloc(GetAllocFun get_alloc_fun, const int thread_count, const int iteration_count) {
		using namespace np;
		ts_.resize(thread_count);

		enum e_op {
			OP_FREE,
			OP_ALLOC
		};

		stopwatch sw;

		for (auto& t : ts_) {
			t = std::thread([&]() {
				vector< void* > allocated;
				allocated.reserve(MaxAllocCount);

				auto ss = rnd_() % MaxAllocCount;
				for (uint32 i = 0; i < ss; ++i)
				{
					auto alloc_size = get_alloc_fun();
					allocated.push_back(API::alloc(alloc_size));
				}

				e_op op = OP_ALLOC;
				for (auto i = 0; i < iteration_count; ++i) {
					switch (op) {
					case OP_ALLOC:
						if (allocated.size() < MaxAllocCount) {
							auto alloc_size = get_alloc_fun();
							allocated.push_back(API::alloc(alloc_size));
						}
						else
							op = OP_FREE;
						break;
					case OP_FREE:
						if (allocated.size() > 0) {
							auto* ptr = allocated.back();
							allocated.pop_back();
							API::free(ptr);
						}
						else
							op = OP_ALLOC;
						break;
					}
				}

				for (auto* ptr : allocated) API::free(ptr);
				allocated.clear();
			});
		}

		for (auto& t : ts_) {
			t.join();
		}

		std::chrono::duration<double> diff = sw.check();
		return diff.count();
	}

	double small_alloc(const int thread_count, const int iteration_count) {
		return t_alloc([&]() {
			// return 100 + rnd_() % 8000; // 100 ~ 7100 bytes
			return 50 + rnd_() % 250; // 50~300 bytes
		}, thread_count, iteration_count);
	}
	double big_alloc(const int thread_count, const int iteration_count) {
		return t_alloc([&]() {
			return 5000 + rnd_() % 2500; // 5000 ~ 7500 bytes
		}, thread_count, iteration_count);
	}
};

struct malloc_API {
	inline static void* alloc(size_t sz) {
		return ::malloc(sz);
	}
	inline static void free(void* ptr) {
		::free(ptr);
	}
};

struct np_alloc_API {
	inline static void* alloc(size_t sz) {
		return np_alloc(sz);
	}
	inline static void free(void* ptr) {
		np_free(ptr);
	}
};

#ifdef HAS_JE_MALLOC
struct je_malloc_API {
	inline static void* alloc(size_t sz) {
		return je_malloc(sz);
	}
	inline static void free(void* ptr) {
		je_free(ptr);
	}
}
#endif

struct test_defs {
	np::string	name;
	np::uint32	n;
	np::uint32	iteration_count;
};


void alloc_bench_test() {

#ifdef _DEBUG
	constexpr const int thread_count = 50;
	constexpr const int iteration_count = 1000;
	constexpr const int max_alloc_count = 1000;
	constexpr const int precalculate_count = 100000;
#else
	constexpr const int thread_count = 50;
	constexpr const int iteration_count = 500000;
	constexpr const int max_alloc_count = 10000;
	constexpr const int precalculate_count = 1000000;
#endif

	determined_rand drand;
	alloc_bench< malloc_API, max_alloc_count> malloc_bench(thread_count,drand);
	alloc_bench< np_alloc_API, max_alloc_count> np_alloc_bench(thread_count, drand);
#ifdef HAS_JE_MALLOC
	alloc_bench< je_malloc_API, max_alloc_count> je_alloc_bench(thread_count, drand);
#endif

	enum {
		MALLOC_OP,
		NP_ALLOC_OP,
		JE_MALLOC_OP,
	};

	std::vector< bench_summary > benchs = 
	{
		{ "malloc", {} },
		{ "np_alloc",  {} },
		{ "je_malloc", {} },
	};

	std::vector< test_defs > tests = {
		{ "random_multithread", bench_summary::OP_RANODM_MULTI, iteration_count },
		{ "random_singlethread", bench_summary::OP_RANDOM_SINGLE, iteration_count*10 },
		{ "small_multithread", bench_summary::OP_SMALL_MULTI, iteration_count },
		{ "small_singlethread", bench_summary::OP_SMALL_SINGLE, iteration_count * 10 },
		{ "big_multithread", bench_summary::OP_BIG_MULTI, iteration_count },
		{ "big_singlethread", bench_summary::OP_BIG_SINGLE, iteration_count * 10 },
	};


	benchs[MALLOC_OP].diff[bench_summary::OP_RANODM_MULTI] = malloc_bench.random_alloc(thread_count, iteration_count);
	benchs[MALLOC_OP].diff[bench_summary::OP_RANDOM_SINGLE] = malloc_bench.random_alloc(1, iteration_count*10);
	benchs[MALLOC_OP].diff[bench_summary::OP_SMALL_MULTI] = malloc_bench.small_alloc(thread_count, iteration_count);
	benchs[MALLOC_OP].diff[bench_summary::OP_SMALL_SINGLE] = malloc_bench.small_alloc(1, iteration_count * 10);
	benchs[MALLOC_OP].diff[bench_summary::OP_BIG_MULTI] = malloc_bench.big_alloc(thread_count, iteration_count);
	benchs[MALLOC_OP].diff[bench_summary::OP_BIG_SINGLE] = malloc_bench.big_alloc(1, iteration_count * 10);

	benchs[NP_ALLOC_OP].diff[bench_summary::OP_RANODM_MULTI] = np_alloc_bench.random_alloc(thread_count, iteration_count);
	benchs[NP_ALLOC_OP].diff[bench_summary::OP_RANDOM_SINGLE] = np_alloc_bench.random_alloc(1, iteration_count*10);
	benchs[NP_ALLOC_OP].diff[bench_summary::OP_SMALL_MULTI] = np_alloc_bench.small_alloc(thread_count, iteration_count);
	benchs[NP_ALLOC_OP].diff[bench_summary::OP_SMALL_SINGLE] = np_alloc_bench.small_alloc(1, iteration_count * 10);
	benchs[NP_ALLOC_OP].diff[bench_summary::OP_BIG_MULTI] = np_alloc_bench.big_alloc(thread_count, iteration_count);
	benchs[NP_ALLOC_OP].diff[bench_summary::OP_BIG_SINGLE] = np_alloc_bench.big_alloc(1, iteration_count * 10);
#ifdef HAS_JE_MALLOC
	benchs[NP_ALLOC_OP].diff[bench_summary::OP_RANODM] = je_alloc_bench.random_alloc(thread_count, iteration_count);
#endif

	auto print_result = [&]() {
		for (const auto& b : benchs) {
			std::cout << b.name << " {\n";
			for (const auto& t : tests) {
				std::cout << "\t " << t.name << " (" << std::dec << t.iteration_count << ") - " << b.diff[t.n] << "s\n";
			}
			std::cout << "}\n";
		}
	};

	print_result();
}

void mutex_test();

void general_test() {
    // bench_instrinc();
	// mutex_test();
	alloc_bench_test();
}

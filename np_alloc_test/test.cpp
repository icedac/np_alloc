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

#include "../src/np_common.h"

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

void general_test() {
    bench_instrinc();
}

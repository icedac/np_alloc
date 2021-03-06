/****************************************************************************
 *
 *  main.cpp
 *      ($\nativecoin-cpp\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"
#include "platform.h"
#include <string>
#include <thread>
#include <atomic>

#include "local_new.h"
#include "np_thread.h"
#include "np_alloc.h"

#pragma message( "          -----------------------" )
#pragma message( "          [ compiler: " __COMPILER_NAME )
#ifdef USE_CPP_17
#pragma message( "          [ c++std: C++17      ]" )
#else
#pragma message( "          [ c++std: C++14      ]" )
#endif
#pragma message( "          -----------------------" )
 

class test_c {
public:
    test_c(const std::string& s1, std::uint64_t s2, double s3) : s1_(s1), s2_(s2), s3_(s3) {}

public:
    std::string     s1_;
    std::uint64_t   s2_;
    double          s3_;
};

std::atomic<int> deleted_count(0);

void _test() {

    std::thread t[100];
    for (auto i = 0; i < 100; ++i) {
        t[i] = std::thread([]() {
            test_c* a = np::internal::t_new<test_c>("a", 100, 12.34);
            // internal::t_delete(a);

            np::thread_atexit([a]() {
                deleted_count += 1;
                np::internal::t_delete(a);
            });
        });
    }

    for (auto i = 0; i < 100; ++i)
        t[i].join();

    printf("deleted_count[%d]\n", deleted_count.load() );

    test_c* a = np::internal::t_new<test_c>("a", 100, 12.34);
    // internal::t_delete(a);

    np::thread_atexit([a]() { 
        printf("internal::t_delete called.\n");
        np::internal::t_delete(a);
    });

    if (!np::thread_test_atexit()) {
        printf("thread_test_atexit() - test failed.\n");
    }

    void* p = np_alloc( 50, __FILE__, __LINE__ );
    ::memset(p, 0x33, 50);
    np_free(p);
}

#ifndef _LIB
int main()
{
    _test();
    return 0;
}
#endif

/****************************************************************************
 *  linux_main.cpp — Linux-compatible benchmark runner
 *  Replaces np_alloc_test/main.cpp (Windows-specific)
 ***/
#include <iostream>

extern void general_test();

int main() {
    std::cout << "=== np_alloc Linux Benchmark ===" << std::endl;
    general_test();
    return 0;
}

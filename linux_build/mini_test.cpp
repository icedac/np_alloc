#include <iostream>
#include <vector>
#include <random>
#include "../src/np_common.h"
#include "../src/np_alloc.h"

int main() {
    std::cout << "Mini allocation test..." << std::endl;
    void* warmup = np_alloc(64);
    np_free(warmup);

    std::mt19937 gen(42);
    std::uniform_int_distribution<int> dist(100, 7100);

    std::vector<void*> ptrs;
    ptrs.reserve(1000);

    // Phase 1: allocate 1000
    std::cout << "Allocating 1000..." << std::endl;
    for (int i = 0; i < 1000; ++i) {
        void* p = np_alloc(dist(gen));
        if (!p) { std::cout << "ALLOC FAILED at " << i << std::endl; return 1; }
        ptrs.push_back(p);
    }
    std::cout << "OK. Freeing..." << std::endl;

    for (auto* p : ptrs) np_free(p);
    ptrs.clear();
    std::cout << "OK." << std::endl;

    // Phase 2: bigger test
    std::cout << "Allocating 10000..." << std::endl;
    for (int i = 0; i < 10000; ++i) {
        void* p = np_alloc(dist(gen));
        if (!p) { std::cout << "ALLOC FAILED at " << i << std::endl; return 1; }
        ptrs.push_back(p);
        if (i % 2 == 0 && ptrs.size() > 1) {
            np_free(ptrs.back());
            ptrs.pop_back();
        }
    }
    std::cout << "Freeing " << ptrs.size() << "..." << std::endl;
    for (auto* p : ptrs) np_free(p);
    std::cout << "All done." << std::endl;

    np_debug_print();
    return 0;
}

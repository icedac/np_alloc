#include <iostream>
#include "../src/np_common.h"
#include "../src/np_alloc.h"

int main() {
    std::cout << "Simple np_alloc test..." << std::endl;

    void* p = np_alloc(100);
    std::cout << "Allocated: " << p << std::endl;

    if (p) {
        np_free(p);
        std::cout << "Freed OK" << std::endl;
    }

    std::cout << "Done." << std::endl;
    return 0;
}

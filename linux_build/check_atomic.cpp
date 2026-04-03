#include <atomic>
#include <cstdio>
#include <cstdint>

struct head_t {
    uintptr_t aba = 0;
    void* ptr = nullptr;
};

int main() {
    std::atomic<head_t> h;
    printf("sizeof(head_t) = %zu\n", sizeof(head_t));
    printf("sizeof(atomic<head_t>) = %zu\n", sizeof(h));
    printf("is_lock_free = %d\n", h.is_lock_free());
    printf("is_always_lock_free = %d\n", std::atomic<head_t>::is_always_lock_free);
    return 0;
}

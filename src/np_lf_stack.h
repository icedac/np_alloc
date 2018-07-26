#pragma once
/****************************************************************************
 *
 *  np_lf_stack.h
 *      ($\np_alloc\src)
 *
 *
 *  simple lock-free stack (ABA free)
 *
 *  by icedac 
 *
 ***/
#ifndef ___NP_ALLOC__NP_ALLOC__NP_LF_STACK_H_
#define ___NP_ALLOC__NP_ALLOC__NP_LF_STACK_H_

#include <atomic>
#include <functional>

namespace np {

    template < typename T >
    const T* get_next( const T* t) {
        return typename t->next;
    }
    template < typename T >
    void set_next(T* t, T* next) {
        typename t->next = next;
    }

    template < typename T >
    struct tagged_ptr {
        T* ptr;
        operator T* () const {
            return ptr & 0xffffffffffffffff;
        }
    };

    template < typename T >
    // requires ( get_next<T>() && set_next<T>()
    class lf_stack {
    public:
        struct head_t {
            uintptr_t   aba = 0;
            T*          ptr = nullptr;
        };

        T* pop() {
            head_t next, curr = head_.load(std::memory_order_relaxed);

            do {
                if (!curr.ptr) break;
                next.aba = curr.aba + 1;
                next.ptr = curr.ptr->next;

            } while (!head_.compare_exchange_weak( curr, next, std::memory_order_release));

            return curr.ptr;
        }
        void push(T* ptr) {
            head_t next, curr = head_.load(std::memory_order_relaxed);
            do {
                ptr->next = curr.ptr;
                next.aba = curr.aba + 1;
                next.ptr = ptr;
            } while ( !head_.compare_exchange_weak( curr, next, std::memory_order_release) );
        }

        void for_each(std::function< void(const T*) > fun) {
            head_t next, curr = head_.load(std::memory_order_relaxed);

            do {
                if (!curr.ptr) break;
                next.aba = curr.aba + 1;
                next.ptr = nullptr;

            } while (!head_.compare_exchange_weak(curr, next, std::memory_order_release));

            // now we acquired full stack
            T* ptr = curr.ptr;
            while (ptr) {
                fun(ptr);
                T* next = ptr->next;
                push(ptr);
                ptr = next;
            }
        }

        uint64 count_free_list() {
            uint64 c = 0;
            for_each([&](const T*) {
                c += 1;
            });
            return c;
        }

        uint64 debug_free_list() {
            uint64 c = 0;
            for_each([&](const T* ptr) {
                std::cout << "\t[" << c << "] ";
                std::cout << "head: " << std::hex << std::setw(10) << (uint64)ptr << ", next: " << std::hex << std::setw(10) << (uint64)ptr->next << "]\n";
                c += 1;
            });
            return c;
        }

    private:
        std::atomic<head_t> head_;
    };

}

#endif// ___NP_ALLOC__NP_ALLOC__NP_LF_STACK_H_


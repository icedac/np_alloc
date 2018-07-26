#pragma once
/****************************************************************************
 *
 *  np_mmap.h
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#ifndef _____NP_ALLOC__NP_MMAP_H_
#define _____NP_ALLOC__NP_MMAP_H_

#include <atomic>
#include <iomanip>

#include "np_common.h"
#include "np_bitset.h"
#include "np_lf_stack.h"

namespace np {

    constexpr static const uint64 kDefaultChunkSize = 1_MB;
    constexpr static const uint64 kDefaultChunkCount = 64 * 64; // 4096
    constexpr static const uint64 kDefaultWholeChunkSize = 256_GB;

    class mmap {
    public:
        uint64 reserve(uint64 size = kDefaultWholeChunkSize);

    public:
        struct chunk_header {
            chunk_header* next = nullptr;
            uint64 index = 0;
        };

        mmap() : chunk_size_(kDefaultChunkSize) {
            init();
        }
        ~mmap();

        void init();

        inline uint64 get_chunk_size() const { return chunk_size_; }

        inline uint64 get_chunk_count() const { return kDefaultChunkCount; }

        inline void* alloc_chunk() {
            // get if has free_list
            auto* ptr = free_head_.pop();
            if (ptr) {
                auto index = get_chunk_index(ptr);
                // alloc_chunk_index_.set(index); // allocated
                perf_alloc_from_free.fetch_add(1);
                // printf("mmap::allocate from free_list; ptr[%I64x] index[%I64d] size[%I64d]\n", (uint64)head, index, get_chunk_size());
                return ptr;
            }

            // otherwise allocate new and return it
            return alloc_chunk_from_new();
        }

        void* alloc_chunk_from_new();

        inline void free_chunk(void* ptr) {
            chunk_header* new_head = reinterpret_cast<chunk_header*>(ptr);
            free_head_.push(new_head);

            // assert(alloc_chunk_index_.get(index) == true);
            // alloc_chunk_index_.clear(index); // deallocated

            perf_dealloc.fetch_add(1);
            // printf("mmap::deallocate ptr[%I64x] index[%I64d] head->next[%I64x]\n", (uint64)new_head, index, (uint64)new_head->next );
        }

        inline uint64 get_chunk_index(void* ptr) const {
            void* begin_ptr = ptr_.load();
            assert(ptr);
            void* end_ptr = (void*)(((byte*)begin_ptr) + size_);
            auto pos = (uint64)ptr - (uint64)begin_ptr;

            if ((pos % chunk_size_)) {
                return -1;
            }

            auto chunk_index = pos / chunk_size_;
            assert(chunk_index <= get_chunk_count());
            return chunk_index;
        }

        inline void* get_chunk_ptr(uint64 chunk_index) const {
            assert(chunk_index <= get_chunk_count());
            void* base_ptr = ptr_.load();
            assert(base_ptr);
            void* ptr = (void*)(((byte*)base_ptr) + chunk_index * get_chunk_size());
            return ptr;
        }

        inline bool validate_ptr(void* ptr) const {
            void* begin_ptr = ptr_.load();
            assert(ptr);
            void* end_ptr = (void*)(((byte*)begin_ptr) + size_);
            if ((uint64)ptr < (uint64)begin_ptr) return false;
            if ((uint64)ptr >= (uint64)end_ptr) return false;
            return true;
        }

        inline uint64 get_total_allocated() const {
            return perf_alloc.load();
        }

        // unsafe
        inline uint64 count_free_list() /*const*/ {
            return free_head_.count_free_list();
        }

        inline uint64 debug_free_list() /*const*/ {
            return free_head_.debug_free_list();
        }

        string debug_as_string() /*const*/;

    private:
        std::atomic<void*>              ptr_ = nullptr;
        np::bitset<kDefaultChunkCount>  alloc_chunk_index_;

        std::atomic<void*>              head_ = nullptr; // allocated from here

        // lock-free stack
        lf_stack<chunk_header>          free_head_;

        std::atomic<uint64>             perf_alloc = 0;
        std::atomic<uint64>             perf_dealloc = 0;
        std::atomic<uint64>             perf_alloc_from_free = 0;

        uint64              size_ = 0;
        uint64              base_page_size_ = 0;
        const uint64        chunk_size_ = 0;
    };
}

#endif// _____NP_ALLOC__NP_MMAP_H_


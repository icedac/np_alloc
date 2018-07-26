#pragma once
/****************************************************************************
 *
 *  global_pool.h
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#ifndef _____NP_ALLOC__GLOBAL_POOL_H_
#define _____NP_ALLOC__GLOBAL_POOL_H_

#include "np_common.h"
#include "np_mmap.h"

namespace np {

    class global_pool {
    public:
        inline global_pool() {
            mm_.reserve();
        }

        ~global_pool();

        inline uint64 get_chunk_size() const { return mm_.get_chunk_size(); }

        inline void* alloc_chunk() { return mm_.alloc_chunk(); }

        inline void free_chunk(void* ptr) { mm_.free_chunk(ptr); }

        inline string debug_as_string() /*const*/ { return mm_.debug_as_string(); }

    private:
        np::mmap    mm_;
    };
}

#endif// _____NP_ALLOC__GLOBAL_POOL_H_

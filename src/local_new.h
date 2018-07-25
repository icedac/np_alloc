#pragma once
/****************************************************************************
 *
 *  local_new.h
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 *
 *  new/delete wrapper to not interfere with global new/delete overloading
 *
 *
 ***/
#ifndef _____NP_ALLOC__LOCAL_NEW_H_
#define _____NP_ALLOC__LOCAL_NEW_H_

#include <malloc.h>

namespace np {

    namespace internal {
        template < typename T, typename... Targs >
        inline T* t_new(const Targs&... args) {
            T* t = reinterpret_cast<T*>(malloc(sizeof(T)));
            new (t) T(args...);
            return t;
        }

        template < typename T >
        inline void t_delete(T* t) {
            t->~T();
            free(t);
        }

        template < typename T, typename... Targs >
        inline T* t_aligned_new(size_t aligned_size, const Targs&... args) {
            T* t = reinterpret_cast<T*>(_aligned_malloc(sizeof(T), aligned_size));
            new (t) T(args...);
            return t;
        }

        template < typename T >
        inline void t_aligned_delete(T* t) {
            t->~T();
            free(t);
        }
    }

}

#endif// _____NP_ALLOC__LOCAL_NEW_H_


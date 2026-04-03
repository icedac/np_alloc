#pragma once
/****************************************************************************
 *
 *  np_thread.h
 *      ($\np_alloc\src)
 *
 *  by icedac
 *
 ***/
#ifndef _____NP_ALLOC__NP_THREAD_H_
#define _____NP_ALLOC__NP_THREAD_H_

#include "config.h"
#include <functional>

namespace np {

    /****************************************************************************
     *  thread_atexit() — register per-thread cleanup callbacks
     *
     *  Windows: implemented via FlsAlloc() (Fiber Local Storage)
     *    https://learn.microsoft.com/en-us/windows/win32/api/fibersapi/nf-fibersapi-flsalloc
     *  POSIX:   implemented via pthread_key_create() destructor
     */
    typedef std::function< void(void) > fn_atexit_callback;
    NP_API void thread_atexit(fn_atexit_callback);

    NP_API bool thread_test_atexit();
}

#endif// _____NP_ALLOC__NP_THREAD_H_

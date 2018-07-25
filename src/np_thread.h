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
     * 	thread_atexit() - implemented with FlsAlloc()
     *
     *      FlsAlloc() https://msdn.microsoft.com/ko-kr/dc348ef3-37e5-40f2-bd5c-5f8aebc7cc59
     */
    typedef std::function< void(void) > fn_atexit_callback;
    NP_API void thread_atexit(fn_atexit_callback);

    NP_API bool thread_test_atexit();
}

#endif// _____NP_ALLOC__NP_THREAD_H_


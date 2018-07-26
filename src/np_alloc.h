#pragma once
/****************************************************************************
 *
 *  np_alloc.h
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#ifndef _____NP_ALLOC__NP_ALLOC_H_
#define _____NP_ALLOC__NP_ALLOC_H_

#include "platform.h"
#include "config.h"
#include "np_common.h"

NP_API void*    np_alloc(size_t bytes);
NP_API void*    np_alloc(size_t bytes, const char file[], int line);
NP_API void     np_free(void * ptr);

NP_API void     np_debug_print();

#endif// _____NP_ALLOC__NP_ALLOC_H_


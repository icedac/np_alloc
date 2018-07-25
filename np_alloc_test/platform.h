#pragma once
/****************************************************************************
 *
 *	platform.h
 *		($C:\_repo\nativecoin-cpp\src)
 *
 *	by icedac (2018/7/16)
 *
 ***/
#ifndef ___NP_ALLOC_TEST__NP_ALLOC_TEST__PLATFORM_H_
#define ___NP_ALLOC_TEST__NP_ALLOC_TEST__PLATFORM_H_

#if defined(_MSC_VER)

	#define COMPILER_MSVC
	#define COMPILER_VERSION	_MSC_VER

	#if COMPILER_VERSION >= 1914
		#define COMPILER_MSVC_VERSION	1414 // vc++2017; MVVC++ 14.1
		#define __COMPILER_NAME			"MSVC++ 14.14 (VS2017 15.7)"
		#define USE_CPP_17
	#elif COMPILER_VERSION >= 1910
		#define COMPILER_MSVC_VERSION	1410 // vc++2017; MVVC++ 14.1
		#define __COMPILER_NAME			"MSVC++ 14.1 (VS2017)"
		#define USE_CPP_17
	#elif COMPILER_VERSION >= 1900
		#define COMPILER_MSVC_VERSION	1400 // vc++2015
		#define __COMPILER_NAME			"MSVC++ 14.0 (VS2015)"
	#else
		#define COMPILER_MSVC_VERSION	1399 // prev
		#define __COMPILER_NAME			"ver < MSVC++ 14.0 (VS2015)"
	#endif

#else
	#define __COMPILER_NAME			"unknown compiler"

#endif

#endif// ___NP_ALLOC_TEST__NP_ALLOC_TEST__PLATFORM_H_


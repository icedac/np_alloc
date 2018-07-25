#pragma once
/****************************************************************************
 *
 *  np_common.h
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#ifndef _____NP_ALLOC__NP_COMMON_H_
#define _____NP_ALLOC__NP_COMMON_H_

#include <vector>
#include <array>
#include <stdint.h>
#include <string.h>
#include <sstream>

#ifdef _MSC_VER
#include <intrin.h>
#include <minwindef.h>
#endif 

namespace np {
    typedef std::uint8_t byte;
    typedef std::uint32_t uint32;
    typedef std::int32_t int32;
    typedef std::uint64_t uint64;
    typedef std::int64_t int64;
    typedef std::string string;
    typedef std::istringstream istringstream;
    typedef std::ostringstream ostringstream;
    typedef std::stringstream stringstream;

    template < typename T >
    using vector = std::vector<T>;
    template < typename T, size_t SZ >
    using array = std::array<T,SZ>;


    /****************************************************************************
    * number litteral
    */
    constexpr uint64 operator "" _MB(uint64 n) {
        return (n * 1024LL * 1024LL);
    }
    constexpr uint64 operator "" _GB(uint64 n) {
        return (n * 1024LL * 1024LL * 1024LL);
    }

    /****************************************************************************
     * 	
     */
#if defined(__GNUC__)
    inline uint32 clz(uint32 x) { if (x) return __builtin_clz(x); return sizeof(x) * 8; }
    inline uint32 ctz(uint32 x) { if (x) return __builtin_ctz(x); return sizeof(x) * 8; }
    inline uint64 clz(uint64 x) { if (x) return __builtin_clzll(x); return sizeof(x) * 8; }
    inline uint64 ctz(uint64 x) { if (x) return __builtin_ctzll(x); return sizeof(x) * 8; }
    inline uint32 popcnt(uint32 x) { return __builtin_popcount(x); }
    inline uint32 popcnt(uint64 x) { return __builtin_popcountll(x); }
#elif defined(_MSC_VER)
    inline uint32 clz(uint32 x) { // count leading zero
        DWORD lz = 0; 
        if (_BitScanReverse(&lz, x)) return (sizeof(x) * 8 - 1) - lz; 
        return sizeof(x) * 8;
    }
    inline uint64 clz(uint64 x) {
        DWORD lz = 0; 
        if (_BitScanReverse64(&lz, x)) return (sizeof(x) * 8 - 1) - lz; 
        return sizeof(x) * 8;
    }
    inline uint32 ctz(uint32 x) { // count trailing zero
        DWORD tz = 0;
        if (_BitScanForward(&tz, x)) return tz;
        return sizeof(x) * 8;
    }
    inline uint64 ctz(uint64 x) {
        DWORD tz = 0; 
        if (_BitScanForward64(&tz, x)) return tz; 
        return sizeof(x) * 8;
    }
    inline uint32 popcnt(uint32 x) { // count all zero
        return __popcnt(x);
    }
    inline uint64 popcnt(uint64 x) {
        return __popcnt64(x);
    }
#else
#error UNKNOWN COMPILER
#endif

    inline auto log2(uint64 x) {
        return sizeof(x) * 8 - clz(x) - 1;
    }
}


#endif// _____NP_ALLOC__NP_COMMON_H_


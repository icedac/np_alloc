#pragma once
/****************************************************************************
 *
 *	util.h
 *		($\nativecoin-cpp\src)
 *
 *	by icedac
 *
 ***/
#ifndef ___NP_ALLOC__NP_ALLOC__UTIL_H_
#define ___NP_ALLOC__NP_ALLOC__UTIL_H_

#include "nc.h"
#include <chrono>

namespace nc {
    namespace util {

        bool is_hexstring(gsl::span< const char > bin);

        string binary_to_hexstring(gsl::span< const byte > bin);

        vector<byte> hexstring_to_binary(gsl::span< const char > bin);

        // tab to every line
        string tab_strings(string s);

        timestamp_t time_since_epoch();
    }
}

#endif// ___NP_ALLOC__NP_ALLOC__UTIL_H_


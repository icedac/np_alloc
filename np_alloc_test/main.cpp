/****************************************************************************
*
*  main.cpp
*      ($\nativecoin-cpp\src)
*
*  by icedac
*
***/
#include "stdafx.h"
#include "../src/np_alloc.h"
#include <bitset>
#include <iostream>

#pragma message( "          -----------------------" )
#pragma message( "          [ compiler: " __COMPILER_NAME )
#ifdef USE_CPP_17
#pragma message( "          [ c++std: C++17      ]" )
#else
#pragma message( "          [ c++std: C++14      ]" )
#endif
#pragma message( "          -----------------------" )

#define MB (1024LL*1024LL)
// #define LOG2(X) ((unsigned) (8*sizeof (unsigned long long) - __builtin_clzll((X)) - 1))

std::string GetLastErrorAsString()
{
    //Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0)
        return std::string(); //No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    //Free the buffer.
    LocalFree(messageBuffer);

    return message;
}

void check_address_space() {
    using namespace np;

    auto reserve_size = 1024_GB;
    LPVOID hint_address = (LPVOID)0x10000000000;
    for (auto i = 0; i < 0xf; ++i) {
        LPVOID address = (LPVOID)( ((uint64)hint_address)*(i + 1) );
        LPVOID v = VirtualAlloc( address, reserve_size, MEM_RESERVE, PAGE_READWRITE);
        auto lerr = ::GetLastError();
        auto lerr_str = GetLastErrorAsString();
        LPVOID v_end = (LPVOID)(((np::byte*)v) + reserve_size);

        if (v) {
            printf("addr[%I64x]; ok\n", (uint64)address );

            auto t = VirtualAlloc(address, 64_MB, MEM_COMMIT, PAGE_READWRITE);
            if (t) {
                if (!VirtualFree(t, 64_MB, MEM_DECOMMIT)) {
                    auto lerr_str = GetLastErrorAsString();
                    printf("addr[%I64x]; VirtualFree failed - %s", (uint64)address, lerr_str.c_str());
                }
            }
            else {
                lerr_str = GetLastErrorAsString();
                printf("addr[%I64x]; commit failed - %s\n", (uint64)address, lerr_str.c_str());
            }
        }
        else
        {
            printf("addr[%I64x]; failed - %s", (uint64)address, lerr_str.c_str());
        }

        if (v) {
            // dealloc
            if (!VirtualFree(v, 0, MEM_RELEASE)) {
                auto lerr_str = GetLastErrorAsString();
                printf("addr[%I64x]; VirtualFree failed - %s", (uint64)address, lerr_str.c_str());
            }
        }
    }
}

void __test() {
	using namespace np;

	SYSTEM_INFO si;
	::memset(&si, 0, sizeof(si));
	::GetSystemInfo(&si);

	si.dwPageSize;

	//auto reserve_size = 1024_GB;
	//// LPVOID hint_address = (LPVOID)0x60000000000;
	//LPVOID hint_address = (LPVOID)0x10000000000;
	//0x00000000000; // from
	//0x7FFFFFFFFFF; // to

	//LPVOID v = VirtualAlloc(hint_address, reserve_size, MEM_RESERVE, PAGE_READWRITE);
	//auto lerr = ::GetLastError();
	//auto lerr_str = GetLastErrorAsString();
	//LPVOID v_end = (LPVOID)(((byte*)v) + reserve_size);


	check_address_space();
	check_address_space();

	void* p = np_alloc(100);
	np_free(p);
}

int main()
{
	// __test();

    void general_test();
    general_test();
}


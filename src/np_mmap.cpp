/****************************************************************************
 *
 *  np_mmap.cpp
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"
#include "np_mmap.h"

namespace np {

    uint64 mmap::reserve(uint64 size /*= kDefaultWholeChunkSize*/)
    {
        LPVOID hint_address = (LPVOID)0x10000000000;

        for (auto i = 0; i < 0xf; ++i) {
            LPVOID address = (LPVOID)(((uint64)hint_address)*(i + 1));
            LPVOID v = VirtualAlloc(address, size, MEM_RESERVE, PAGE_READWRITE);
            if (v) {

                void* ptr_null = nullptr;

                if (ptr_.compare_exchange_strong(ptr_null, v)) {
                    size_ = size;
                    alloc_chunk_index_.clear_all();
                    head_.store(v);
                    return size;
                }

                VirtualFree(v, 0, MEM_RELEASE);
            }
        }

        return 0;
    }

    void mmap::init()
    {
        SYSTEM_INFO si;
        ::memset(&si, 0, sizeof(si));
        ::GetSystemInfo(&si);
        
        base_page_size_ = si.dwPageSize;
    }

    string mmap::debug_as_string() /*const*/
    {
        auto fn_padding_space = [](const std::string& s, size_t min_size) {
            if (s.size() >= min_size) return s;
            auto padding_size = min_size - s.size();
            assert(padding_size <= 64);
            return (s + (&"                                                                "[64 - padding_size]));
        };

        stringstream ss;
        ss << "mmap {\n";
        ss << "\t " << fn_padding_space("total_size", 20) << ": " << get_chunk_count() * get_chunk_size() << " bytes \n";
        ss << "\t " << fn_padding_space("chunk_size", 20) << ": " << get_chunk_size() << " bytes \n";
        // ss << "\t " << fn_padding_space("allocated", 20) << ": " << allocated() << "/" << get_chunk_count() << " units \n";
        ss << "\t " << fn_padding_space("perf_alloc", 20) << ": " << perf_alloc.load(std::memory_order_relaxed) << " times \n";
        ss << "\t " << fn_padding_space("perf_dealloc", 20) << ": " << perf_dealloc.load(std::memory_order_relaxed) << " times \n";
        ss << "\t " << fn_padding_space("perf_alloc_from_free", 20) << ": " << perf_alloc_from_free.load(std::memory_order_relaxed) << " times \n";
        ss << "\t " << fn_padding_space("__free_list_count", 20) << ": " << count_free_list() << " units \n";

        ss << "}\n";
        return ss.str();
    }

}

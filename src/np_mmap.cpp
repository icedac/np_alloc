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

#include <math.h>

#ifdef _MSC_VER

#else
extern "C" {
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
}
#endif

namespace np {
    namespace internal {
#ifdef _MSC_VER
        static void* virtual_reserve(void* hint_address, uint64 size) {
            return (void*)VirtualAlloc((LPVOID)hint_address, size, MEM_RESERVE, PAGE_READWRITE);
        }

        static bool virtual_release(void* ptr) {
            return TRUE == VirtualFree((LPVOID)ptr, 0, MEM_RELEASE);
        }

        static void* virtual_alloc(void* address, uint64 chunk_size) {
            return (void*)VirtualAlloc( (LPVOID)address, chunk_size, MEM_COMMIT, PAGE_READWRITE);
        }

        static bool virtual_free(void* address, uint64 chunk_size) {
            return TRUE == VirtualFree((LPVOID)address, chunk_size, MEM_DECOMMIT);
        }
#else
        static void* virtual_reserve(void* hint_address, uint64 size) {
            // return (void*)VirtualAlloc((LPVOID)hint_address, size, MEM_RESERVE, PAGE_READWRITE);
            return nullptr;
        }

        static bool virtual_release(void* ptr) {
            // return TRUE == VirtualFree((LPVOID)ptr, 0, MEM_RELEASE);
            return true;
        }

        static void* virtual_alloc(void* address, uint64 chunk_size) {
            // return (void*)VirtualAlloc( (LPVOID)address, chunk_size, MEM_COMMIT, PAGE_READWRITE);

# if defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
            void* ptr = ::mmap(address, chunk_size, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
# else
            const int fd(::open("/dev/zero", O_RDONLY));
            assert(-1 != fd);
            void* ptr = ::mmap(address, chunk_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
# endif
            return ptr;
        }

        static bool virtual_free(void* address, uint64 chunk_size) {
            // return TRUE == VirtualFree((LPVOID)address, chunk_size, MEM_DECOMMIT);
            ::munmap( address, chunk_size );
        }
#endif
    }

    void mmap::init()
    {
#if _MSC_VER
        SYSTEM_INFO si;
        ::memset(&si, 0, sizeof(si));
        ::GetSystemInfo(&si);

        base_page_size_ = si.dwPageSize;
#else
        base_page_size_ = ::getpagesize();
#endif
    }

    uint64 mmap::reserve(uint64 size /*= kDefaultWholeChunkSize*/)
    {
        void* hint_address = (void*)0x10000000000;

        for (auto i = 0; i < 0xf; ++i) {
            void* address = (void*)(((uint64)hint_address)*(i + 1));
            void* v = internal::virtual_reserve(address, size);
            if (v) {

                void* ptr_null = nullptr;

                if (ptr_.compare_exchange_strong(ptr_null, v)) {
                    size_ = size;
                    alloc_chunk_index_.clear_all();
                    head_.store(v);
                    return size;
                }

                internal::virtual_release(v);
            }
        }

        return 0;
    }

    mmap::~mmap()
    {
        void* v = ptr_.load();

        if (!internal::virtual_release(v)) {
            //auto lerr_str = GetLastErrorAsString();
            //printf("addr[%I64x]; VirtualFree failed - %s", (uint64)address, lerr_str.c_str());
        }
    }

    void* mmap::alloc_chunk_from_new()
    {
        void* head = head_.load(std::memory_order_relaxed);
        void* next_ptr = nullptr;

        do {
            if (!head) return nullptr;
            auto index = get_chunk_index(head);
            assert(index != -1);
            next_ptr = get_chunk_ptr(index + 1);

        } while (!head_.compare_exchange_weak(head, next_ptr, std::memory_order_release));

        head = internal::virtual_alloc( head, chunk_size_);
        if (!head) {
            assert(head);
        }

        auto index = get_chunk_index(head);
        // alloc_chunk_index_.set(index); // allocated
        perf_alloc.fetch_add(1);
        // printf("mmap::allocate ptr[%I64x] index[%I64d] size[%I64d]\n", (uint64)head, index, get_chunk_size());

        return head;
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

/****************************************************************************
 *
 *  np_alloc.cpp
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"

#include <thread>
#include <mutex>

#include "np_common.h"
#include "np_alloc.h"
#include "np_thread.h"
#include "global_pool.h"

#include "local_new.h"

// assert if freeing corrupted memory or wrong memory
#define NP_DEBUG_MEMORY_FENCE

//  intel                               
//  Skylake             | i7 6700   | 64 bytes  |
//  Haswell             | i7 4770   | 64 bytes  |
//  Sandy Bridge        | i3 2120   | 64 bytes  |
//  Bradwell(i7,xeon)   |           | 64 bytes  |
//  Coffee Lake(2017)   |           | 64 bytes  |
//
//  amd
//  zen1 14nm(2017)     |           | ?         |
//  zen+ 12nm(2018)     |           | 64 bytes  |
//
//  so almost modern mainstream cpu has 64 bytes of L1 Cache Line size

namespace np {

    constexpr bool      kUse32And48AllocSize = false;
    constexpr size_t    kDefaultAlignSize = 16;
    constexpr size_t    kL1CacheAlignSize = 64;
    constexpr size_t    kMaxBlockSize = 8192 + kL1CacheAlignSize - kDefaultAlignSize;

    static std::mutex   g_mtx_cout; // implemented in critical section


    global_pool::~global_pool() {
        std::cout << "[" << std::hex << std::this_thread::get_id() << "] fuck me\n";
    }

    // void * _aligned_malloc(size_t size, size_t alignment);
    namespace internal {

        static void del_glolbal_pool();

        static np::global_pool* get_global_pool() {
            static np::global_pool* pool = nullptr;
            static bool _init = [&]() {
                pool = new np::global_pool();

                std::atexit(del_glolbal_pool);

                return true;
            }();
            auto init = (_init == true);
            return pool;
        }

        static void del_glolbal_pool() {
            auto* p = get_global_pool();
            delete p;
        }
    }

    // tls per-size pool
    template < int BUCKET_SIZE >
    class tls_ps_pool {
    public:
        static const int kBucketSize = BUCKET_SIZE;

        struct free_header {
            free_header* next = nullptr;
        };

        tls_ps_pool(size_t alloc_size) : alloc_size_(alloc_size) {
            chunk_size_ = internal::get_global_pool()->get_chunk_size();
        }

        ~tls_ps_pool() {
            // free 
            deinit();
        }

        size_t get_alloc_size() const {
            return alloc_size_;
        }

        void* alloc(size_t bytes, const char file[], int line) {
            void* ptr = nullptr;
            // if has free_list, then return from it
            if (free_) {
                ptr = free_;
                free_ = free_->next;
            }
            else {
                // or alloc from local pool
                ptr = alloc_from_local_pool(bytes);
            }

            // null
            if (!ptr) return ptr;

            ++allocated_;
            return ptr;
        }

        void dealloc(void * ptr) {
            --allocated_;

            // add to free list
            assert(get_alloc_size() >= sizeof(free_header));
            free_header* fh = (free_header*)ptr;
            fh->next = free_;
            free_ = fh;

            // [todo] if has too many free then need to pass it to global pool for reuse
        }

        struct pool {
            pool* next = nullptr;
            void* pool_curr = nullptr;
        };

        void* get_alloc_end( void* ptr ) const {
            return (void*)(((byte*)(ptr)) + chunk_size_);
        }

        // return current pool free count
        uint64 get_free_count() const {
            void* end = get_alloc_end(head_);            
            auto free_size = ((int64)end - (int64)(head_->pool_curr));
            if (free_size <= 0) return 0;
            auto free_count = (uint64)free_size / get_alloc_size();
            return free_count;
        }

        void* alloc_from_local_pool(size_t bytes) {
            if ( !head_ || get_free_count() == 0 ) alloc_from_global();

            void* ptr = head_->pool_curr;
            head_->pool_curr = (void*) ((byte*)ptr + get_alloc_size());
            return ptr;
        }

        void alloc_from_global() {
            assert(internal::get_global_pool()->get_chunk_size() >= sizeof(pool));

            pool* new_pool = (pool*)internal::get_global_pool()->alloc_chunk();
            new_pool->pool_curr = (void*)(new_pool + 1);
            new_pool->next = head_;
            head_ = new_pool;
        }

        void deinit() {
            auto c = 0;
            free_header* f = free_;
            while (f) {
                ++c;
                f = f->next;
            }

            if ( 0 != allocated_) {
                std::lock_guard<std::mutex> lg(g_mtx_cout);
                std::cerr << "tls_ps_pool["<< alloc_size_ << "]::thread [" << std::hex << std::this_thread::get_id() << "] has allocated and not freed: " << allocated_ << " units. free_count: " << c << " units\n";
            }

            while (head_) {
                pool* p = head_;
                head_ = head_->next;
                internal::get_global_pool()->free_chunk(p);
            }
        }

    private:
        pool*           head_ = nullptr;
        free_header*    free_ = nullptr;

        const size_t    alloc_size_;
        size_t          chunk_size_;

        uint64          allocated_;
    };

    // alloc header
    struct alloc_header_t
    {
        std::uint32_t   size;
        std::uint16_t   line;
        std::uint16_t   thread_id;
        const char*	    file;
    };
    constexpr size_t calc_aligned_size(size_t actual_size, size_t aligned_size) {
        return ((actual_size / aligned_size) + (actual_size%aligned_size) == 0 ? 0 : 1) * aligned_size;
    }

    static const uint64 kMemoryFencePadding[] = {
        0xbeefbabe19771218,
        0x5ca1ab1e20180721,
        0xbeefbabe19771218,
        0x5ca1ab1e20180721,
    };

    class thread_local_pool {
    public:
        // per-thread singleton
        static thread_local_pool& get() {
            static thread_local thread_local_pool* s_pool = nullptr;
            if (!s_pool) {
                auto* pool = internal::t_new<thread_local_pool>();

                np::thread_atexit([pool]() {
                    internal::t_delete(s_pool);
                });

                s_pool = pool;
            }

            return *s_pool;
        }

        void* alloc(size_t bytes, const char file[], int line) {
            constexpr auto header_size = calc_aligned_size(sizeof(alloc_header_t), kDefaultAlignSize);
            auto alloc_size = bytes;
            alloc_size += header_size;
#ifdef NP_DEBUG_MEMORY_FENCE
            alloc_size += (16 * 2);
#endif
            void* p = nullptr;

            if (alloc_size <= max_alloc_size_)
                p = mapping_pool_[alloc_size]->alloc(alloc_size, file, line);
            else
            {
                // too big alloc size, just pass to malloc
                p = malloc(alloc_size);
            }

            alloc_header_t* h = reinterpret_cast<alloc_header_t*>(p);
            h->size = (std::uint32_t)bytes;
            h->line = (std::uint16_t)line;
            h->file = file;
            h->thread_id = (uint16_t) *((uint32*)(&std::this_thread::get_id())); // std::thread::id implemented as uint32

            p = reinterpret_cast<void*>(((byte*)p) + header_size);
#ifdef NP_DEBUG_MEMORY_FENCE
            uint64_t* pre_pad = (uint64_t*)p;
            uint64_t* post_pad = (uint64_t*)(((byte*)p) + 16 + h->size);
            pre_pad[0] = kMemoryFencePadding[0];
            pre_pad[1] = kMemoryFencePadding[1];
            post_pad[0] = kMemoryFencePadding[2];
            post_pad[1] = kMemoryFencePadding[3];
            p = reinterpret_cast<void*>(((byte*)p) + 16);
#endif
            return p;
        }

        void dealloc(void * ptr) {
            constexpr auto header_size = calc_aligned_size(sizeof(alloc_header_t), kDefaultAlignSize);
            alloc_header_t* h = reinterpret_cast<alloc_header_t*>( ((byte*)ptr) - header_size );
#ifdef NP_DEBUG_MEMORY_FENCE
            h = reinterpret_cast<alloc_header_t*>(((byte*)ptr) - header_size - 16 );
#endif

            size_t alloc_size = h->size + header_size;
#ifdef NP_DEBUG_MEMORY_FENCE
            alloc_size += (16 * 2);

            // check magic padding
            uint64_t* pre_pad = (uint64_t*)(((byte*)h) + header_size);
            uint64_t* post_pad = (uint64_t*)(((byte*)h) + header_size + 16 + h->size);
            
            assert(pre_pad[0] == kMemoryFencePadding[0]);
            assert(pre_pad[1] == kMemoryFencePadding[1]);
            assert(post_pad[0] == kMemoryFencePadding[2]);
            assert(post_pad[1] == kMemoryFencePadding[3]);
#endif

            if (alloc_size <= max_alloc_size_)
                mapping_pool_[alloc_size]->dealloc(h);
            else
            {
                free(h);                
            }
        }

        const alloc_header_t* get_alloc_header(void* ptr) const {
            constexpr auto header_size = calc_aligned_size(sizeof(alloc_header_t), kDefaultAlignSize);
            alloc_header_t* h = reinterpret_cast<alloc_header_t*>(((byte*)ptr) - header_size);
#ifdef NP_DEBUG_MEMORY_FENCE
            h = reinterpret_cast<alloc_header_t*>(((byte*)ptr) - header_size - 16);
#endif
            return h;
        }

        void init() {
            auto sizes = get_alloc_size_list();

            // initialzie tls_pool
            for (auto sz : sizes) {
                //printf("%d, ", sz);
                if (max_alloc_size_ < sz) max_alloc_size_ = sz;
                tls_pool pool(sz);
                tls_pool_.emplace_back(std::move(pool));
            }
            //printf("\n");

            // initialize table to tls_pool per alloc size
            // only sizeof(void*)*81xx bytes
            mapping_pool_ = (tls_pool**)np::aligned_alloc(kL1CacheAlignSize, sizeof(tls_pool*) * (max_alloc_size_ + 1) );
            size_t cur_size = 0;
            for (auto& pool : tls_pool_ ) {
                auto alloc_size = pool.get_alloc_size();
                while (alloc_size >= cur_size) {
                    mapping_pool_[cur_size++] = &pool;
                }
            }
            assert(cur_size == (max_alloc_size_ + 1));
        }

        ~thread_local_pool() {
            np::aligned_free(mapping_pool_);
        }

        static vector<size_t> get_alloc_size_list() {
            vector<size_t > sizes;
            if (kUse32And48AllocSize) {
                sizes.push_back(32);
                sizes.push_back(48);
            }
            for (auto i = 0; i < 4; ++i)
                sizes.push_back(kL1CacheAlignSize*(i + 1));
            for (auto i = 0; true; ++i) {
                for (auto j = 4; j < 8; ++j) {
                    size_t alloc_size = (j << i) * kL1CacheAlignSize + kL1CacheAlignSize;
                    sizes.push_back(alloc_size);
                    if (alloc_size >= kMaxBlockSize + kDefaultAlignSize) goto done;
                }
            }
        done:
            assert(sizes.size() <= 32); // "per-size allocator should be lower than 32");
            return sizes;
        }
    private:
        typedef np::tls_ps_pool<512> tls_pool;
        vector<tls_pool>    tls_pool_;
        tls_pool**          mapping_pool_ = nullptr;
        size_t              max_alloc_size_ = 0;

    private:
        template < typename T, typename... Targs >
        friend T* internal::t_new(const Targs&... args);
        template < typename T, typename... Targs >
        friend T* internal::t_aligned_new(size_t aligned_size, const Targs&... args);

        thread_local_pool() {
            init();
        }
    };
}

NP_API void* np_alloc(size_t bytes)
{
    return np_alloc(bytes, "no debug info", 0);
}

NP_API void* np_alloc(size_t bytes, const char file[], int line)
{
    return np::thread_local_pool::get().alloc(bytes, file, line);
}

NP_API void np_free(void * ptr)
{
    np::thread_local_pool::get().dealloc(ptr);
}

NP_API void np_debug_print()
{
    auto dbg_string = np::internal::get_global_pool()->debug_as_string();
    std::cout << dbg_string;
}

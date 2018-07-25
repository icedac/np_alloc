/****************************************************************************
 *
 *  np_alloc.cpp
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"

#include "np_alloc.h"
#include "global_pool.h"
#include "local_new.h"
#include "np_thread.h"
#include "global_pool.h"
#include <vector>

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

    // void * _aligned_malloc(size_t size, size_t alignment);
    namespace internal {
        static np::global_pool* get_global_pool() {
            static np::global_pool* pool = nullptr;
            static bool _init = [&]() {
                pool = new np::global_pool();
                return true;
            }();
            auto init = (_init == true);
            return pool;
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

        size_t get_alloc_size() const {
            return alloc_size_;
        }

        void* alloc(size_t bytes, const char file[], int line) {
            // if has free_list, then return from it
            if (free_) {
                auto* ptr = free_;
                free_ = free_->next;
                return ptr;
            }

            // or alloc from local pool
            return alloc_from_local_pool(bytes);
        }

        void dealloc(void * ptr) {
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

    private:
        pool*           head_ = nullptr;
        free_header*    free_ = nullptr;

        const size_t    alloc_size_;
        size_t          chunk_size_;
    };

    // alloc header
    struct alloc_header_t
    {
        std::uint32_t   size;
        std::uint32_t   line;
        const char*	    file;
    };
    constexpr size_t calc_aligned_size(size_t actual_size, size_t aligned_size) {
        return ((actual_size / aligned_size) + (actual_size%aligned_size) == 0 ? 0 : 1) * aligned_size;
    }

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
            auto alloc_size = bytes;
            auto header_size = calc_aligned_size(sizeof(alloc_header_t), kDefaultAlignSize);
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
            h->file = file;
            h->line = line;

            p = reinterpret_cast<void*>(((byte*)p) + header_size);
#ifdef NP_DEBUG_MEMORY_FENCE
            uint64_t* pre_pad = (uint64_t*)p;
            uint64_t* post_pad = (uint64_t*)(((byte*)p) + 16 + h->size);
            pre_pad[0] = 0xbeefbabe19771218;
            pre_pad[1] = 0x5ca1ab1e20180721;
            post_pad[0] = 0xbeefbabe19771218;
            post_pad[1] = 0x5ca1ab1e20180721;
            p = reinterpret_cast<void*>(((byte*)p) + 16);
#endif
            return p;
        }

        void dealloc(void * ptr) {
            auto header_size = calc_aligned_size(sizeof(alloc_header_t), kDefaultAlignSize);
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
            
            assert(pre_pad[0] == 0xbeefbabe19771218);
            assert(pre_pad[1] == 0x5ca1ab1e20180721);
            assert(post_pad[0] == 0xbeefbabe19771218);
            assert(post_pad[1] == 0x5ca1ab1e20180721);
#endif

            if (alloc_size <= max_alloc_size_)
                mapping_pool_[alloc_size]->dealloc(h);
            else
            {
                free(h);                
            }
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
            mapping_pool_ = (tls_pool**)_aligned_malloc(sizeof(tls_pool*) * (max_alloc_size_ + 1), kL1CacheAlignSize);
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
            _aligned_free(mapping_pool_);
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
            assert(sizes.size() <= 32, "per-size allocator should be lower than 32");
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

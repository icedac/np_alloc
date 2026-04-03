/****************************************************************************
 *  np_thread_linux.cpp — POSIX implementation of thread_atexit
 *  Replaces np_thread.cpp (Windows FLS) with pthread_key_create
 ***/
#include <pthread.h>
#include <atomic>
#include <thread>
#include <cassert>
#include <functional>

// Pull in local_new.h from parent
#include "../src/np_common.h"
#include "../src/np_thread.h"
#include "../src/local_new.h"

namespace np {

    class thread_atexit_caller {
        struct call_node {
            fn_atexit_callback  callback;
            call_node* next = nullptr;
        };
        call_node* call_list_ = nullptr;

        void invoke() {
            while (call_list_) {
                auto* cb = call_list_;
                call_list_ = call_list_->next;
                if (cb->callback)
                    cb->callback();
                internal::t_delete(cb);
            }
        }

    public:
        ~thread_atexit_caller() {
            invoke();
        }

        void add(fn_atexit_callback f) {
            if (f) {
                auto* cb = internal::t_new<call_node>();
                cb->callback = f;
                cb->next = call_list_;
                call_list_ = cb;
            }
        }
    };

    static pthread_key_t s_tls_key;
    static pthread_once_t s_key_once = PTHREAD_ONCE_INIT;

    static void tls_destructor(void* data) {
        auto* caller = reinterpret_cast<thread_atexit_caller*>(data);
        if (caller) {
            internal::t_delete(caller);
        }
    }

    static void create_tls_key() {
        pthread_key_create(&s_tls_key, tls_destructor);
    }

    static thread_local thread_atexit_caller* tls_caller = nullptr;

    NP_API void thread_atexit(std::function< void(void) > f)
    {
        pthread_once(&s_key_once, create_tls_key);

        if (nullptr == tls_caller) {
            tls_caller = internal::t_new<thread_atexit_caller>();
            assert(tls_caller);
            pthread_setspecific(s_tls_key, tls_caller);
        }

        tls_caller->add(f);
    }

    NP_API bool thread_test_atexit()
    {
        std::atomic<bool> s_set_test{false};

        std::thread t([&s_set_test]() {
            thread_atexit([&s_set_test]() {
                s_set_test = true;
            });
        });
        t.join();

        bool expected_true = true;
        if (!s_set_test.compare_exchange_strong(expected_true, false))
        {
            return false;
        }

        return true;
    }
}

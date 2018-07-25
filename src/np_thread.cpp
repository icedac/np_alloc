/****************************************************************************
 *
 *  np_thread.cpp
 *      ($\np_alloc\src)
 *
 *  by icedac 
 *
 ***/
#include "stdafx.h"
#include "np_thread.h"
#include "local_new.h"
#include <atomic>
#include <thread>
#include <Fibersapi.h>

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

        static void NTAPI _fls_callback(PVOID data) // PFLS_CALLBACK_FUNCTION
        {
            auto* caller = reinterpret_cast<thread_atexit_caller*>(data);
            internal::t_delete(caller);            
        }
    };

    static thread_local thread_atexit_caller* tls_caller = nullptr;    

    NP_API void thread_atexit(std::function< void(void) > f)
    {
        if (nullptr == tls_caller) {
            tls_caller = internal::t_new<thread_atexit_caller>();
            assert(tls_caller);

            DWORD fls_index = ::FlsAlloc(&thread_atexit_caller::_fls_callback);
            ::FlsSetValue(fls_index, tls_caller);
        }

        tls_caller->add(f);
    }

    std::atomic<bool> s_set_test;

    NP_API bool thread_test_atexit()
    {
        s_set_test = false;

        std::thread t([]() {
            thread_atexit([]() {
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

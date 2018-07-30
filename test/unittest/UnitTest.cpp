// CornUnitTest.cpp : Defines the entry point for the console application.
//
#include <windows.h>
#include <bitset>
#include <thread>
#include <iomanip>
#include <condition_variable>

#include "gtest/gtest.h"
#include "../../src/np_common.h"
#include "../../src/np_bitset.h"
#include "../../src/np_mmap.h"
#include "../../src/np_alloc.h"
 
//
//TEST(SquareTests, Square)
//{
//	EXPECT_EQ(0, square(0));
//	EXPECT_EQ(-4, square(-2));
//}

TEST(np_alloc, bitset_64) {
    using namespace np;

    bitset<64> b;
    EXPECT_EQ(b.count(), 0);

    b.set(10);
    b.set(20);
    b.set(30);
    EXPECT_EQ(b.count(), 3);
    EXPECT_EQ(b.get_frist(), 10);
    b.clear(10);
    EXPECT_EQ(b.count(), 2);
    EXPECT_EQ(b.get_frist(), 20);
    b.clear(20);
    b.clear(30);
    b.set(50);
    EXPECT_EQ(b.count(), 1);
    EXPECT_EQ(b.get_frist(), 50);

    b.set(25);
    
    auto itr = b.at(b.get_frist());
    EXPECT_EQ(*itr, true);
    EXPECT_EQ(itr.get_pos(), 25);
    EXPECT_EQ(itr.end(), false);
    itr.next();
    EXPECT_EQ(*itr, true);
    EXPECT_EQ(itr.get_pos(), 50);
    EXPECT_EQ(itr.end(), false);
    itr.next();
    EXPECT_EQ(itr.end(), true);
}

TEST(np_alloc, bitset_4096) {
    using namespace np;

    bitset<4096> b;
    EXPECT_EQ(b.count(), 0);

    b.set(1000);
    b.set(2000);
    b.set(3000);
    EXPECT_EQ(b.count(), 3);
    EXPECT_EQ(b.get_frist(), 1000);
    b.clear(1000);
    EXPECT_EQ(b.count(), 2);
    EXPECT_EQ(b.get_frist(), 2000);
    b.clear(2000);
    b.clear(3000);
    b.set(3500);
    EXPECT_EQ(b.count(), 1);
    EXPECT_EQ(b.get_frist(), 3500);

    b.set(2500);
    auto itr = b.at(b.get_frist());
    EXPECT_EQ(*itr, true);
    EXPECT_EQ(itr.get_pos(), 2500);
    EXPECT_EQ(itr.end(), false);
    itr.next();
    EXPECT_EQ(*itr, true);
    EXPECT_EQ(itr.get_pos(), 3500);
    EXPECT_EQ(itr.end(), false);
    itr.next();
    EXPECT_EQ(itr.end(), true);
}


struct lfstack_test {
    lfstack_test* next = nullptr;
};

TEST(np_alloc, lf_stack) {
    using namespace np;

    np::lf_stack< lfstack_test > stack;

    auto test_alloc_fun = [&]() {
        return new lfstack_test;
    };

    // multithread test

    enum e_op {
        OP_FREE,
        OP_ALLOC
    };

    const auto kThreadCount = 20;
    const auto kMaxAllocCount = 50;
    const auto kIterationCount = 2000;

    const auto kDefaultAllocCount = 2000;

    std::cout << "free_count: " << stack.count_free_list() << "\n";

    for (auto i = 0; i < kDefaultAllocCount; ++i) {
        stack.push(test_alloc_fun());
    }

    std::cout << "free_count: " << stack.count_free_list() << "\n";

    std::thread ts[kThreadCount];
    for (auto& t : ts) {
        t = std::thread([&]() {
            vector< lfstack_test* > allocated;

            auto ss = std::rand() % kMaxAllocCount;
            for (auto i = 0; i < ss; ++i)
                allocated.push_back(stack.pop());


            e_op op = OP_ALLOC;
            for (auto i = 0; i < kIterationCount; ++i) {
                switch (op) {
                case OP_ALLOC:
                    if (allocated.size() < kMaxAllocCount)
                    {
                        auto* t = stack.pop();
                        if (!t) t = test_alloc_fun();
                        allocated.push_back(t);
                    }
                    else
                        op = OP_FREE;
                    break;
                case OP_FREE:
                    if (allocated.size() > 0) {
                        auto* ptr = allocated.back();
                        allocated.pop_back();
                        stack.push(ptr);
                    }
                    else
                        op = OP_ALLOC;
                    break;
                }
            }

            // std::cout << std::setw(10) << std::this_thread::get_id() << ":thread finished\n";

            for (auto* v : allocated) stack.push(v);
            allocated.clear();
        });
    }

    std::cout << "\trunning...\n";

    for (auto& t : ts) {
        t.join();
    }

    std::cout << "\tdone...\n";

    std::cout << "free_count: " << stack.count_free_list() << "\n";
    
    EXPECT_EQ(kDefaultAllocCount, stack.count_free_list());

    lfstack_test* ptr = stack.pop();
    while (ptr) {
        delete ptr;
        ptr = stack.pop();
    }

}


TEST(np_alloc, mmap) {
    using namespace np;

    mmap mm;
    mm.reserve();

    std::cout << mm.debug_as_string();

    vector< void* > allocated;
    for (auto i = 0; i < 5; ++i) {
        allocated.push_back(mm.alloc_chunk());
    }
    for (auto* v : allocated) mm.free_chunk(v);
    allocated.clear();

    for (auto i = 0; i < 10; ++i) {
        allocated.push_back(mm.alloc_chunk());
    }
    for (auto* v : allocated) mm.free_chunk(v);
    allocated.clear();

    for (auto i = 0; i < 15; ++i) {
        allocated.push_back(mm.alloc_chunk());
    }

    std::cout << mm.debug_as_string();
    for (auto* v : allocated) mm.free_chunk(v);
    allocated.clear();


    // multithread test

    enum e_op {
        OP_FREE,
        OP_ALLOC
    };

    const auto kThreadCount = 50;
    const auto kMaxAllocCount = 50;
#ifdef _DEBUG
    const auto kIterationCount = 500;
#else
    const auto kIterationCount = 5000;
#endif

    std::thread ts[kThreadCount];
    for (auto& t : ts) {
        t = std::thread([&]() {
            vector< void* > allocated;

            auto ss = std::rand() % kMaxAllocCount;
            for ( auto i = 0; i < ss; ++i )
                allocated.push_back(mm.alloc_chunk());


            e_op op = OP_ALLOC;
            for (auto i = 0; i < kIterationCount; ++i) {
                switch (op) {
                case OP_ALLOC:
                    if (allocated.size() < kMaxAllocCount)
                        allocated.push_back(mm.alloc_chunk());
                    else
                        op = OP_FREE;
                    break;
                case OP_FREE:
                    if (allocated.size() > 0) {
                        auto* ptr = allocated.back();
                        allocated.pop_back();
                        mm.free_chunk(ptr);
                    }
                    else
                        op = OP_ALLOC;
                    break;
                }
            }

            // std::cout << std::setw(10) << std::this_thread::get_id() << ":thread finished\n";

            for (auto* v : allocated) mm.free_chunk(v);
            allocated.clear();
        });
    }

    std::cout << "\trunning...\n";

    for (auto& t : ts) {
        t.join();
    }

    std::cout << "\tdone...\n";

    // mm.__debug_free_list();


    EXPECT_EQ(mm.get_total_allocated(), mm.count_free_list());

    std::cout << mm.debug_as_string();
}

class event {
public:
	event() {
		handle_ = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);
	}
	~event() {
		::CloseHandle(handle_);
	}

	void signal_all() {
		::SetEvent(handle_);
	}

	void wait() {
		::WaitForSingleObject(handle_, INFINITE);
	}

private:
	HANDLE handle_ = 0;
};

TEST(np_alloc, np_alloc) {
    using namespace np;

    // multithread test

    enum e_op {
        OP_FREE,
        OP_ALLOC
    };

	const auto kThreadCreationTestCount = 500;
	const auto kThreadCount = 50;
    const auto kMaxAllocCount = 1000;

#ifdef _DEBUG
    const auto kIterationCount = 3000;
#else
    const auto kIterationCount = 1000000;
#endif

	std::cout << "thread [" << std::hex << std::this_thread::get_id() << "]: main thread\n";


	// singleton creation thread contention test 
	std::thread ts_creation[kThreadCreationTestCount];
	event ev;
	for (auto& t : ts_creation) {
		t = std::thread( [&]() {
			ev.wait();
			auto* p = np_alloc(100);
			np_free(p);
		} );
	}

	ev.signal_all();

	for (auto& t : ts_creation) {
		t.join();
	}



    np_debug_print();

	std::thread ts[kThreadCount];
	for (auto& t : ts) {
        t = std::thread([&]() {
            vector< void* > allocated;

            auto ss = std::rand() % kMaxAllocCount;
            for (auto i = 0; i < ss; ++i)
            {
                auto alloc_size = 100 + std::rand() % 3000; // 100 ~ 3100
                allocated.push_back(np_alloc(alloc_size));
            }

            e_op op = OP_ALLOC;
            for (auto i = 0; i < kIterationCount; ++i) {
                switch (op) {
                case OP_ALLOC:
                    if (allocated.size() < kMaxAllocCount) {
                        auto alloc_size = 100 + std::rand() % 3000; // 100 ~ 3100
                        allocated.push_back( np_alloc(alloc_size) );
                    }
                    else
                        op = OP_FREE;
                    break;
                case OP_FREE:
                    if (allocated.size() > 0) {
                        auto* ptr = allocated.back();
                        allocated.pop_back();
                        np_free(ptr);
                    }
                    else
                        op = OP_ALLOC;
                    break;
                }
            }

            // std::cout << std::setw(10) << std::this_thread::get_id() << ":thread finished\n";

            for (auto* ptr : allocated) np_free(ptr);
            allocated.clear();
        });
    }

    std::cout << "\trunning...\n";

    for (auto& t : ts) {
        t.join();
    }

    std::cout << "\tdone...\n";

    np_debug_print();

    // mm.__debug_free_list();
}


class InstructionSet
{
public:
    std::string Vendor(void) { return vendor_; }
    std::string Brand(void) { return brand_; }

    bool SSE3(void) { return f_1_ECX_[0]; }
    bool PCLMULQDQ(void) { return f_1_ECX_[1]; }
    bool MONITOR(void) { return f_1_ECX_[3]; }
    bool SSSE3(void) { return f_1_ECX_[9]; }
    bool FMA(void) { return f_1_ECX_[12]; }
    bool CMPXCHG16B(void) { return f_1_ECX_[13]; }
    bool SSE41(void) { return f_1_ECX_[19]; }
    bool SSE42(void) { return f_1_ECX_[20]; }
    bool MOVBE(void) { return f_1_ECX_[22]; }
    bool POPCNT(void) { return f_1_ECX_[23]; }
    bool AES(void) { return f_1_ECX_[25]; }
    bool XSAVE(void) { return f_1_ECX_[26]; }
    bool OSXSAVE(void) { return f_1_ECX_[27]; }
    bool AVX(void) { return f_1_ECX_[28]; }
    bool F16C(void) { return f_1_ECX_[29]; }
    bool RDRAND(void) { return f_1_ECX_[30]; }

    bool MSR(void) { return f_1_EDX_[5]; }
    bool CX8(void) { return f_1_EDX_[8]; }
    bool SEP(void) { return f_1_EDX_[11]; }
    bool CMOV(void) { return f_1_EDX_[15]; }
    bool CLFSH(void) { return f_1_EDX_[19]; }
    bool MMX(void) { return f_1_EDX_[23]; }
    bool FXSR(void) { return f_1_EDX_[24]; }
    bool SSE(void) { return f_1_EDX_[25]; }
    bool SSE2(void) { return f_1_EDX_[26]; }

    bool FSGSBASE(void) { return f_7_EBX_[0]; }
    bool BMI1(void) { return f_7_EBX_[3]; }
    bool HLE(void) { return isIntel_ && f_7_EBX_[4]; }
    bool AVX2(void) { return f_7_EBX_[5]; }
    bool BMI2(void) { return f_7_EBX_[8]; }
    bool ERMS(void) { return f_7_EBX_[9]; }
    bool INVPCID(void) { return f_7_EBX_[10]; }
    bool RTM(void) { return isIntel_ && f_7_EBX_[11]; }
    bool AVX512F(void) { return f_7_EBX_[16]; }
    bool RDSEED(void) { return f_7_EBX_[18]; }
    bool ADX(void) { return f_7_EBX_[19]; }
    bool AVX512PF(void) { return f_7_EBX_[26]; }
    bool AVX512ER(void) { return f_7_EBX_[27]; }
    bool AVX512CD(void) { return f_7_EBX_[28]; }
    bool SHA(void) { return f_7_EBX_[29]; }

    bool PREFETCHWT1(void) { return f_7_ECX_[0]; }

    bool LAHF(void) { return f_81_ECX_[0]; }
    bool LZCNT(void) { return isIntel_ && f_81_ECX_[5]; }
    bool ABM(void) { return isAMD_ && f_81_ECX_[5]; }
    bool SSE4a(void) { return isAMD_ && f_81_ECX_[6]; }
    bool XOP(void) { return isAMD_ && f_81_ECX_[11]; }
    bool TBM(void) { return isAMD_ && f_81_ECX_[21]; }

    bool SYSCALL(void) { return isIntel_ && f_81_EDX_[11]; }
    bool MMXEXT(void) { return isAMD_ && f_81_EDX_[22]; }
    bool RDTSCP(void) { return isIntel_ && f_81_EDX_[27]; }
    bool _3DNOWEXT(void) { return isAMD_ && f_81_EDX_[30]; }
    bool _3DNOW(void) { return isAMD_ && f_81_EDX_[31]; }

public:
    InstructionSet()
        : nIds_{ 0 },
        nExIds_{ 0 },
        isIntel_{ false },
        isAMD_{ false },
        f_1_ECX_{ 0 },
        f_1_EDX_{ 0 },
        f_7_EBX_{ 0 },
        f_7_ECX_{ 0 },
        f_81_ECX_{ 0 },
        f_81_EDX_{ 0 },
        data_{},
        extdata_{}
    {
        std::array<int, 4> cpui;

        // Calling __cpuid with 0x0 as the function_id argument  
        // gets the number of the highest valid function ID.  
        __cpuid(cpui.data(), 0);
        nIds_ = cpui[0];

        for (int i = 0; i <= nIds_; ++i)
        {
            __cpuidex(cpui.data(), i, 0);
            data_.push_back(cpui);
        }

        // Capture vendor string  
        char vendor[0x20];
        memset(vendor, 0, sizeof(vendor));
        *reinterpret_cast<int*>(vendor) = data_[0][1];
        *reinterpret_cast<int*>(vendor + 4) = data_[0][3];
        *reinterpret_cast<int*>(vendor + 8) = data_[0][2];
        vendor_ = vendor;
        if (vendor_ == "GenuineIntel")
        {
            isIntel_ = true;
        }
        else if (vendor_ == "AuthenticAMD")
        {
            isAMD_ = true;
        }

        // load bitset with flags for function 0x00000001  
        if (nIds_ >= 1)
        {
            f_1_ECX_ = data_[1][2];
            f_1_EDX_ = data_[1][3];
        }

        // load bitset with flags for function 0x00000007  
        if (nIds_ >= 7)
        {
            f_7_EBX_ = data_[7][1];
            f_7_ECX_ = data_[7][2];
        }

        // Calling __cpuid with 0x80000000 as the function_id argument  
        // gets the number of the highest valid extended ID.  
        __cpuid(cpui.data(), 0x80000000);
        nExIds_ = cpui[0];

        char brand[0x40];
        memset(brand, 0, sizeof(brand));

        for (int i = 0x80000000; i <= nExIds_; ++i)
        {
            __cpuidex(cpui.data(), i, 0);
            extdata_.push_back(cpui);
        }

        // load bitset with flags for function 0x80000001  
        if (nExIds_ >= 0x80000001)
        {
            f_81_ECX_ = extdata_[1][2];
            f_81_EDX_ = extdata_[1][3];
        }

        // Interpret CPU brand string if reported  
        if (nExIds_ >= 0x80000004)
        {
            memcpy(brand, extdata_[2].data(), sizeof(cpui));
            memcpy(brand + 16, extdata_[3].data(), sizeof(cpui));
            memcpy(brand + 32, extdata_[4].data(), sizeof(cpui));
            brand_ = brand;
        }
    };

private:
    int nIds_;
    int nExIds_;
    std::string vendor_;
    std::string brand_;
    bool isIntel_;
    bool isAMD_;
    std::bitset<32> f_1_ECX_;
    std::bitset<32> f_1_EDX_;
    std::bitset<32> f_7_EBX_;
    std::bitset<32> f_7_ECX_;
    std::bitset<32> f_81_ECX_;
    std::bitset<32> f_81_EDX_;
    std::vector<std::array<int, 4>> data_;
    std::vector<std::array<int, 4>> extdata_;
};

void test_cpuid() {
    auto fn_padding_space = [](const std::string& s, size_t min_size) {
        if (s.size() >= min_size) return s;
        auto padding_size = min_size - s.size();
        assert(padding_size <= 64);
        return (s + (&"                                                                "[64 - padding_size]));
    };

    auto& outstream = std::cout;
    auto support_message = [fn_padding_space, &outstream](std::string isa_feature, bool is_supported) {
        outstream << fn_padding_space(isa_feature, 20) << (is_supported ? " supported" : " not supported") << std::endl;
    };

    InstructionSet is;
    std::cout << is.Vendor() << std::endl;
    std::cout << is.Brand() << std::endl;
    support_message("POPCNT", is.POPCNT());
    support_message("LZCNT", is.LZCNT());
}

TEST(np_alloc, test_cpuid) {
    using namespace np;

    InstructionSet is;
    EXPECT_EQ(is.POPCNT(), true);
}


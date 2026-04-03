cmake_minimum_required(VERSION 3.14)
project(np_alloc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Library ──────────────────────────────────────────────────────────────
add_library(np_alloc STATIC
    src/np_alloc.cpp
    src/np_mmap.cpp
    src/np_thread.cpp
)

target_include_directories(np_alloc PUBLIC src)

if(MSVC)
    target_compile_options(np_alloc PRIVATE /FI"stdafx.h")
else()
    # Force-include stdafx.h (precompiled header emulation)
    target_compile_options(np_alloc PRIVATE -include stdafx.h)
    target_link_libraries(np_alloc PUBLIC pthread)

    # GCC on x86_64: 16-byte atomic CAS (cmpxchg16b) needs -mcx16 and -latomic
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        target_compile_options(np_alloc PUBLIC -mcx16)
        target_link_libraries(np_alloc PUBLIC atomic)
    endif()
endif()

# ── Benchmark ────────────────────────────────────────────────────────────
add_executable(np_bench bench/bench.cpp)
target_link_libraries(np_bench PRIVATE np_alloc)

if(MSVC)
    target_compile_options(np_bench PRIVATE /FI"stdafx.h")
else()
    target_compile_options(np_bench PRIVATE -include stdafx.h)
endif()

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

# Force-include stdafx.h (precompiled header emulation)
if(MSVC)
    target_compile_options(np_alloc PRIVATE /FI"stdafx.h")
else()
    target_compile_options(np_alloc PRIVATE -include stdafx.h)
    target_link_libraries(np_alloc PUBLIC pthread)
endif()

# ── Benchmark ────────────────────────────────────────────────────────────
add_executable(np_bench bench/bench.cpp)
target_link_libraries(np_bench PRIVATE np_alloc)

if(MSVC)
    target_compile_options(np_bench PRIVATE /FI"stdafx.h")
else()
    target_compile_options(np_bench PRIVATE -include stdafx.h)
endif()

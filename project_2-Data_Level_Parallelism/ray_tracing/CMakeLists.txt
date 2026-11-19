cmake_minimum_required(VERSION 3.16)
project(ray_tracing_p2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sources
set(SHARED_SRC  ${CMAKE_SOURCE_DIR}/shared/src/utils/Timer.cpp)
set(RT_SRC
    src/core/SequentialRenderer.cpp
    src/main_benchmark.cpp
)

add_executable(rt_benchmark ${RT_SRC} ${SHARED_SRC})

target_include_directories(rt_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/include     # geometry/, config/, utils/
    ${CMAKE_CURRENT_SOURCE_DIR}/include    # core/strategies/
)

target_compile_options(rt_benchmark PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:$<$<CONFIG:Release>:-O3 -march=native>>
    $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release>:/O2>>
)

cmake_minimum_required(VERSION 3.16)
project(dlp_p2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(DLP_ENABLE_CUDA "Build the CUDA ray-tracing renderer" OFF)
if(DLP_ENABLE_CUDA)
    include(CheckLanguage)
    check_language(CUDA)
    if(NOT CMAKE_CUDA_COMPILER)
        message(FATAL_ERROR "DLP_ENABLE_CUDA is ON, but no CUDA compiler (nvcc) was found")
    endif()
    enable_language(CUDA)
    set(DLP_CUDA_ARCHITECTURE "61" CACHE STRING "CUDA compute capability for the target GPU")
endif()

include(CTest)
add_subdirectory(ray_tracing)
add_executable(plot_benchmark scripts/plot_benchmark.cpp)

message(STATUS "dlp_p2 targets: rt_benchmark")

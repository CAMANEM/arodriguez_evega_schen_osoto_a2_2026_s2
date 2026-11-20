cmake_minimum_required(VERSION 3.16)
project(dlp_p2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(CTest)
add_subdirectory(ray_tracing)

message(STATUS "dlp_p2 targets: rt_benchmark")

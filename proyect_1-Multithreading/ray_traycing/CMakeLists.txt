cmake_minimum_required(VERSION 3.10)
project(ray_traycing LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(raytracing_core
    src/core/CMPRenderer.cpp
    src/core/CameraOrbit.cpp
    src/core/CoarseRenderer.cpp
    src/core/FinegrainedRenderer.cpp
    src/core/SMTRenderer.cpp
    src/core/SequentialRenderer.cpp
    src/core/Timer.cpp
)
target_include_directories(raytracing_core PUBLIC include)
target_link_libraries(raytracing_core PUBLIC Threads::Threads)

add_executable(raytracing_benchmark
    src/main_benchmark.cpp
    ../shared/src/metrics_interface.cpp
)
target_include_directories(raytracing_benchmark PRIVATE ../shared/include)
target_link_libraries(raytracing_benchmark PRIVATE raytracing_core)

add_executable(raytracing_visual src/main_visual.cpp)
target_link_libraries(raytracing_visual PRIVATE raytracing_core)

if(BUILD_TESTING)
    add_executable(raytracing_pixel_kernel_test tests/pixel_kernel_equivalence.cpp)
    target_link_libraries(raytracing_pixel_kernel_test PRIVATE raytracing_core)
    add_test(NAME pixel_kernel_equivalence COMMAND raytracing_pixel_kernel_test)
endif()
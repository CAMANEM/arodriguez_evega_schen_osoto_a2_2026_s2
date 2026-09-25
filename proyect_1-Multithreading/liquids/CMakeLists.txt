cmake_minimum_required(VERSION 3.16)

project(liquids_demo2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(LIQUIDS_BUILD_TESTS "Build the liquids interface tests" ON)

find_package(Threads REQUIRED)

add_library(liquids_core STATIC
    src/core/SphConfig.cpp
    src/core/SphFluid.cpp
    src/core/SphMetrics.cpp
    src/core/SphParticle.cpp
    ../shared/src/metrics_interface.cpp
)

target_include_directories(liquids_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)
target_link_libraries(liquids_core PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(liquids_core PRIVATE /W4)
else()
    target_compile_options(liquids_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(LIQUIDS_BUILD_TESTS)
    enable_testing()
    add_executable(liquids_interface_tests tests/test_liquids_interface.cpp)
    target_link_libraries(liquids_interface_tests PRIVATE liquids_core)
    add_test(NAME liquids_interface_tests COMMAND liquids_interface_tests)
endif()

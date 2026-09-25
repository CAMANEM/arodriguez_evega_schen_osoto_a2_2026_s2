cmake_minimum_required(VERSION 3.16)

project(boids_demo2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BOIDS_BUILD_VISUAL "Build the optional Raylib visualization" ON)
option(BOIDS_BUILD_TESTS "Build the automated validation tests" ON)

find_package(Threads REQUIRED)

set(BOIDS_CORE_SOURCES
    src/core/Boid.cpp
    src/core/CmpScheme.cpp
    src/core/CoarseGrainedScheme.cpp
    src/core/FineGrainedScheme.cpp
    src/core/Flock.cpp
    src/core/FlockingConfig.cpp
    src/core/FlockingRules.cpp
    src/core/FrameWriter.cpp
    src/core/SequentialScheme.cpp
    src/core/SmtScheme.cpp
    src/core/SteeringContext.cpp
    src/core/ThreadedScheme.cpp
    src/core/Timer.cpp
    src/core/Vector2D.cpp
    ../shared/src/metrics_interface.cpp
)

add_library(boids_core STATIC ${BOIDS_CORE_SOURCES})
target_compile_features(boids_core PUBLIC cxx_std_17)
target_include_directories(boids_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)
target_link_libraries(boids_core PUBLIC Threads::Threads)

if(MSVC)
    target_compile_options(boids_core PRIVATE /W4)
else()
    target_compile_options(boids_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(boids_benchmark src/main_benchmark.cpp)
target_compile_features(boids_benchmark PRIVATE cxx_std_17)
target_link_libraries(boids_benchmark PRIVATE boids_core)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(boids_benchmark PRIVATE stdc++fs)
endif()

if(BOIDS_BUILD_VISUAL)
    find_package(raylib QUIET)
    if(TARGET raylib)
        add_executable(boids_visual
            src/main_visual.cpp
            src/visual/RaylibRenderer.cpp
        )
        target_link_libraries(boids_visual PRIVATE boids_core raylib)
    elseif(TARGET raylib::raylib)
        add_executable(boids_visual
            src/main_visual.cpp
            src/visual/RaylibRenderer.cpp
        )
        target_link_libraries(boids_visual PRIVATE boids_core raylib::raylib)
    else()
        message(STATUS "Raylib not found: boids_visual will not be built")
    endif()
endif()

if(BOIDS_BUILD_TESTS)
    enable_testing()
    add_executable(boids_tests tests/test_boids.cpp)
    target_link_libraries(boids_tests PRIVATE boids_core)
    add_test(NAME boids_tests COMMAND boids_tests)
endif()

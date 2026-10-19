cmake_minimum_required(VERSION 3.16)

# Unified build tree for Project 1 (Multithreading).
# Produces separate process binaries so campaign cells cannot leak state/memory
# across problems.
project(multithreading_p1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Campaign-oriented defaults: benchmarks without GUI / tests.
# Child option() calls keep these values when already set in the cache.
option(BOIDS_BUILD_VISUAL "Link Raylib into the Boids demo binary" OFF)
option(BOIDS_BUILD_TESTS "Build Boids automated tests" OFF)
option(LIQUIDS_BUILD_TESTS "Build Liquids interface tests" OFF)
option(BUILD_TESTING "Build CTest targets (Ray / others)" OFF)
option(NBODY_ENABLE "Build the N-Body binary (requires Raylib)" ON)

add_subdirectory(Boids)
add_subdirectory(liquids)
add_subdirectory(ray_traycing)

if(NBODY_ENABLE)
    add_subdirectory(n_body)
endif()

message(STATUS "multithreading_p1 targets:")
message(STATUS "  boids_benchmark, liquids_benchmark, raytracing_benchmark")
if(NBODY_ENABLE)
    message(STATUS "  n_body (Raylib required)")
endif()

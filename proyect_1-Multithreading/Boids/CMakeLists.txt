cmake_minimum_required(VERSION 3.16)

project(boids_demo2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BOIDS_BUILD_VISUAL "Link Raylib into the unified boids binary (--gui)" ON)
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
    src/cli/CliOptions.cpp
    src/cli/SchemeFactory.cpp
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

# Ejecutable unificado: modelo / parametros / GUI se eligen por CLI.
add_executable(boids src/main.cpp)
target_compile_features(boids PRIVATE cxx_std_17)
target_link_libraries(boids PRIVATE boids_core)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
    target_link_libraries(boids PRIVATE stdc++fs)
endif()

set(BOIDS_RAYLIB_TARGET "")
if(BOIDS_BUILD_VISUAL)
    find_package(raylib QUIET)
    if(TARGET raylib)
        set(BOIDS_RAYLIB_TARGET raylib)
    elseif(TARGET raylib::raylib)
        set(BOIDS_RAYLIB_TARGET raylib::raylib)
    else()
        include(FetchContent)
        set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
        FetchContent_Declare(
            raylib
            GIT_REPOSITORY https://github.com/raysan5/raylib.git
            GIT_TAG 5.5
        )
        FetchContent_MakeAvailable(raylib)
        if(TARGET raylib)
            set(BOIDS_RAYLIB_TARGET raylib)
        endif()
    endif()
endif()

if(BOIDS_RAYLIB_TARGET)
    target_sources(boids PRIVATE src/visual/RaylibRenderer.cpp)
    target_link_libraries(boids PRIVATE ${BOIDS_RAYLIB_TARGET})
    target_compile_definitions(boids PRIVATE BOIDS_HAS_GUI=1)
    message(STATUS "boids: GUI habilitada (use --gui)")
else()
    message(STATUS "boids: sin Raylib --gui no estara disponible")
endif()

if(BOIDS_BUILD_TESTS)
    enable_testing()
    add_executable(boids_tests tests/test_boids.cpp)
    target_link_libraries(boids_tests PRIVATE boids_core)
    add_test(NAME boids_tests COMMAND boids_tests)
endif()

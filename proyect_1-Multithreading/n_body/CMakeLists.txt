cmake_minimum_required(VERSION 3.16)

project(n_body LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

# ---------------------------------------------------------------------------
# Raylib discovery (mirrors the legacy Makefile defaults).
# Override with -DNBODY_RAYLIB_ROOT=... or env RAYLIB_PATH / NBODY_RAYLIB_ROOT.
# ---------------------------------------------------------------------------
set(_nbody_raylib_root "")
if(DEFINED NBODY_RAYLIB_ROOT AND NOT "${NBODY_RAYLIB_ROOT}" STREQUAL "")
    set(_nbody_raylib_root "${NBODY_RAYLIB_ROOT}")
elseif(DEFINED ENV{NBODY_RAYLIB_ROOT} AND NOT "$ENV{NBODY_RAYLIB_ROOT}" STREQUAL "")
    set(_nbody_raylib_root "$ENV{NBODY_RAYLIB_ROOT}")
elseif(DEFINED ENV{RAYLIB_PATH} AND NOT "$ENV{RAYLIB_PATH}" STREQUAL "")
    set(_nbody_raylib_root "$ENV{RAYLIB_PATH}")
elseif(EXISTS "C:/raylib/w64devkit")
    set(_nbody_raylib_root "C:/raylib/w64devkit")
endif()

set(NBODY_RAYLIB_TARGET "")
find_package(raylib QUIET)
if(TARGET raylib)
    set(NBODY_RAYLIB_TARGET raylib)
elseif(TARGET raylib::raylib)
    set(NBODY_RAYLIB_TARGET raylib::raylib)
elseif(NOT "${_nbody_raylib_root}" STREQUAL "")
    set(_rl_inc "${_nbody_raylib_root}/include")
    set(_rl_lib "${_nbody_raylib_root}/lib")
    find_library(NBODY_RAYLIB_LIB
        NAMES raylib libraylib
        PATHS "${_rl_lib}"
        NO_DEFAULT_PATH
    )
    if(NBODY_RAYLIB_LIB AND EXISTS "${_rl_inc}/raylib.h")
        add_library(nbody_raylib UNKNOWN IMPORTED)
        set_target_properties(nbody_raylib PROPERTIES
            IMPORTED_LOCATION "${NBODY_RAYLIB_LIB}"
            INTERFACE_INCLUDE_DIRECTORIES "${_rl_inc}"
        )
        set(NBODY_RAYLIB_TARGET nbody_raylib)
        message(STATUS "n_body: using Raylib from ${_nbody_raylib_root}")
    endif()
endif()

if(NBODY_RAYLIB_TARGET STREQUAL "")
    message(STATUS "n_body: Raylib not found locally; fetching raysan5/raylib 5.5")
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
        set(NBODY_RAYLIB_TARGET raylib)
    elseif(TARGET raylib::raylib)
        set(NBODY_RAYLIB_TARGET raylib::raylib)
    endif()
endif()

if(NBODY_RAYLIB_TARGET STREQUAL "")
    message(FATAL_ERROR
        "n_body requires Raylib. Install raylib, set "
        "-DNBODY_RAYLIB_ROOT=<prefix> (include/raylib.h + lib/), "
        "or allow network for FetchContent. "
        "Windows Makefile default: C:/raylib/w64devkit. "
        "To skip N-Body: -DNBODY_ENABLE=OFF")
endif()

add_executable(n_body
    src/main.cpp
    src/body.cpp
    src/fine_grained.cpp
    src/coarse_grained.cpp
    src/smt.cpp
    src/cmp.cpp
    src/stall_policy.cpp
)

target_include_directories(n_body
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../shared/include
)

target_link_libraries(n_body
    PRIVATE
        Threads::Threads
        ${NBODY_RAYLIB_TARGET}
)

if(WIN32)
    target_link_libraries(n_body PRIVATE opengl32 gdi32 winmm)
endif()

if(MSVC)
    target_compile_options(n_body PRIVATE /W4)
else()
    target_compile_options(n_body PRIVATE -Wall -Wextra -Wpedantic)
endif()

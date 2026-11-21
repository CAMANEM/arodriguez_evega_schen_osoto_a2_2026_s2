cmake_minimum_required(VERSION 3.16)
project(ray_tracing_p2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Sources
set(SHARED_SRC  ${CMAKE_SOURCE_DIR}/shared/src/utils/Timer.cpp)
set(SIMD_SRC src/core/SimdRenderer.cpp)
set(CUDA_SRC src/cuda/CudaRenderer.cu)
set(CUDA_TEST_SRC tests/correctness/cuda_renderer_test.cu)
set(RT_SRC
    src/core/SequentialRenderer.cpp
    ${SIMD_SRC}
    src/main_benchmark.cpp
)

if(DLP_ENABLE_CUDA)
    list(APPEND RT_SRC ${CUDA_SRC})
endif()

add_executable(rt_benchmark ${RT_SRC} ${SHARED_SRC})

target_include_directories(rt_benchmark PRIVATE
    ${CMAKE_SOURCE_DIR}/shared/include     # geometry/, config/, utils/
    ${CMAKE_CURRENT_SOURCE_DIR}/include    # core/strategies/
)

if(MSVC)
    set_source_files_properties(${SIMD_SRC} PROPERTIES COMPILE_OPTIONS "/arch:AVX2")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${SIMD_SRC} PROPERTIES COMPILE_OPTIONS "-mavx2")
else()
    message(FATAL_ERROR "The SIMD renderer requires an MSVC, GNU, or Clang compiler with AVX2 support")
endif()

target_compile_options(rt_benchmark PRIVATE
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-O3>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:GNU,Clang>,$<CONFIG:Release>>:-march=native>
    $<$<AND:$<COMPILE_LANGUAGE:CXX>,$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/O2>
)

if(DLP_ENABLE_CUDA)
    set_source_files_properties(${CUDA_SRC} PROPERTIES
        COMPILE_OPTIONS "--gpu-architecture=sm_${DLP_CUDA_ARCHITECTURE};--fmad=false"
    )
    set_target_properties(rt_benchmark PROPERTIES
        CUDA_STANDARD 17
        CUDA_STANDARD_REQUIRED YES
    )
    target_compile_definitions(rt_benchmark PRIVATE DLP_ENABLE_CUDA=1)
endif()

if(BUILD_TESTING)
    add_executable(simd_correctness_test
        tests/correctness/simd_renderer_test.cpp
        src/core/SequentialRenderer.cpp
        ${SIMD_SRC}
    )
    target_include_directories(simd_correctness_test PRIVATE
        ${CMAKE_SOURCE_DIR}/shared/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    add_test(NAME simd_correctness COMMAND simd_correctness_test)

    if(DLP_ENABLE_CUDA)
        add_executable(cuda_correctness_test
            ${CUDA_TEST_SRC}
            src/core/SequentialRenderer.cpp
            ${CUDA_SRC}
        )
        target_include_directories(cuda_correctness_test PRIVATE
            ${CMAKE_SOURCE_DIR}/shared/include
            ${CMAKE_CURRENT_SOURCE_DIR}/include
        )
        set_source_files_properties(${CUDA_SRC} ${CUDA_TEST_SRC} PROPERTIES
            COMPILE_OPTIONS "--gpu-architecture=sm_${DLP_CUDA_ARCHITECTURE};--fmad=false"
        )
        set_target_properties(cuda_correctness_test PROPERTIES
            CUDA_STANDARD 17
            CUDA_STANDARD_REQUIRED YES
        )
        add_test(NAME cuda_correctness COMMAND cuda_correctness_test)
        set_tests_properties(cuda_correctness PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endif()

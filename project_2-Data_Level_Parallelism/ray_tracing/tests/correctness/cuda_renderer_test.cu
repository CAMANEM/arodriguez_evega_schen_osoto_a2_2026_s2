#include "config/raytracing_config.hpp"
#include "core/strategies/CudaRenderer.h"
#include "core/strategies/SequentialRenderer.h"

#include <cuda_runtime.h>

#include <iostream>
#include <vector>

int main() {
    int device_count = 0;
    const cudaError_t device_status = cudaGetDeviceCount(&device_count);
    if (device_status == cudaErrorNoDevice ||
        device_status == cudaErrorInsufficientDriver ||
        (device_status == cudaSuccess && device_count == 0)) {
        cudaGetLastError();
        std::cout << "CUDA correctness test skipped: no usable CUDA device.\n";
        return 77;
    }
    if (device_status != cudaSuccess) {
        std::cerr << "cudaGetDeviceCount failed: "
                  << cudaGetErrorString(device_status) << '\n';
        return 1;
    }

    constants::set_image_dimensions(17, 19);
    SequentialRenderer sequential;
    CudaRenderer cuda;
    sequential.set_camera_pos(constants::CAMERA_ORIGIN);
    cuda.set_camera_pos(constants::CAMERA_ORIGIN);

    const std::vector<Vector3> expected = sequential.render_frame();
    const std::vector<Vector3> actual = cuda.render_frame();
    if (actual.size() != expected.size()) {
        std::cerr << "CUDA frame has an unexpected number of pixels.\n";
        return 1;
    }

    for (std::size_t index = 0; index < expected.size(); ++index) {
        if (actual[index].x != expected[index].x ||
            actual[index].y != expected[index].y ||
            actual[index].z != expected[index].z) {
            std::cerr << "CUDA frame differs at pixel (" << index % 17
                      << ", " << index / 17 << ").\n";
            return 1;
        }
    }

    std::cout << "CUDA frame matches the sequential renderer at 17x19.\n";
    return 0;
}

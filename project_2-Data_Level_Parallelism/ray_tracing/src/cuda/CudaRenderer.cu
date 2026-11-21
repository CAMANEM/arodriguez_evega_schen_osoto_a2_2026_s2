#include "core/strategies/CudaRenderer.h"

#include <cuda_runtime.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct DeviceCircle {
    double center_x;
    double center_y;
    double radius;
    double red;
    double green;
    double blue;
};

struct DeviceColor {
    double red;
    double green;
    double blue;
};

void check_cuda(cudaError_t status, const char* operation) {
    if (status != cudaSuccess) {
        throw std::runtime_error(std::string(operation) + ": " +
                                 cudaGetErrorString(status));
    }
}

template <typename T>
class DeviceBuffer {
public:
    explicit DeviceBuffer(std::size_t count) {
        check_cuda(cudaMalloc(reinterpret_cast<void**>(&data_), count * sizeof(T)),
                   "cudaMalloc");
    }

    ~DeviceBuffer() {
        if (data_ != nullptr) {
            cudaFree(data_);
        }
    }

    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;

    T* get() const { return data_; }

    void release() {
        if (data_ != nullptr) {
            T* allocation = data_;
            data_ = nullptr;
            check_cuda(cudaFree(allocation), "cudaFree");
        }
    }

private:
    T* data_ = nullptr;
};

__global__ void render_kernel(DeviceColor* frame,
                              const DeviceCircle* circles,
                              std::size_t circle_count,
                              int width,
                              int height,
                              double camera_x,
                              double camera_y,
                              double view_scale,
                              DeviceColor background) {
    const int x = static_cast<int>(blockIdx.x * blockDim.x + threadIdx.x);
    const int y = static_cast<int>(blockIdx.y * blockDim.y + threadIdx.y);
    if (x >= width || y >= height) {
        return;
    }

    const double aspect = static_cast<double>(width) / height;
    const double target_x = (2.0 * x / width - 1.0) * aspect * view_scale;
    const double target_y = (1.0 - 2.0 * y / height) * view_scale;

    double direction_x = target_x - camera_x;
    double direction_y = target_y - camera_y;
    const double direction_length =
        sqrt(direction_x * direction_x + direction_y * direction_y);
    direction_x /= direction_length;
    direction_y /= direction_length;

    double nearest_t = INFINITY;
    DeviceColor color = background;
    for (std::size_t index = 0; index < circle_count; ++index) {
        const DeviceCircle circle = circles[index];
        const double offset_x = camera_x - circle.center_x;
        const double offset_y = camera_y - circle.center_y;
        const double a = direction_x * direction_x + direction_y * direction_y;
        const double b = 2.0 * (offset_x * direction_x + offset_y * direction_y);
        const double c = offset_x * offset_x + offset_y * offset_y
                       - circle.radius * circle.radius;
        const double discriminant = b * b - 4.0 * a * c;
        if (discriminant < 0.0) {
            continue;
        }

        const double t = (-b - sqrt(discriminant)) / (2.0 * a);
        if (t > 0.0 && t < nearest_t) {
            nearest_t = t;
            color = {circle.red, circle.green, circle.blue};
        }
    }

    const std::size_t pixel_index =
        static_cast<std::size_t>(y) * static_cast<std::size_t>(width)
        + static_cast<std::size_t>(x);
    frame[pixel_index] = color;
}

} // namespace

void CudaRenderer::set_camera_pos(const Vector2& pos) {
    IRenderer::set_camera_pos(pos);
    camera_pos_ = pos;
}

std::vector<Vector3> CudaRenderer::render_frame() {
    const int width = constants::IMAGE_WIDTH;
    const int height = constants::IMAGE_HEIGHT;
    const std::size_t pixel_count =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);

    std::vector<DeviceCircle> host_circles;
    host_circles.reserve(constants::CIRCLES.size());
    for (std::size_t index = 0; index < constants::CIRCLES.size(); ++index) {
        const auto& circle = constants::CIRCLES[index];
        host_circles.push_back({
            circle.center.x, circle.center.y, circle.radius,
            circle.color.x, circle.color.y, circle.color.z
        });
    }

    DeviceBuffer<DeviceCircle> device_circles(host_circles.size());
    DeviceBuffer<DeviceColor> device_frame(pixel_count);
    check_cuda(cudaMemcpy(device_circles.get(), host_circles.data(),
                         host_circles.size() * sizeof(DeviceCircle),
                         cudaMemcpyHostToDevice),
               "cudaMemcpy scene to device");

    constexpr unsigned int block_width = 16;
    constexpr unsigned int block_height = 16;
    const dim3 block(block_width, block_height);
    const dim3 grid(
        (static_cast<unsigned int>(width) + block_width - 1) / block_width,
        (static_cast<unsigned int>(height) + block_height - 1) / block_height);
    const DeviceColor background{
        constants::BACKGROUND_COLOR.x,
        constants::BACKGROUND_COLOR.y,
        constants::BACKGROUND_COLOR.z
    };

    render_kernel<<<grid, block>>>(
        device_frame.get(), device_circles.get(), host_circles.size(),
        width, height, camera_pos_.x, camera_pos_.y, constants::VIEW_SCALE,
        background);
    check_cuda(cudaGetLastError(), "launch render_kernel");
    check_cuda(cudaDeviceSynchronize(), "synchronize render_kernel");

    std::vector<DeviceColor> device_colors(pixel_count);
    check_cuda(cudaMemcpy(device_colors.data(), device_frame.get(),
                          pixel_count * sizeof(DeviceColor),
                          cudaMemcpyDeviceToHost),
               "cudaMemcpy frame to host");

    std::vector<Vector3> frame;
    frame.reserve(pixel_count);
    for (const DeviceColor& color : device_colors) {
        frame.emplace_back(color.red, color.green, color.blue);
    }

    device_frame.release();
    device_circles.release();
    return frame;
}

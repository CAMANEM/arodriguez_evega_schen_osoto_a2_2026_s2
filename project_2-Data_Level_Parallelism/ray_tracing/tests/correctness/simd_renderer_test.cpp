#include "config/raytracing_config.hpp"
#include "core/strategies/SequentialRenderer.h"
#include "core/strategies/SimdRenderer.h"

#include <array>
#include <iostream>
#include <vector>

int main() {
    std::vector<std::array<int, 2>> dimensions;
    for (int width = 1; width <= 33; ++width)
        dimensions.push_back({{width, 1 + (width * 7) % 19}});
    dimensions.insert(dimensions.end(), {{{1, 64}}, {{64, 1}}, {{64, 37}}, {{100, 100}}});

    const std::array<Vector2, 6> cameras = {{
        constants::CAMERA_ORIGIN,
        {1.25, 2.0},
        {-5.5, 6.0},
        {5.5, 6.0},
        {0.0, -12.0},
        {-2.25, 15.0}
    }};
    std::size_t cases_checked = 0;

    for (const auto& size : dimensions) {
        constants::set_image_dimensions(size[0], size[1]);
        for (const Vector2& camera : cameras) {
            SequentialRenderer sequential;
            SimdRenderer simd;
            sequential.set_camera_pos(camera);
            simd.set_camera_pos(camera);
            const auto expected = sequential.render_frame();
            const auto actual = simd.render_frame();

            if (actual.size() != expected.size()) {
                std::cerr << "frame size differs at " << size[0] << 'x' << size[1]
                          << " for camera (" << camera.x << ", " << camera.y << ")\n";
                return 1;
            }
            for (std::size_t i = 0; i < expected.size(); ++i) {
                if (actual[i].x != expected[i].x ||
                    actual[i].y != expected[i].y ||
                    actual[i].z != expected[i].z) {
                    std::cerr << "pixel (" << i % size[0] << ", " << i / size[0]
                              << ") differs at " << size[0] << 'x' << size[1]
                              << " for camera (" << camera.x << ", " << camera.y << ")\n";
                    return 1;
                }
            }
            ++cases_checked;
        }
    }

    std::cout << "SIMD frames match sequential frames across "
              << cases_checked << " resolution/camera cases.\n";
    return 0;
}

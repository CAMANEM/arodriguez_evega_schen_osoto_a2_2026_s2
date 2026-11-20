#include "config/raytracing_config.hpp"
#include "core/strategies/SequentialRenderer.h"
#include "core/strategies/SimdRenderer.h"

#include <array>
#include <iostream>

int main() {
    const std::array<std::array<int, 2>, 6> dimensions = {{
        {{1, 1}}, {{3, 2}}, {{4, 3}}, {{5, 4}}, {{9, 3}}, {{17, 11}}
    }};
    const std::array<Vector3, 2> cameras = {{
        constants::CAMERA_ORIGIN, {1.25, 2.0, 9.0}
    }};

    for (const auto& size : dimensions) {
        constants::set_image_dimensions(size[0], size[1]);
        for (const Vector3& camera : cameras) {
            SequentialRenderer sequential;
            SimdRenderer simd;
            sequential.set_camera_pos(camera);
            simd.set_camera_pos(camera);
            const auto expected = sequential.render_frame();
            const auto actual = simd.render_frame();

            if (actual.size() != expected.size()) {
                std::cerr << "frame size differs at " << size[0] << 'x' << size[1] << '\n';
                return 1;
            }
            for (std::size_t i = 0; i < expected.size(); ++i) {
                if (actual[i].x != expected[i].x ||
                    actual[i].y != expected[i].y ||
                    actual[i].z != expected[i].z) {
                    std::cerr << "pixel " << i << " differs at "
                              << size[0] << 'x' << size[1] << '\n';
                    return 1;
                }
            }
        }
    }

    std::cout << "SIMD frames match sequential frames.\n";
    return 0;
}

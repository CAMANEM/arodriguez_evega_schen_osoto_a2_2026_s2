#include "core/strategies/SequentialRenderer.h"
#include "config/raytracing_config.hpp"

/** @brief Bucle puro row-major sin paralelismo: referencia de correctitud y tiempo. */
std::vector<Vector3> SequentialRenderer::render_frame() {
    using namespace constants;
    std::vector<Vector3> frame(IMAGE_WIDTH * IMAGE_HEIGHT);
    for (int y = 0; y < IMAGE_HEIGHT; ++y)
        for (int x = 0; x < IMAGE_WIDTH; ++x)
            frame[y * IMAGE_WIDTH + x] = pixel_kernel_.compute(x, y);
    return frame;
}

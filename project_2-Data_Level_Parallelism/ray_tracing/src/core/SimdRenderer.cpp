#include "core/strategies/SimdRenderer.h"
#include "config/raytracing_config.hpp"

#include <array>
#include <immintrin.h>

namespace {

std::array<Vector3, 4> trace_four(const Scene& scene,
                                  const Vector2& origin,
                                  int x,
                                  int y,
                                  double aspect,
                                  double view_scale) {
    using namespace constants;

    const __m256d target_x = _mm256_setr_pd(
        (2.0 * x / IMAGE_WIDTH - 1.0) * aspect * view_scale,
        (2.0 * (x + 1) / IMAGE_WIDTH - 1.0) * aspect * view_scale,
        (2.0 * (x + 2) / IMAGE_WIDTH - 1.0) * aspect * view_scale,
        (2.0 * (x + 3) / IMAGE_WIDTH - 1.0) * aspect * view_scale);
    const double target_y = (1.0 - 2.0 * y / IMAGE_HEIGHT) * view_scale;

    __m256d direction_x = _mm256_sub_pd(target_x, _mm256_set1_pd(origin.x));
    __m256d direction_y =
        _mm256_sub_pd(_mm256_set1_pd(target_y), _mm256_set1_pd(origin.y));

    const __m256d length_squared = _mm256_add_pd(
        _mm256_mul_pd(direction_x, direction_x),
        _mm256_mul_pd(direction_y, direction_y));
    const __m256d length = _mm256_sqrt_pd(length_squared);
    direction_x = _mm256_div_pd(direction_x, length);
    direction_y = _mm256_div_pd(direction_y, length);

    const __m256d zero = _mm256_setzero_pd();
    const __m256d two = _mm256_set1_pd(2.0);
    const __m256d four = _mm256_set1_pd(4.0);
    __m256d nearest_t = _mm256_set1_pd(INFINITY);
    __m256d color_x = _mm256_set1_pd(BACKGROUND_COLOR.x);
    __m256d color_y = _mm256_set1_pd(BACKGROUND_COLOR.y);
    __m256d color_z = _mm256_set1_pd(BACKGROUND_COLOR.z);

    for (const Circle& circle : scene.circles) {
        const double oc_x = origin.x - circle.center.x;
        const double oc_y = origin.y - circle.center.y;
        const __m256d offset_dot_direction = _mm256_add_pd(
            _mm256_mul_pd(_mm256_set1_pd(oc_x), direction_x),
            _mm256_mul_pd(_mm256_set1_pd(oc_y), direction_y));
        const __m256d a = _mm256_add_pd(
            _mm256_mul_pd(direction_x, direction_x),
            _mm256_mul_pd(direction_y, direction_y));
        const __m256d b = _mm256_mul_pd(two, offset_dot_direction);
        const double c = oc_x * oc_x + oc_y * oc_y
                       - circle.radius * circle.radius;
        const __m256d discriminant = _mm256_sub_pd(
            _mm256_mul_pd(b, b),
            _mm256_mul_pd(_mm256_mul_pd(four, a), _mm256_set1_pd(c)));
        const __m256d safe_discriminant = _mm256_max_pd(discriminant, zero);
        const __m256d t = _mm256_div_pd(
            _mm256_sub_pd(_mm256_sub_pd(zero, b),
                          _mm256_sqrt_pd(safe_discriminant)),
            _mm256_mul_pd(two, a));
        const __m256d hit = _mm256_and_pd(
            _mm256_and_pd(
                _mm256_cmp_pd(discriminant, zero, _CMP_GE_OQ),
                _mm256_cmp_pd(t, zero, _CMP_GT_OQ)),
            _mm256_cmp_pd(t, nearest_t, _CMP_LT_OQ));

        nearest_t = _mm256_blendv_pd(nearest_t, t, hit);
        color_x = _mm256_blendv_pd(color_x, _mm256_set1_pd(circle.color.x), hit);
        color_y = _mm256_blendv_pd(color_y, _mm256_set1_pd(circle.color.y), hit);
        color_z = _mm256_blendv_pd(color_z, _mm256_set1_pd(circle.color.z), hit);
    }

    std::array<double, 4> red;
    std::array<double, 4> green;
    std::array<double, 4> blue;
    _mm256_storeu_pd(red.data(), color_x);
    _mm256_storeu_pd(green.data(), color_y);
    _mm256_storeu_pd(blue.data(), color_z);

    return {{{red[0], green[0], blue[0]},
             {red[1], green[1], blue[1]},
             {red[2], green[2], blue[2]},
             {red[3], green[3], blue[3]}}};
}

} // namespace

void SimdRenderer::set_camera_pos(const Vector2& pos) {
    IRenderer::set_camera_pos(pos);
    camera_pos_ = pos;
}

std::vector<Vector3> SimdRenderer::render_frame() {
    using namespace constants;

    const double aspect = static_cast<double>(IMAGE_WIDTH) / IMAGE_HEIGHT;
    std::vector<Vector3> frame(IMAGE_WIDTH * IMAGE_HEIGHT);

    for (int y = 0; y < IMAGE_HEIGHT; ++y) {
        int x = 0;
        for (; x + 3 < IMAGE_WIDTH; x += 4) {
            const auto colors = trace_four(scene_, camera_pos_, x, y, aspect, VIEW_SCALE);
            for (int lane = 0; lane < 4; ++lane)
                frame[y * IMAGE_WIDTH + x + lane] = colors[lane];
        }
        for (; x < IMAGE_WIDTH; ++x)
            frame[y * IMAGE_WIDTH + x] = pixel_kernel_.compute(x, y);
    }
    return frame;
}

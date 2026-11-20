#include "core/strategies/SimdRenderer.h"
#include "config/raytracing_config.hpp"

#include <array>
#include <immintrin.h>

namespace {

/**
 * Traza cuatro píxeles de una fila simultáneamente; cada carril AVX2 representa
 * un rayo independiente y el resultado conserva el orden de los píxeles.
 */
std::array<Vector3, 4> trace_four(const Scene& scene,
                                  const Vector3& origin,
                                  int x,
                                  int y,
                                  double aspect,
                                  const Vector3& forward,
                                  const Vector3& right,
                                  const Vector3& up) {
    using namespace constants;

    // Coordenadas de cámara para los cuatro píxeles contiguos.
    const __m256d u = _mm256_setr_pd(
        (2.0 * x / IMAGE_WIDTH) - 1.0,
        (2.0 * (x + 1) / IMAGE_WIDTH) - 1.0,
        (2.0 * (x + 2) / IMAGE_WIDTH) - 1.0,
        (2.0 * (x + 3) / IMAGE_WIDTH) - 1.0);
    const double v = 1.0 - (2.0 * y / IMAGE_HEIGHT);
    const __m256d horizontal = _mm256_mul_pd(u, _mm256_set1_pd(aspect));
    const __m256d vertical = _mm256_set1_pd(v);

    // Construye y normaliza en paralelo las direcciones de los cuatro rayos.
    __m256d direction_x = _mm256_add_pd(
        _mm256_add_pd(_mm256_set1_pd(forward.x),
                      _mm256_mul_pd(_mm256_set1_pd(right.x), horizontal)),
        _mm256_mul_pd(_mm256_set1_pd(up.x), vertical));
    __m256d direction_y = _mm256_add_pd(
        _mm256_add_pd(_mm256_set1_pd(forward.y),
                      _mm256_mul_pd(_mm256_set1_pd(right.y), horizontal)),
        _mm256_mul_pd(_mm256_set1_pd(up.y), vertical));
    __m256d direction_z = _mm256_add_pd(
        _mm256_add_pd(_mm256_set1_pd(forward.z),
                      _mm256_mul_pd(_mm256_set1_pd(right.z), horizontal)),
        _mm256_mul_pd(_mm256_set1_pd(up.z), vertical));

    const __m256d length_squared = _mm256_add_pd(
        _mm256_add_pd(_mm256_mul_pd(direction_x, direction_x),
                      _mm256_mul_pd(direction_y, direction_y)),
        _mm256_mul_pd(direction_z, direction_z));
    const __m256d length = _mm256_sqrt_pd(length_squared);
    direction_x = _mm256_div_pd(direction_x, length);
    direction_y = _mm256_div_pd(direction_y, length);
    direction_z = _mm256_div_pd(direction_z, length);

    const __m256d zero = _mm256_setzero_pd();
    const __m256d two = _mm256_set1_pd(2.0);
    const __m256d four = _mm256_set1_pd(4.0);
    __m256d nearest_t = _mm256_set1_pd(INFINITY);
    __m256d color_x = _mm256_set1_pd(BACKGROUND_COLOR.x);
    __m256d color_y = _mm256_set1_pd(BACKGROUND_COLOR.y);
    __m256d color_z = _mm256_set1_pd(BACKGROUND_COLOR.z);

    // Las propiedades de cada esfera son escalares; se aplican a los cuatro rayos.
    for (const Sphere& sphere : scene.spheres) {
        const double oc_x = origin.x - sphere.center.x;
        const double oc_y = origin.y - sphere.center.y;
        const double oc_z = origin.z - sphere.center.z;
        const __m256d offset_dot_direction = _mm256_add_pd(
            _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(oc_x), direction_x),
                          _mm256_mul_pd(_mm256_set1_pd(oc_y), direction_y)),
            _mm256_mul_pd(_mm256_set1_pd(oc_z), direction_z));
        const __m256d a = _mm256_add_pd(
            _mm256_add_pd(_mm256_mul_pd(direction_x, direction_x),
                          _mm256_mul_pd(direction_y, direction_y)),
            _mm256_mul_pd(direction_z, direction_z));
        const __m256d b = _mm256_mul_pd(two, offset_dot_direction);
        const double c = (oc_x * oc_x + oc_y * oc_y + oc_z * oc_z)
                       - sphere.radius * sphere.radius;
        const __m256d discriminant = _mm256_sub_pd(
            _mm256_mul_pd(b, b),
            _mm256_mul_pd(_mm256_mul_pd(four, a), _mm256_set1_pd(c)));
        const __m256d safe_discriminant = _mm256_max_pd(discriminant, zero);
        const __m256d t = _mm256_div_pd(
            _mm256_sub_pd(_mm256_sub_pd(zero, b),
                          _mm256_sqrt_pd(safe_discriminant)),
            _mm256_mul_pd(two, a));
        // Actualiza solo los carriles con intersección positiva más cercana.
        const __m256d hit = _mm256_and_pd(
            _mm256_and_pd(
                _mm256_cmp_pd(discriminant, zero, _CMP_GE_OQ),
                _mm256_cmp_pd(t, zero, _CMP_GT_OQ)),
            _mm256_cmp_pd(t, nearest_t, _CMP_LT_OQ));

        nearest_t = _mm256_blendv_pd(nearest_t, t, hit);
        color_x = _mm256_blendv_pd(color_x, _mm256_set1_pd(sphere.color.x), hit);
        color_y = _mm256_blendv_pd(color_y, _mm256_set1_pd(sphere.color.y), hit);
        color_z = _mm256_blendv_pd(color_z, _mm256_set1_pd(sphere.color.z), hit);
    }

    // Extrae los cuatro colores del registro para escribirlos en el frame.
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

void SimdRenderer::set_camera_pos(const Vector3& pos) {
    IRenderer::set_camera_pos(pos);
    camera_pos_ = pos;
}

/** Renderiza la imagen en bloques AVX2 de cuatro píxeles, con sobrante escalar. */
std::vector<Vector3> SimdRenderer::render_frame() {
    using namespace constants;

    const Vector3 scene_center(SCENE_CENTER_X, SCENE_CENTER_Y, SCENE_CENTER_Z);
    const Vector3 forward = (scene_center - camera_pos_).normalize();
    const Vector3 right = forward.cross(CAMERA_UP).normalize();
    const Vector3 up = right.cross(forward).normalize();
    const double aspect = static_cast<double>(IMAGE_WIDTH) / IMAGE_HEIGHT;

    std::vector<Vector3> frame(IMAGE_WIDTH * IMAGE_HEIGHT);
    for (int y = 0; y < IMAGE_HEIGHT; ++y) {
        int x = 0;
        // Cada llamada calcula cuatro píxeles contiguos en paralelo.
        for (; x + 3 < IMAGE_WIDTH; x += 4) {
            const auto colors = trace_four(scene_, camera_pos_, x, y, aspect,
                                            forward, right, up);
            for (int lane = 0; lane < 4; ++lane)
                frame[y * IMAGE_WIDTH + x + lane] = colors[lane];
        }
        // Procesa los últimos 0-3 píxeles que no completan un bloque AVX2.
        for (; x < IMAGE_WIDTH; ++x)
            frame[y * IMAGE_WIDTH + x] = pixel_kernel_.compute(x, y);
    }
    return frame;
}

#pragma once
#include "geometry/Vector2.h"
#include "config/raytracing_config.hpp"

/** @brief Rayo 2D con origen y dirección unitaria. */
struct Ray {
    Vector2 origin;
    Vector2 direction;

    Ray(const Vector2& o, const Vector2& d) : origin(o), direction(d.normalize()) {}
};

/**
 * @brief Proyecta el píxel (px, py) como rayo 2D usando una cámara perspectiva look-at.
 *
 * px y py se mapean a un punto objetivo en el plano de vista 2D centrado en la escena.
 * La dirección del rayo se calcula desde cam_pos hacia ese punto, produciendo
 * N = width × height rayos 2D completamente independientes.
 */
inline Ray make_ray(int px, int py, const Vector2& cam_pos) {
    using namespace constants;
    const double aspect = static_cast<double>(IMAGE_WIDTH) / IMAGE_HEIGHT;

    const double tx = (2.0 * px / IMAGE_WIDTH  - 1.0) * aspect * VIEW_SCALE;
    const double ty = (1.0 - 2.0 * py / IMAGE_HEIGHT)          * VIEW_SCALE;

    return Ray(cam_pos, Vector2(tx, ty) - cam_pos);
}

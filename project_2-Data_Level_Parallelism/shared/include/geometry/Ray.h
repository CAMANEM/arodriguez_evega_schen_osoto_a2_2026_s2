#pragma once
#include "geometry/Vector3.h"
#include "config/raytracing_config.hpp"
#include <cmath>

/** @brief Rayo 3D con origen y dirección unitaria. */
struct Ray {
    Vector3 origin;
    Vector3 direction;

    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d.normalize()) {}
};

/**
 * @brief Proyecta el píxel (x, y) como rayo usando una cámara look-at.
 *
 * La variable paralelizable del benchmark es el conjunto de píxeles: cada llamada
 * a make_ray genera exactamente un rayo independiente de los demás.
 */
inline Ray make_ray(int x, int y, const Vector3& cam_pos) {
    using namespace constants;
    const Vector3 scene_center(SCENE_CENTER_X, SCENE_CENTER_Y, SCENE_CENTER_Z);

    const Vector3 fwd   = (scene_center - cam_pos).normalize();
    const Vector3 right = fwd.cross(CAMERA_UP).normalize();
    const Vector3 up    = right.cross(fwd).normalize();

    const double u      = (2.0 * x / IMAGE_WIDTH)  - 1.0;
    const double v      = 1.0 - (2.0 * y / IMAGE_HEIGHT);
    const double aspect = static_cast<double>(IMAGE_WIDTH) / IMAGE_HEIGHT;

    return Ray(cam_pos, fwd + right * (u * aspect) + up * v);
}

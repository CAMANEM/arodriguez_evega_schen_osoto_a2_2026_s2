/**
 * @file PixelKernel.h
 * @brief Cálculo de color por píxel independiente del modelo de ejecución.
 */
#ifndef PIXEL_KERNEL_H
#define PIXEL_KERNEL_H

#include "core/geometry/Ray.h"
#include "core/geometry/Scene.h"

/** @brief Calcula por ray tracing el color de un píxel, sin conocer workers ni schedulers. */
class PixelKernel {
public:
    /** @brief Actualiza la cámara usada para proyectar rayos. */
    void set_camera_pos(const Vector3& camera_pos) { camera_pos_ = camera_pos; }

    /** @brief Calcula el color del píxel solicitado. */
    Vector3 compute(int x, int y) const {
        return scene_.trace(make_ray(x, y, camera_pos_));
    }

private:
    Scene scene_;
    Vector3 camera_pos_;
};

#endif // PIXEL_KERNEL_H
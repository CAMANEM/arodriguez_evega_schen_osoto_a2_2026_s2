#pragma once
#include "geometry/Ray.h"
#include "geometry/Scene.h"

/**
 * @brief Núcleo de cómputo por píxel: construye el rayo y lo traza contra la escena.
 *
 * Encapsula la escena y la posición de cámara de forma que todos los renderers
 * (Sequential, SIMD, CUDA host, FPGA host) usen exactamente la misma lógica de rayo.
 */
class PixelKernel {
public:
    void    set_camera_pos(const Vector3& pos) { camera_pos_ = pos; }
    Vector3 compute(int x, int y) const { return scene_.trace(make_ray(x, y, camera_pos_)); }

private:
    Scene   scene_;
    Vector3 camera_pos_;
};

/**
 * @file Ray.h
 * @brief Rayos normalizados y proyección de coordenadas de píxel a escena.
 */
#ifndef RAY_H
#define RAY_H

#include "Vector3.h"
#include "raytracing_config.hpp"
#include <cmath>

/** @brief Rayo 3D definido por un origen y una dirección unitaria. */
struct Ray {
    Vector3 origin;      // Punto donde inicia el rayo
    Vector3 direction;   // Dirección del rayo (normalizada)

    /**
     * @brief Construye un rayo y normaliza su dirección.
     * @param o Origen del rayo.
     * @param d Dirección antes de normalizar.
     */
    Ray(const Vector3& o, const Vector3& d) : origin(o), direction(d.normalize()) {}
};

/**
 * @brief Proyecta un píxel desde la cámara inicial al espacio de la escena.
 * @param x Coordenada horizontal del píxel.
 * @param y Coordenada vertical del píxel.
 * @return Rayo normalizado con corrección de relación de aspecto.
 */
inline Ray make_ray(int x, int y) {
    double u      = (2.0 * x / constants::IMAGE_WIDTH)  - 1.0;
    double v      = 1.0 - (2.0 * y / constants::IMAGE_HEIGHT);
    double aspect = static_cast<double>(constants::IMAGE_WIDTH) / constants::IMAGE_HEIGHT;
    Vector3 origin = constants::CAMERA_ORIGIN;
    Vector3 direction(u * aspect, v, -1);
    return Ray(origin, direction);
}

/**
 * @brief Proyecta un píxel usando una cámara orientada al centro de la escena.
 * @param x Coordenada horizontal del píxel.
 * @param y Coordenada vertical del píxel.
 * @param cam_pos Posición actual de la cámara.
 * @return Rayo normalizado construido con la base look-at de la cámara.
 * @note En la posición inicial genera la misma proyección que la sobrecarga simple.
 */
inline Ray make_ray(int x, int y, const Vector3& cam_pos) {
    using namespace constants;
    Vector3 scene_center(SCENE_CENTER_X, SCENE_CENTER_Y, SCENE_CENTER_Z);
    Vector3 up_world = CAMERA_UP;

    // Base ortonormal de la cámara: forward, right, up
    Vector3 fwd   = (scene_center - cam_pos).normalize();
    Vector3 right = fwd.cross(up_world).normalize();
    Vector3 up    = right.cross(fwd).normalize();

    double u      = (2.0 * x / IMAGE_WIDTH)  - 1.0;
    double v      = 1.0 - (2.0 * y / IMAGE_HEIGHT);
    double aspect = static_cast<double>(IMAGE_WIDTH) / IMAGE_HEIGHT;

    Vector3 direction = fwd + right * (u * aspect) + up * v;
    return Ray(cam_pos, direction);
}

#endif // RAY_H
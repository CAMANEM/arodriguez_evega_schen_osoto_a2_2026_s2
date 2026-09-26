/**
 * @file Workload.h
 * @brief Selección del cálculo por píxel y función común de evaluación.
 */
#ifndef WORKLOAD_H
#define WORKLOAD_H

#include "raytracing_config.hpp"
#include "Ray.h"
#include "Scene.h"

/** @brief Carga real de ray tracing o patrón sintético para el benchmark. */
enum class Workload {
    raytracing,
    dummy
};

/**
 * @brief Calcula el color de un píxel usando la carga seleccionada.
 * @param scene Geometría consultada por la carga raytracing.
 * @param x Coordenada horizontal del píxel.
 * @param y Coordenada vertical del píxel.
 * @param camera_pos Posición de cámara para proyectar el rayo.
 * @param workload Cálculo real o patrón dummy.
 * @return Color RGB del píxel.
 */
inline Vector3 compute_pixel(const Scene& scene, int x, int y,
                             const Vector3& camera_pos, Workload workload) {
    if (workload == Workload::raytracing)
        return scene.trace(make_ray(x, y, camera_pos));

    const double width = static_cast<double>(constants::IMAGE_WIDTH);
    const double height = static_cast<double>(constants::IMAGE_HEIGHT);
    return Vector3(x / width, y / height,
                   ((x + y) % constants::IMAGE_WIDTH) / width);
}

#endif
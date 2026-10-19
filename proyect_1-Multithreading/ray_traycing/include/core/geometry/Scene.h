/**
 * @file Scene.h
 * @brief Escena de ray tracing y búsqueda de intersecciones cercanas.
 */
#ifndef SCENE_H
#define SCENE_H

#include "core/geometry/Sphere.h"
#include "core/config/raytracing_config.hpp"
#include "core/geometry/Ray.h"
#include <cmath>
#include <vector>

/**
 * @brief Colección de esferas que define la geometría renderizable.
 * @note La búsqueda recorre todos los objetos, con coste O(n) por rayo.
 */
struct Scene {
    std::vector<Sphere> spheres;  // Lista de objetos en la escena

    Scene() {
        for (const auto& sphere : constants::SPHERES)
            spheres.emplace_back(sphere.center, sphere.radius, sphere.color);
    }

    /**
     * @brief Devuelve el color del objeto intersectado más cercano.
     * @param ray Rayo que se consulta.
     * @return Color RGB normalizado o BACKGROUND_COLOR si no hay intersección.
     */
    Vector3 trace(const Ray& ray) const {
        double t_min = INFINITY;
        Vector3 color = constants::BACKGROUND_COLOR;
        for (const auto& sphere : spheres) {
            double t;
            if (sphere.intersect(ray, t) && t < t_min) {
                t_min = t;
                color = sphere.color;
            }
        }
        return color;
    }
};
#endif // SCENE_H
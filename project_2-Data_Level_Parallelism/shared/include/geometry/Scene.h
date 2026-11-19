#pragma once
#include "geometry/Sphere.h"
#include "config/raytracing_config.hpp"
#include <cmath>
#include <vector>

/** @brief Colección de esferas; búsqueda de intersección más cercana O(n) por rayo. */
struct Scene {
    std::vector<Sphere> spheres;

    Scene() {
        for (const auto& s : constants::SPHERES)
            spheres.emplace_back(s.center, s.radius, s.color);
    }

    /** @return Color del objeto más cercano, o BACKGROUND_COLOR si no hay intersección. */
    Vector3 trace(const Ray& ray) const {
        double  t_min = INFINITY;
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

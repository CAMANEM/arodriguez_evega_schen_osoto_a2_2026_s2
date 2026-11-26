#pragma once
#include "geometry/Circle.h"
#include "config/raytracing_config.hpp"
#include <cmath>
#include <vector>

/** @brief Colección de círculos 2D; búsqueda de intersección más cercana O(n) por rayo. */
struct Scene {
    std::vector<Circle> circles;

    Scene() {
        for (const auto& c : constants::CIRCLES)
            circles.emplace_back(c.center, c.radius, c.color);
    }

    /** @return Color del círculo más cercano, o BACKGROUND_COLOR si no hay intersección. */
    Vector3 trace(const Ray& ray) const {
        double  t_min = INFINITY;
        Vector3 color = constants::BACKGROUND_COLOR;
        for (const auto& circle : circles) {
            double t;
            if (circle.intersect(ray, t) && t < t_min) {
                t_min = t;
                color = circle.color;
            }
        }
        return color;
    }
};

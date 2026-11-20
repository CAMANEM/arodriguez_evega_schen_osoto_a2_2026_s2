#pragma once
#include "geometry/Vector2.h"
#include "geometry/Vector3.h"
#include "geometry/Ray.h"
#include <cmath>

/** @brief Círculo 2D renderizable: intersección rayo-círculo por ecuación cuadrática 2D. */
struct Circle {
    Vector2 center;
    double  radius;
    Vector3 color;

    Circle(const Vector2& c, double r, const Vector3& col)
        : center(c), radius(r), color(col) {}

    /**
     * @brief Evalúa la raíz cercana de la intersección rayo-círculo en 2D.
     * @param t Distancia al punto de golpe; solo se modifica si hay intersección válida.
     * @return true si hay intersección delante del origen del rayo.
     */
    bool intersect(const Ray& ray, double& t) const {
        const Vector2 oc = ray.origin - center;
        const double a   = ray.direction.dot(ray.direction);
        const double b   = 2.0 * oc.dot(ray.direction);
        const double c   = oc.dot(oc) - radius * radius;
        const double discriminant = b*b - 4*a*c;
        if (discriminant < 0) return false;
        t = (-b - std::sqrt(discriminant)) / (2 * a);
        return t > 0;
    }

    /** @brief Normal 2D unitaria en el punto de golpe (radial desde el centro). */
    Vector2 normal_at(const Vector2& hit_point) const {
        return (hit_point - center) / radius;
    }
};

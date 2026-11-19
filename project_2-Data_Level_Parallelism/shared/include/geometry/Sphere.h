#pragma once
#include "geometry/Vector3.h"
#include "geometry/Ray.h"
#include <cmath>

/** @brief Esfera renderizable: intersección rayo-esfera por ecuación cuadrática. */
struct Sphere {
    Vector3 center;
    double  radius;
    Vector3 color;

    Sphere(const Vector3& c, double r, const Vector3& col)
        : center(c), radius(r), color(col) {}

    /**
     * @brief Evalúa la raíz cercana de la intersección.
     * @param t Distancia de la raíz; solo se modifica si es positiva.
     * @return true si hay intersección delante del origen.
     */
    bool intersect(const Ray& ray, double& t) const {
        const Vector3 oc = ray.origin - center;
        const double a = ray.direction.dot(ray.direction);
        const double b = 2.0 * oc.dot(ray.direction);
        const double c = oc.dot(oc) - radius * radius;
        const double discriminant = b*b - 4*a*c;
        if (discriminant < 0) return false;
        t = (-b - std::sqrt(discriminant)) / (2 * a);
        return t > 0;
    }
};

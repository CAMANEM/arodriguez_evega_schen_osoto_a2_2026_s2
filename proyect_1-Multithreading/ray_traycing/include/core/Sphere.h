/**
 * @file Sphere.h
 * @brief Geometría de una esfera y su intersección con rayos.
 */
#ifndef SPHERE_H
#define SPHERE_H

#include "Vector3.h"
#include "Ray.h"

/**
 * @brief Esfera renderizable con centro, radio y color RGB normalizado.
 *
 * La intersección se calcula resolviendo la ecuación cuadrática entre el rayo
 * y la superficie esférica.
 */
struct Sphere {
    Vector3 center;    // Centro de la esfera en coordenadas 3D
    double radius;     // Radio de la esfera
    Vector3 color;     // Color RGB (valores en [0, 1])

    /**
     * @brief Inicializa la geometría y el color de la esfera.
     * @param c Centro en coordenadas del mundo.
     * @param r Radio positivo.
     * @param col Color RGB normalizado en [0, 1].
     */
    Sphere(const Vector3& c, double r, const Vector3& col) : center(c), radius(r), color(col) {}

    /**
     * @brief Evalúa la raíz cercana de la intersección rayo-esfera.
     * @param ray Rayo con dirección normalizada.
     * @param t Distancia de la raíz cercana; solo se modifica si es positiva.
     * @return true si la raíz cercana está delante del origen.
     * @note Si el origen está dentro de la esfera, la raíz cercana es negativa y
     *       no se prueba la raíz lejana.
     */
    bool intersect(const Ray& ray, double& t) const {
        Vector3 oc = ray.origin - center;
        double a = ray.direction.dot(ray.direction);
        double b = 2.0 * oc.dot(ray.direction);
        double c = oc.dot(oc) - radius * radius;
        double discriminant = b * b - 4 * a * c;
        if (discriminant < 0) return false;
        t = (-b - sqrt(discriminant)) / (2 * a);
        return t > 0;
    }
};

#endif // SPHERE_H
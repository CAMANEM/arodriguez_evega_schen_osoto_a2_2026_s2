/**
 * @file Vector3.h
 * @brief Operaciones básicas para vectores, puntos y colores RGB 3D.
 */
#ifndef VECTOR3_H
#define VECTOR3_H

#include <cmath>
#include <iostream>

/** @brief Vector tridimensional usado también para puntos y colores RGB. */
struct Vector3 {
    double x, y, z;

    /** @brief Construye el vector cero. */
    Vector3() : x(0), y(0), z(0) {}
    /** @brief Construye un vector con componentes explícitas. */
    Vector3(double x, double y, double z) : x(x), y(y), z(z) {}

    /** @brief Suma componentes de dos vectores. */
    Vector3 operator+(const Vector3& v) const { return Vector3(x + v.x, y + v.y, z + v.z); }
    /** @brief Resta componentes de dos vectores. */
    Vector3 operator-(const Vector3& v) const { return Vector3(x - v.x, y - v.y, z - v.z); }
    
    /** @brief Multiplica el vector por un escalar. */
    Vector3 operator*(double s) const { return Vector3(x * s, y * s, z * s); }
    /** @brief Divide el vector por un escalar no nulo. */
    Vector3 operator/(double s) const { return Vector3(x / s, y / s, z / s); }
    
    /** @brief Calcula el producto punto. */
    double dot(const Vector3& v) const { return x * v.x + y * v.y + z * v.z; }
    
    /** @brief Calcula el producto cruz, perpendicular a ambos vectores. */
    Vector3 cross(const Vector3& v) const {
        return Vector3(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x);
    }
    
    /** @return Magnitud euclidiana del vector. */
    double length() const { return sqrt(dot(*this)); }
    
    /**
     * @return Vector unitario con la misma dirección.
     * @pre La magnitud debe ser distinta de cero.
     */
    Vector3 normalize() const { return *this / length(); }
};

#endif // VECTOR3_H
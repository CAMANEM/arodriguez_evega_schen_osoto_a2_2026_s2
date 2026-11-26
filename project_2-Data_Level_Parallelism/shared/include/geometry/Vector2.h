#pragma once
#include <cmath>

/** @brief Vector bidimensional para posiciones y direcciones en la escena 2D. */
struct Vector2 {
    double x, y;

    Vector2() : x(0), y(0) {}
    Vector2(double x, double y) : x(x), y(y) {}

    Vector2 operator+(const Vector2& v) const { return {x + v.x, y + v.y}; }
    Vector2 operator-(const Vector2& v) const { return {x - v.x, y - v.y}; }
    Vector2 operator*(double s)         const { return {x * s,   y * s  }; }
    Vector2 operator/(double s)         const { return {x / s,   y / s  }; }

    double  dot(const Vector2& v) const { return x*v.x + y*v.y; }
    double  length()              const { return std::sqrt(dot(*this)); }
    Vector2 normalize()           const { return *this / length(); }

    /** @brief Perpendicular 90° en sentido antihorario: (-y, x). */
    Vector2 perp() const { return {-y, x}; }
};

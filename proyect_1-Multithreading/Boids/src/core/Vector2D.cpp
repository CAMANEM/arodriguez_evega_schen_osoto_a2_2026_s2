#include "core/Vector2D.hpp"

#include <cmath>

Vector2D::Vector2D(double x, double y) : x_(x), y_(y) {
}

double Vector2D::getX() const {
    return x_;
}

double Vector2D::getY() const {
    return y_;
}

Vector2D Vector2D::operator+(const Vector2D& other) const {
    return Vector2D(x_ + other.x_, y_ + other.y_);
}

Vector2D Vector2D::operator-(const Vector2D& other) const {
    return Vector2D(x_ - other.x_, y_ - other.y_);
}

Vector2D Vector2D::operator*(double scalar) const {
    return Vector2D(x_ * scalar, y_ * scalar);
}

Vector2D& Vector2D::operator+=(const Vector2D& other) {
    x_ += other.x_;
    y_ += other.y_;
    return *this;
}

double Vector2D::magnitude() const {
    return std::sqrt(x_ * x_ + y_ * y_);
}

Vector2D Vector2D::normalized() const {
    const double length = magnitude();
    if (length < 1e-9) {
        return Vector2D(0.0, 0.0);
    }
    return Vector2D(x_ / length, y_ / length);
}

Vector2D Vector2D::limited(double maxMagnitude) const {
    const double length = magnitude();
    if (length <= maxMagnitude || length < 1e-9) {
        return *this;
    }
    return normalized() * maxMagnitude;
}

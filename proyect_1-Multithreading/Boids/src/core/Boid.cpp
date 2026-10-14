#include "core/Boid.hpp"

#include <cmath>

namespace {

/**
 * @brief Envuelve un valor dentro del rango [0, limit), simulando un mundo
 *        toroidal (lo que sale por un borde reaparece por el opuesto).
 */
double wrapCoordinate(double value, double limit) {
    double wrapped = std::fmod(value, limit);
    if (wrapped < 0.0) {
        wrapped += limit;
    }
    return wrapped;
}

} // namespace

Boid::Boid(int id, const Vector2D& position, const Vector2D& velocity, double mass)
    : object_interface(id, mass) {
    set_position(position.getX(), position.getY());
    set_velocity(velocity.getX(), velocity.getY());
}

Vector2D Boid::getPosition() const {
    return Vector2D(pos_x, pos_y);
}

Vector2D Boid::getVelocity() const {
    return Vector2D(speed_x, speed_y);
}

void Boid::update(double dt) {
    if (dt <= 0.0 || mass <= 0.0) {
        return;
    }

    acc_x = force_x / mass;
    acc_y = force_y / mass;
    speed_x += acc_x * dt;
    speed_y += acc_y * dt;
    pos_x += speed_x * dt;
    pos_y += speed_y * dt;
}

void Boid::reset() {
    set_position(0.0, 0.0);
    set_velocity(0.0, 0.0);
    set_acceleration(0.0, 0.0);
    set_force(0.0, 0.0);
}

void Boid::integrate(const Vector2D& steeringForce, const FlockingConfig& config) {
    const Vector2D previousPosition = getPosition();
    set_force(steeringForce.getX(), steeringForce.getY());
    update(config.getDeltaTime());

    const Vector2D nextVelocity = getVelocity().limited(config.getMaxSpeed());
    set_velocity(nextVelocity.getX(), nextVelocity.getY());

    const Vector2D nextPosition =
        previousPosition + nextVelocity * config.getDeltaTime();
    set_position(wrapCoordinate(nextPosition.getX(), config.getWorldWidth()),
                 wrapCoordinate(nextPosition.getY(), config.getWorldHeight()));
}

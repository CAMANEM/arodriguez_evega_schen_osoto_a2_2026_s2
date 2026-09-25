#include "core/SphParticle.hpp"

SphParticle::SphParticle(int id, double mass, double x, double y,
                         double velocityX, double velocityY)
    : object_interface(id, mass),
      initialPosX_(x),
      initialPosY_(y),
      initialSpeedX_(velocityX),
      initialSpeedY_(velocityY),
      density_(0.0),
      pressure_(0.0) {
    set_position(x, y);
    set_velocity(velocityX, velocityY);
}

void SphParticle::update(double dt) {
    set_acceleration(get_force_x() / get_mass(), get_force_y() / get_mass());
    set_velocity(get_speed_x() + get_acc_x() * dt,
                 get_speed_y() + get_acc_y() * dt);
    set_position(get_pos_x() + get_speed_x() * dt,
                 get_pos_y() + get_speed_y() * dt);
}

void SphParticle::reset() {
    set_position(initialPosX_, initialPosY_);
    set_velocity(initialSpeedX_, initialSpeedY_);
    set_acceleration(0.0, 0.0);
    set_force(0.0, 0.0);
    density_ = 0.0;
    pressure_ = 0.0;
}

double SphParticle::getDensity() const { return density_; }
double SphParticle::getPressure() const { return pressure_; }
void SphParticle::setDensity(double density) { density_ = density; }
void SphParticle::setPressure(double pressure) { pressure_ = pressure; }

#include "../include/body.hpp"
#include <random>

/* ===== CONSTRUCTOR =========================================================================== */
Body::Body(int id, double mass) : object_interface(id, mass) {
    reset();
}

/* ===== OTHER METHODS ========================================================================= */
void Body::update(double dt) {
    // a = F / m
    acc_x = force_x / mass;
    acc_y = force_y / mass;

    // v += a * dt
    speed_x += acc_x * dt;
    speed_y += acc_y * dt;

    // pos += v * dt
    pos_x += speed_x * dt;
    pos_y += speed_y * dt;

    // Border bouncing: flip velocity and acceleration on contact
    if (pos_x <= 0.0) {
        pos_x = 0.0;
        speed_x *= -1.0;
        acc_x   *= -1.0;
    } else if (pos_x >= static_cast<double>(SCREEN_W)) {
        pos_x = static_cast<double>(SCREEN_W);
        speed_x *= -1.0;
        acc_x   *= -1.0;
    }

    if (pos_y <= 0.0) {
        pos_y = 0.0;
        speed_y *= -1.0;
        acc_y   *= -1.0;
    } else if (pos_y >= static_cast<double>(SCREEN_H)) {
        pos_y = static_cast<double>(SCREEN_H);
        speed_y *= -1.0;
        acc_y   *= -1.0;
    }
}

void Body::reset() {
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_real_distribution<double> dist_x(0.0, static_cast<double>(SCREEN_W));
    std::uniform_real_distribution<double> dist_y(0.0, static_cast<double>(SCREEN_H));

    pos_x   = dist_x(rng);
    pos_y   = dist_y(rng);
    speed_x = 0.0;
    speed_y = 0.0;
    acc_x   = 0.0;
    acc_y   = 0.0;
    force_x = 0.0;
    force_y = 0.0;
}

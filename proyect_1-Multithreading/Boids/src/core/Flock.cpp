#include "core/Flock.hpp"

#include <random>

Flock::Flock(const FlockingConfig& config, unsigned int seed) {
    std::mt19937 randomEngine(seed);
    std::uniform_real_distribution<double> positionXDistribution(0.0, config.getWorldWidth());
    std::uniform_real_distribution<double> positionYDistribution(0.0, config.getWorldHeight());
    std::uniform_real_distribution<double> velocityDistribution(-1.0, 1.0);

    boids_.reserve(config.getBoidCount());
    for (int i = 0; i < config.getBoidCount(); ++i) {
        const Vector2D position(positionXDistribution(randomEngine), positionYDistribution(randomEngine));
        const Vector2D velocity(velocityDistribution(randomEngine), velocityDistribution(randomEngine));
        boids_.emplace_back(i, position, velocity);
    }
}

int Flock::getBoidCount() const {
    return static_cast<int>(boids_.size());
}

const Boid& Flock::getBoid(int index) const {
    return boids_[index];
}

void Flock::applyIntegration(int index, const Vector2D& steeringForce, const FlockingConfig& config) {
    boids_[index].integrate(steeringForce, config);
}

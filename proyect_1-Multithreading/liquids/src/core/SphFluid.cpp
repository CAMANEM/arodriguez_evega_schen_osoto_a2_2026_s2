#include "core/SphFluid.hpp"

#include <random>

SphFluid::SphFluid(const SphConfig& config) {
    std::mt19937 generator(config.getSeed());
    std::uniform_real_distribution<double> xDistribution(0.0, config.getDomainWidth());
    std::uniform_real_distribution<double> yDistribution(0.0, config.getDomainHeight());

    particles_.reserve(static_cast<std::size_t>(config.getParticleCount()));
    for (int id = 0; id < config.getParticleCount(); ++id) {
        particles_.emplace_back(id, config.getParticleMass(),
                                xDistribution(generator), yDistribution(generator));
    }
}

int SphFluid::getParticleCount() const {
    return static_cast<int>(particles_.size());
}

const SphParticle& SphFluid::getParticle(int index) const {
    return particles_.at(static_cast<std::size_t>(index));
}

SphParticle& SphFluid::getParticle(int index) {
    return particles_.at(static_cast<std::size_t>(index));
}

void SphFluid::reset() {
    for (SphParticle& particle : particles_) {
        particle.reset();
    }
}

#include "core/SphFluid.hpp"

#include <algorithm>
#include <cmath>

SphFluid::SphFluid(const SphConfig& config) {
    particles_.reserve(static_cast<std::size_t>(config.getParticleCount()));
    const double spacing = config.getSmoothingLength() * 0.5;
    const int columns = std::max(
        1, static_cast<int>((config.getDomainWidth() * 0.25) / spacing));
    const double startX = spacing;
    const double startY = spacing;

    for (int id = 0; id < config.getParticleCount(); ++id) {
        const int row = id / columns;
        const int column = id % columns;
        const double x = startX + static_cast<double>(column) * spacing;
        const double y = startY + static_cast<double>(row) * spacing;
        particles_.emplace_back(id, config.getParticleMass(),
                                std::min(x, config.getDomainWidth()),
                                std::min(y, config.getDomainHeight()));
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

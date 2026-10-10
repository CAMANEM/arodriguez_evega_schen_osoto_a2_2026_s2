#include "core/SphForces.hpp"

#include <algorithm>
#include <cmath>

namespace {

double squaredDistance(const SphParticle& first, const SphParticle& second,
                       double& dx, double& dy) {
    dx = first.get_pos_x() - second.get_pos_x();
    dy = first.get_pos_y() - second.get_pos_y();
    return dx * dx + dy * dy;
}

double densityKernel(double squaredDistanceValue, double smoothingLength) {
    const double h2 = smoothingLength * smoothingLength;
    if (squaredDistanceValue >= h2) {
        return 0.0;
    }
    const double difference = h2 - squaredDistanceValue;
    return difference * difference * difference;
}

} // namespace

SphNeighborList SphForces::findNeighbors(const SphFluid& fluid,
                                          const SphConfig& config) {
    SphNeighborList neighbors(
        static_cast<std::size_t>(fluid.getParticleCount()));
    const double h2 = config.getSmoothingLength() *
                      config.getSmoothingLength();
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        for (int otherIndex = 0; otherIndex < fluid.getParticleCount();
             ++otherIndex) {
            double dx = 0.0;
            double dy = 0.0;
            if (squaredDistance(fluid.getParticle(index),
                                fluid.getParticle(otherIndex), dx, dy) < h2) {
                neighbors[static_cast<std::size_t>(index)].push_back(otherIndex);
            }
        }
    }
    return neighbors;
}

double SphForces::computeDensityForParticle(int particleIndex,
                                            const SphFluid& fluid,
                                            const SphConfig& config,
                                            const SphNeighborList& neighbors) {
    double density = 0.0;
    for (const int neighborIndex :
         neighbors.at(static_cast<std::size_t>(particleIndex))) {
        density += computeDensityContribution(particleIndex, neighborIndex,
                                              fluid, config);
    }
    return std::max(density, 1e-12);
}

double SphForces::computeDensityContribution(int particleIndex,
                                             int neighborIndex,
                                             const SphFluid& fluid,
                                             const SphConfig& config) {
    double dx = 0.0;
    double dy = 0.0;
    const double distanceSquared =
        squaredDistance(fluid.getParticle(particleIndex),
                        fluid.getParticle(neighborIndex), dx, dy);
    return config.getParticleMass() *
           densityKernel(distanceSquared, config.getSmoothingLength());
}

void SphForces::computeAllDensities(SphFluid& fluid, const SphConfig& config,
                                    const SphNeighborList& neighbors) {
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        fluid.getParticle(index).setDensity(
            computeDensityForParticle(index, fluid, config, neighbors));
    }
}

void SphForces::computeAllPressures(SphFluid& fluid, const SphConfig& config) {
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        SphParticle& particle = fluid.getParticle(index);
        particle.setPressure(
            config.getGasStiffness() *
            (particle.getDensity() - config.getRestDensity()));
    }
}

std::vector<SphForceData> SphForces::computeAllForces(
    const SphFluid& fluid, const SphConfig& config,
    const SphNeighborList& neighbors) {
    std::vector<SphForceData> forces(
        static_cast<std::size_t>(fluid.getParticleCount()), {0.0, 0.0});

    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        const SphParticle& particle = fluid.getParticle(index);
        SphForceData force{0.0, particle.get_mass() * config.getGravity()};

        for (const int neighborIndex :
             neighbors.at(static_cast<std::size_t>(index))) {
            if (neighborIndex == index) {
                continue;
            }
            const SphParticle& neighbor = fluid.getParticle(neighborIndex);
            double dx = 0.0;
            double dy = 0.0;
            const double distanceSquared =
                squaredDistance(particle, neighbor, dx, dy);
            const double distance = std::sqrt(distanceSquared);
            if (distance <= 1e-12 ||
                distance >= config.getSmoothingLength()) {
                continue;
            }

            const double inverseDistance = 1.0 / distance;
            const double pressureTerm =
                -config.getParticleMass() *
                (particle.getPressure() + neighbor.getPressure()) /
                (2.0 * neighbor.getDensity()) *
                (config.getSmoothingLength() - distance) * inverseDistance;
            const double viscosityTerm =
                config.getViscosity() * config.getParticleMass() /
                neighbor.getDensity();
            force.forceX += (pressureTerm * dx) +
                            viscosityTerm *
                                (neighbor.get_speed_x() - particle.get_speed_x());
            force.forceY += (pressureTerm * dy) +
                            viscosityTerm *
                                (neighbor.get_speed_y() - particle.get_speed_y());
        }
        forces[static_cast<std::size_t>(index)] = force;
    }
    return forces;
}

void SphForces::applyForcesAndIntegrate(
    SphFluid& fluid, const SphConfig& config,
    const std::vector<SphForceData>& forces) {
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        const SphForceData& force = forces.at(static_cast<std::size_t>(index));
        SphParticle& particle = fluid.getParticle(index);
        particle.set_force(force.forceX, force.forceY);
        particle.update(config.getDeltaTime());
    }
}

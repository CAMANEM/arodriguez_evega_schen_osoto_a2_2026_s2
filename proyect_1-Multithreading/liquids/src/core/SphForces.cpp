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
    constexpr double pi = 3.14159265358979323846;
    const double coefficient =
        315.0 / (64.0 * pi * std::pow(smoothingLength, 9.0));
    return coefficient * difference * difference * difference;
}

double spikyGradientFactor(double distance, double smoothingLength) {
    if (distance <= 1e-12 || distance >= smoothingLength) {
        return 0.0;
    }
    constexpr double pi = 3.14159265358979323846;
    const double coefficient =
        -45.0 / (pi * std::pow(smoothingLength, 6.0));
    const double difference = smoothingLength - distance;
    return coefficient * difference * difference;
}

double viscosityLaplacian(double distance, double smoothingLength) {
    if (distance >= smoothingLength) {
        return 0.0;
    }
    constexpr double pi = 3.14159265358979323846;
    const double coefficient =
        45.0 / (pi * std::pow(smoothingLength, 6.0));
    return coefficient * (smoothingLength - distance);
}

} // namespace

SphNeighborList SphForces::findNeighbors(const SphFluid& fluid,
                                          const SphConfig& config) {
    SphNeighborList neighbors(
        static_cast<std::size_t>(fluid.getParticleCount()));
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        neighbors[static_cast<std::size_t>(index)] =
            findNeighborsForParticle(index, fluid, config);
    }
    return neighbors;
}

std::vector<int> SphForces::findNeighborsForParticle(
    int particleIndex, const SphFluid& fluid, const SphConfig& config) {
    std::vector<int> neighbors;
    const double h2 = config.getSmoothingLength() *
                      config.getSmoothingLength();
    for (int otherIndex = 0; otherIndex < fluid.getParticleCount();
         ++otherIndex) {
        double dx = 0.0;
        double dy = 0.0;
        if (squaredDistance(fluid.getParticle(particleIndex),
                            fluid.getParticle(otherIndex), dx, dy) < h2) {
            neighbors.push_back(otherIndex);
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
    return std::max(
        density, config.getRestDensity() * config.getMinDensityRatio());
}

double SphForces::computePressureForParticle(const SphParticle& particle,
                                             const SphConfig& config) {
    const double rawPressure =
        config.getGasStiffness() *
        (particle.getDensity() - config.getRestDensity());
    return std::clamp(rawPressure, -config.getMaxPressure(),
                      config.getMaxPressure());
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
        particle.setPressure(computePressureForParticle(particle, config));
    }
}

std::vector<SphForceData> SphForces::computeAllForces(
    const SphFluid& fluid, const SphConfig& config,
    const SphNeighborList& neighbors) {
    std::vector<SphForceData> forces(
        static_cast<std::size_t>(fluid.getParticleCount()), {0.0, 0.0});

    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        forces[static_cast<std::size_t>(index)] =
            computeForceForParticle(index, fluid, config, neighbors);
    }
    return forces;
}

SphForceData SphForces::computeForceForParticle(
    int index, const SphFluid& fluid, const SphConfig& config,
    const SphNeighborList& neighbors) {
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
            const double densityFloor =
                config.getRestDensity() * config.getMinDensityRatio();
            const double neighborDensity =
                std::max(neighbor.getDensity(), densityFloor);

            const double pressureTerm =
                -config.getParticleMass() *
                (particle.getPressure() + neighbor.getPressure()) /
                (2.0 * neighborDensity);
            const double gradientFactor =
                spikyGradientFactor(distance, config.getSmoothingLength());
            const double viscosityTerm =
                config.getViscosity() * config.getParticleMass() /
                neighborDensity;
            const double laplacian =
                viscosityLaplacian(distance, config.getSmoothingLength());
            force.forceX += pressureTerm * gradientFactor * dx * inverseDistance;
            force.forceY += pressureTerm * gradientFactor * dy * inverseDistance;
            force.forceX += viscosityTerm * (neighbor.get_speed_x() -
                                             particle.get_speed_x()) *
                            laplacian;
            force.forceY += viscosityTerm * (neighbor.get_speed_y() -
                                             particle.get_speed_y()) *
                            laplacian;
        }
        return force;
}

void SphForces::applyForcesAndIntegrate(
    SphFluid& fluid, const SphConfig& config,
    const std::vector<SphForceData>& forces) {
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        integrateParticle(fluid.getParticle(index), config,
                          forces.at(static_cast<std::size_t>(index)));
    }
}

void SphForces::integrateParticle(SphParticle& particle,
                                   const SphConfig& config,
                                   const SphForceData& force) {
    particle.set_force(force.forceX, force.forceY);
    particle.update(config.getDeltaTime());
    const double speedSquared =
        particle.get_speed_x() * particle.get_speed_x() +
        particle.get_speed_y() * particle.get_speed_y();
        const double maxSpeedSquared =
            config.getMaxSpeed() * config.getMaxSpeed();
        if (speedSquared > maxSpeedSquared) {
            const double scale =
                config.getMaxSpeed() / std::sqrt(speedSquared);
            particle.set_velocity(particle.get_speed_x() * scale,
                                  particle.get_speed_y() * scale);
        }
}

void SphForces::applyBoundary(SphFluid& fluid, const SphConfig& config) {
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        applyBoundaryToParticle(fluid.getParticle(index), config);
    }
}

void SphForces::applyBoundaryToParticle(SphParticle& particle,
                                         const SphConfig& config) {
        if (particle.get_pos_x() < 0.0) {
            particle.set_position(0.0, particle.get_pos_y());
            particle.set_velocity(particle.get_speed_x() *
                                      config.getBoundaryDamping(),
                                  particle.get_speed_y());
        } else if (particle.get_pos_x() > config.getDomainWidth()) {
            particle.set_position(config.getDomainWidth(),
                                  particle.get_pos_y());
            particle.set_velocity(particle.get_speed_x() *
                                      config.getBoundaryDamping(),
                                  particle.get_speed_y());
        }
        if (particle.get_pos_y() < 0.0) {
            particle.set_position(particle.get_pos_x(), 0.0);
            particle.set_velocity(particle.get_speed_x(),
                                  particle.get_speed_y() *
                                      config.getBoundaryDamping());
        } else if (particle.get_pos_y() > config.getDomainHeight()) {
            particle.set_position(particle.get_pos_x(),
                                  config.getDomainHeight());
            particle.set_velocity(particle.get_speed_x(),
                                  particle.get_speed_y() *
                                      config.getBoundaryDamping());
    }
}

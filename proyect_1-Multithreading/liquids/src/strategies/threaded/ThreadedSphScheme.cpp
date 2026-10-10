#include "strategies/threaded/ThreadedSphScheme.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace {

using Clock = std::chrono::steady_clock;

std::vector<SphStageMetrics> makeStageMetrics() {
    return {
        {SphStage::Setup, 0.0, 0},
        {SphStage::NeighborStructure, 0.0, 0},
        {SphStage::Density, 0.0, 0},
        {SphStage::Pressure, 0.0, 0},
        {SphStage::Forces, 0.0, 0},
        {SphStage::Integrate, 0.0, 0},
        {SphStage::Boundary, 0.0, 0},
        {SphStage::Metrics, 0.0, 0},
    };
}

}

void ThreadedSphScheme::initialize(SphFluid&, const SphConfig&) {
    neighbors_.clear();
    forces_.clear();
    stageMetrics_ = makeStageMetrics();
    runMetrics_ = {0.0, 0, stageMetrics_};
    lastEffectiveThreadCount_ = 1;
}

unsigned int ThreadedSphScheme::effectiveThreadCount(int itemCount) const {
    const unsigned int requested = std::max(1u, requestedThreadCount());
    return std::min(requested, static_cast<unsigned int>(itemCount));
}

void ThreadedSphScheme::recordStage(SphStage stage, double seconds) {
    for (SphStageMetrics& metrics : stageMetrics_) {
        if (metrics.stage == stage) {
            metrics.wallSeconds += seconds;
            ++metrics.executions;
            return;
        }
    }
    throw std::logic_error("Unknown SPH stage");
}

void ThreadedSphScheme::runStage(SphStage stage, SphFluid& fluid,
                                 const SphConfig& config) {
    const auto start = Clock::now();
    const int particleCount = fluid.getParticleCount();

    switch (stage) {
    case SphStage::Setup:
        break;
    case SphStage::NeighborStructure:
        neighbors_.resize(static_cast<std::size_t>(particleCount));
        parallelFor(particleCount, [&](int begin, int end) {
            for (int index = begin; index < end; ++index) {
                neighbors_[static_cast<std::size_t>(index)] =
                    SphForces::findNeighborsForParticle(index, fluid, config);
            }
        });
        break;
    case SphStage::Density:
        if (neighbors_.size() != static_cast<std::size_t>(particleCount)) {
            throw std::logic_error("Density stage requires neighbor structure");
        }
        parallelFor(particleCount, [&](int begin, int end) {
            for (int index = begin; index < end; ++index) {
                fluid.getParticle(index).setDensity(
                    SphForces::computeDensityForParticle(index, fluid, config,
                                                         neighbors_));
            }
        });
        break;
    case SphStage::Pressure:
        parallelFor(particleCount, [&](int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphParticle& particle = fluid.getParticle(index);
                particle.setPressure(
                    SphForces::computePressureForParticle(particle, config));
            }
        });
        break;
    case SphStage::Forces:
        forces_.resize(static_cast<std::size_t>(particleCount));
        parallelFor(particleCount, [&](int begin, int end) {
            for (int index = begin; index < end; ++index) {
                forces_[static_cast<std::size_t>(index)] =
                    SphForces::computeForceForParticle(index, fluid, config,
                                                       neighbors_);
            }
        });
        break;
    case SphStage::Integrate:
        if (forces_.size() != static_cast<std::size_t>(particleCount)) {
            throw std::logic_error("Integrate stage requires forces");
        }
        parallelFor(particleCount, [&](int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphForces::integrateParticle(
                    fluid.getParticle(index), config,
                    forces_[static_cast<std::size_t>(index)]);
            }
        });
        break;
    case SphStage::Boundary:
        parallelFor(particleCount, [&](int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphForces::applyBoundaryToParticle(fluid.getParticle(index),
                                                   config);
            }
        });
        break;
    case SphStage::Metrics:
        break;
    }

    recordStage(
        stage, std::chrono::duration<double>(Clock::now() - start).count());
    lastEffectiveThreadCount_ = effectiveThreadCount(particleCount);
}

void ThreadedSphScheme::runStep(SphFluid& fluid, const SphConfig& config) {
    runStage(SphStage::NeighborStructure, fluid, config);
    runStage(SphStage::Density, fluid, config);
    runStage(SphStage::Pressure, fluid, config);
    runStage(SphStage::Forces, fluid, config);
    runStage(SphStage::Integrate, fluid, config);
    runStage(SphStage::Boundary, fluid, config);
    runStage(SphStage::Metrics, fluid, config);
    ++runMetrics_.completedSteps;
}

void ThreadedSphScheme::runAllSteps(SphFluid& fluid,
                                    const SphConfig& config) {
    initialize(fluid, config);
    const auto start = Clock::now();
    for (int step = 0; step < config.getTimeSteps(); ++step) {
        runStep(fluid, config);
    }
    runMetrics_.totalWallSeconds =
        std::chrono::duration<double>(Clock::now() - start).count();
    runMetrics_.perStage = stageMetrics_;
}

const std::vector<SphStageMetrics>& ThreadedSphScheme::stageMetrics() const {
    return stageMetrics_;
}

const SphRunMetrics& ThreadedSphScheme::runMetrics() const {
    return runMetrics_;
}

SphMetrics ThreadedSphScheme::simulate(SphFluid& fluid,
                                        const SphConfig& config) {
    runAllSteps(fluid, config);
    double densitySeconds = 0.0;
    for (const SphStageMetrics& metrics : stageMetrics_) {
        if (metrics.stage == SphStage::Density) {
            densitySeconds = metrics.wallSeconds;
            break;
        }
    }
    return SphMetrics(getExecutionModel(), getSchemeName(),
                      static_cast<int>(lastEffectiveThreadCount_),
                      fluid.getParticleCount(), densitySeconds, runMetrics_);
}

SphMetrics ThreadedSphScheme::simulateStep(SphFluid& fluid,
                                            const SphConfig& config) {
    initialize(fluid, config);
    runStep(fluid, config);
    runMetrics_.perStage = stageMetrics_;
    double densitySeconds = 0.0;
    for (const SphStageMetrics& metrics : stageMetrics_) {
        if (metrics.stage == SphStage::Density) {
            densitySeconds = metrics.wallSeconds;
            break;
        }
    }
    return SphMetrics(getExecutionModel(), getSchemeName(),
                      static_cast<int>(lastEffectiveThreadCount_),
                      fluid.getParticleCount(), densitySeconds, runMetrics_);
}

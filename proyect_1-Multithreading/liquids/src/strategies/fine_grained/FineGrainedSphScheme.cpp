#include "strategies/fine_grained/FineGrainedSphScheme.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>

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

} // namespace

FineGrainedSphScheme::FineGrainedSphScheme(int threadCount, int quantumItems,
                                           TraceCallback traceCallback)
    : threadCount_(std::max(1, threadCount)),
      quantumItems_(std::max(1, quantumItems)),
      traceCallback_(std::move(traceCallback)) {}

FineGrainedSphScheme::FineGrainedSphScheme(int threadCount,
                                           TraceCallback traceCallback)
    : FineGrainedSphScheme(threadCount, 1, std::move(traceCallback)) {}

template <typename Task>
void FineGrainedSphScheme::runFineGrained(int itemCount, Task task) {
    if (itemCount <= 0) {
        return;
    }

    const int activeThreads = std::min(threadCount_, itemCount);
    int nextBegin = 0;
    int threadIndex = 0;
    while (nextBegin < itemCount) {
        const int begin = nextBegin;
        const int end = std::min(itemCount, begin + quantumItems_);
        task(threadIndex, begin, end);
        nextBegin = end;
        threadIndex = (threadIndex + 1) % activeThreads;
    }
}

void FineGrainedSphScheme::initialize(SphFluid&, const SphConfig&) {
    neighbors_.clear();
    forces_.clear();
    stageMetrics_ = makeStageMetrics();
    runMetrics_ = {0.0, 0, stageMetrics_};
}

void FineGrainedSphScheme::runStage(SphStage stage, SphFluid& fluid,
                                    const SphConfig& config) {
    const auto start = Clock::now();
    const int particleCount = fluid.getParticleCount();

    switch (stage) {
    case SphStage::Setup:
        break;
    case SphStage::NeighborStructure:
        neighbors_.resize(static_cast<std::size_t>(particleCount));
        runFineGrained(particleCount, [&](int, int begin, int end) {
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
        runFineGrained(particleCount, [&](int threadIndex, int begin, int end) {
            const int activeThreads = std::min(threadCount_, particleCount);
            for (int index = begin; index < end; ++index) {
                fluid.getParticle(index).setDensity(
                    SphForces::computeDensityForParticle(index, fluid, config,
                                                         neighbors_));
                if (traceCallback_) {
                    const auto& list =
                        neighbors_[static_cast<std::size_t>(index)];
                    for (int neighbor : list) {
                        traceCallback_(threadIndex, index, neighbor,
                                       activeThreads);
                    }
                }
            }
        });
        break;
    case SphStage::Pressure:
        runFineGrained(particleCount, [&](int, int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphParticle& particle = fluid.getParticle(index);
                particle.setPressure(
                    SphForces::computePressureForParticle(particle, config));
            }
        });
        break;
    case SphStage::Forces:
        forces_.resize(static_cast<std::size_t>(particleCount));
        runFineGrained(particleCount, [&](int, int begin, int end) {
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
        runFineGrained(particleCount, [&](int, int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphForces::integrateParticle(
                    fluid.getParticle(index), config,
                    forces_[static_cast<std::size_t>(index)]);
            }
        });
        break;
    case SphStage::Boundary:
        runFineGrained(particleCount, [&](int, int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphForces::applyBoundaryToParticle(fluid.getParticle(index),
                                                   config);
            }
        });
        break;
    case SphStage::Metrics:
        break;
    }

    const double seconds =
        std::chrono::duration<double>(Clock::now() - start).count();
    for (SphStageMetrics& metrics : stageMetrics_) {
        if (metrics.stage == stage) {
            metrics.wallSeconds += seconds;
            metrics.executions += 1;
            return;
        }
    }
    throw std::logic_error("Unknown SPH stage");
}

void FineGrainedSphScheme::runStep(SphFluid& fluid, const SphConfig& config) {
    runStage(SphStage::NeighborStructure, fluid, config);
    runStage(SphStage::Density, fluid, config);
    runStage(SphStage::Pressure, fluid, config);
    runStage(SphStage::Forces, fluid, config);
    runStage(SphStage::Integrate, fluid, config);
    runStage(SphStage::Boundary, fluid, config);
    runStage(SphStage::Metrics, fluid, config);
    ++runMetrics_.completedSteps;
}

void FineGrainedSphScheme::runAllSteps(SphFluid& fluid,
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

const std::vector<SphStageMetrics>& FineGrainedSphScheme::stageMetrics() const {
    return stageMetrics_;
}

const SphRunMetrics& FineGrainedSphScheme::runMetrics() const {
    return runMetrics_;
}

SphMetrics FineGrainedSphScheme::simulate(SphFluid& fluid,
                                           const SphConfig& config) {
    runAllSteps(fluid, config);
    double densitySeconds = 0.0;
    for (const SphStageMetrics& metrics : stageMetrics_) {
        if (metrics.stage == SphStage::Density) {
            densitySeconds = metrics.wallSeconds;
            break;
        }
    }
    return SphMetrics(getExecutionModel(), getSchemeName(), threadCount_,
                      fluid.getParticleCount(), densitySeconds, runMetrics_);
}

SphMetrics FineGrainedSphScheme::simulateStep(SphFluid& fluid,
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
    return SphMetrics(getExecutionModel(), getSchemeName(), threadCount_,
                      fluid.getParticleCount(), densitySeconds, runMetrics_);
}

std::string FineGrainedSphScheme::getSchemeName() const {
    return "Fine-grained SPH";
}

execution_model FineGrainedSphScheme::getExecutionModel() const {
    return execution_model::fine_grained;
}

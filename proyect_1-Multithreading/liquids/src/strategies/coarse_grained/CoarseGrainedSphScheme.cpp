#include "strategies/coarse_grained/CoarseGrainedSphScheme.hpp"

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

} // namespace

CoarseGrainedSphScheme::CoarseGrainedSphScheme(int threadCount, int chunkItems)
    : threadCount_(std::max(1, threadCount)),
      chunkItems_(std::max(1, chunkItems)) {}

template <typename Task>
void CoarseGrainedSphScheme::runCoarseGrained(int itemCount, Task task) const {
    if (itemCount <= 0) {
        return;
    }

    const int activeThreads = std::min(threadCount_, itemCount);
    const int partitionSize =
        (itemCount + activeThreads - 1) / activeThreads;
    std::vector<int> cursors(static_cast<std::size_t>(activeThreads));
    std::vector<int> ends(static_cast<std::size_t>(activeThreads));
    int activeCount = 0;

    for (int worker = 0; worker < activeThreads; ++worker) {
        const int begin = worker * partitionSize;
        const int end = std::min(itemCount, begin + partitionSize);
        cursors[static_cast<std::size_t>(worker)] = begin;
        ends[static_cast<std::size_t>(worker)] = end;
        if (begin < end) {
            ++activeCount;
        }
    }

    int currentWorker = 0;
    while (activeCount > 0) {
        int& cursor = cursors[static_cast<std::size_t>(currentWorker)];
        const int end = ends[static_cast<std::size_t>(currentWorker)];
        if (cursor < end) {
            int segmentSize = end - cursor;
            if (shouldYieldOnStall(currentWorker, cursor)) {
                segmentSize =
                    std::min(segmentSize, std::max(1, partitionSize / 3));
            }
            const int segmentEnd = cursor + segmentSize;
            task(currentWorker, cursor, segmentEnd);
            cursor = segmentEnd;
            if (cursor >= end) {
                --activeCount;
            }
        }
        currentWorker = (currentWorker + 1) % activeThreads;
    }
}

bool CoarseGrainedSphScheme::shouldYieldOnStall(int workerId,
                                                 int cursor) const {
    const int signature = (cursor / std::max(1, chunkItems_)) +
                          workerId * 5 + threadCount_ * 3;
    return signature % 11 == 0;
}

void CoarseGrainedSphScheme::initialize(SphFluid&, const SphConfig&) {
    neighbors_.clear();
    forces_.clear();
    stageMetrics_ = makeStageMetrics();
    runMetrics_ = {0.0, 0, stageMetrics_};
}

void CoarseGrainedSphScheme::runStage(SphStage stage, SphFluid& fluid,
                                      const SphConfig& config) {
    const auto start = Clock::now();
    const int particleCount = fluid.getParticleCount();

    switch (stage) {
    case SphStage::Setup:
        break;
    case SphStage::NeighborStructure:
        neighbors_.resize(static_cast<std::size_t>(particleCount));
        runCoarseGrained(particleCount, [&](int, int begin, int end) {
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
        runCoarseGrained(particleCount, [&](int, int begin, int end) {
            for (int index = begin; index < end; ++index) {
                fluid.getParticle(index).setDensity(
                    SphForces::computeDensityForParticle(index, fluid, config,
                                                         neighbors_));
            }
        });
        break;
    case SphStage::Pressure:
        runCoarseGrained(particleCount, [&](int, int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphParticle& particle = fluid.getParticle(index);
                particle.setPressure(
                    SphForces::computePressureForParticle(particle, config));
            }
        });
        break;
    case SphStage::Forces:
        forces_.resize(static_cast<std::size_t>(particleCount));
        runCoarseGrained(particleCount, [&](int, int begin, int end) {
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
        runCoarseGrained(particleCount, [&](int, int begin, int end) {
            for (int index = begin; index < end; ++index) {
                SphForces::integrateParticle(
                    fluid.getParticle(index), config,
                    forces_[static_cast<std::size_t>(index)]);
            }
        });
        break;
    case SphStage::Boundary:
        runCoarseGrained(particleCount, [&](int, int begin, int end) {
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

void CoarseGrainedSphScheme::runStep(SphFluid& fluid,
                                     const SphConfig& config) {
    runStage(SphStage::NeighborStructure, fluid, config);
    runStage(SphStage::Density, fluid, config);
    runStage(SphStage::Pressure, fluid, config);
    runStage(SphStage::Forces, fluid, config);
    runStage(SphStage::Integrate, fluid, config);
    runStage(SphStage::Boundary, fluid, config);
    runStage(SphStage::Metrics, fluid, config);
    ++runMetrics_.completedSteps;
}

void CoarseGrainedSphScheme::runAllSteps(SphFluid& fluid,
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

const std::vector<SphStageMetrics>&
CoarseGrainedSphScheme::stageMetrics() const {
    return stageMetrics_;
}

const SphRunMetrics& CoarseGrainedSphScheme::runMetrics() const {
    return runMetrics_;
}

SphMetrics CoarseGrainedSphScheme::simulate(SphFluid& fluid,
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

SphMetrics CoarseGrainedSphScheme::simulateStep(
    SphFluid& fluid, const SphConfig& config) {
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

std::string CoarseGrainedSphScheme::getSchemeName() const {
    return "Coarse-grained SPH";
}

execution_model CoarseGrainedSphScheme::getExecutionModel() const {
    return execution_model::coarse_grained;
}

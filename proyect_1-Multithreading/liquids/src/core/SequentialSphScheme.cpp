#include "core/SequentialSphScheme.hpp"

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

const char* sphStageName(SphStage stage) {
    switch (stage) {
    case SphStage::Setup:
        return "Setup";
    case SphStage::NeighborStructure:
        return "NeighborStructure";
    case SphStage::Density:
        return "Density";
    case SphStage::Pressure:
        return "Pressure";
    case SphStage::Forces:
        return "Forces";
    case SphStage::Integrate:
        return "Integrate";
    case SphStage::Boundary:
        return "Boundary";
    case SphStage::Metrics:
        return "Metrics";
    }
    return "Unknown";
}

void SequentialSphScheme::initialize(SphFluid& fluid, const SphConfig&) {
    neighbors_.clear();
    forces_.clear();
    stageMetrics_ = makeStageMetrics();
    runMetrics_ = {0.0, 0, stageMetrics_};

    const auto start = Clock::now();
    (void)fluid;
    const auto elapsed = std::chrono::duration<double>(Clock::now() - start);
    stageMetrics_[0].wallSeconds += elapsed.count();
    stageMetrics_[0].executions += 1;
}

void SequentialSphScheme::runStage(SphStage stage, SphFluid& fluid,
                                   const SphConfig& config) {
    const auto start = Clock::now();
    switch (stage) {
    case SphStage::Setup:
        break;
    case SphStage::NeighborStructure:
        neighbors_ = SphForces::findNeighbors(fluid, config);
        break;
    case SphStage::Density:
        if (neighbors_.size() !=
            static_cast<std::size_t>(fluid.getParticleCount())) {
            throw std::logic_error("Density stage requires neighbor structure");
        }
        for (int index = 0; index < fluid.getParticleCount(); ++index) {
            fluid.getParticle(index).setDensity(
                SphForces::computeDensityForParticle(index, fluid, config,
                                                     neighbors_));
        }
        break;
    case SphStage::Pressure:
        SphForces::computeAllPressures(fluid, config);
        break;
    case SphStage::Forces:
        forces_ = SphForces::computeAllForces(fluid, config, neighbors_);
        break;
    case SphStage::Integrate:
        SphForces::applyForcesAndIntegrate(fluid, config, forces_);
        break;
    case SphStage::Boundary:
        SphForces::applyBoundary(fluid, config);
        break;
    case SphStage::Metrics:
        break;
    }

    const auto elapsed = std::chrono::duration<double>(Clock::now() - start);
    for (SphStageMetrics& metrics : stageMetrics_) {
        if (metrics.stage == stage) {
            metrics.wallSeconds += elapsed.count();
            metrics.executions += 1;
            return;
        }
    }
    throw std::logic_error("Unknown SPH stage");
}

void SequentialSphScheme::runStep(SphFluid& fluid, const SphConfig& config) {
    runStage(SphStage::NeighborStructure, fluid, config);
    runStage(SphStage::Density, fluid, config);
    runStage(SphStage::Pressure, fluid, config);
    runStage(SphStage::Forces, fluid, config);
    runStage(SphStage::Integrate, fluid, config);
    runStage(SphStage::Boundary, fluid, config);
    runStage(SphStage::Metrics, fluid, config);
    runMetrics_.completedSteps += 1;
}

void SequentialSphScheme::runAllSteps(SphFluid& fluid,
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

const std::vector<SphStageMetrics>& SequentialSphScheme::stageMetrics() const {
    return stageMetrics_;
}

const SphRunMetrics& SequentialSphScheme::runMetrics() const {
    return runMetrics_;
}

SphMetrics SequentialSphScheme::simulate(SphFluid& fluid,
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
                      config.getThreadCount(), fluid.getParticleCount(),
                      densitySeconds, runMetrics_);
}

SphMetrics SequentialSphScheme::simulateStep(SphFluid& fluid,
                                              const SphConfig& config) {
    initialize(fluid, config);
    runStep(fluid, config);
    runMetrics_.totalWallSeconds = 0.0;
    runMetrics_.perStage = stageMetrics_;
    double densitySeconds = 0.0;
    for (const SphStageMetrics& metrics : stageMetrics_) {
        if (metrics.stage == SphStage::Density) {
            densitySeconds = metrics.wallSeconds;
            break;
        }
    }
    return SphMetrics(getExecutionModel(), getSchemeName(), 1,
                      fluid.getParticleCount(), densitySeconds, runMetrics_);
}

std::string SequentialSphScheme::getSchemeName() const {
    return "Sequential SPH";
}

execution_model SequentialSphScheme::getExecutionModel() const {
    return execution_model::sequential;
}

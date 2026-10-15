#ifndef SPH_STAGE_RUNNER_HPP
#define SPH_STAGE_RUNNER_HPP

#include <string>
#include <vector>

#include "core/SphConfig.hpp"
#include "core/SphFluid.hpp"

enum class SphStage {
    Setup,
    NeighborStructure,
    Density,
    Pressure,
    Forces,
    Integrate,
    Boundary,
    Metrics
};

const char* sphStageName(SphStage stage);

struct SphStageMetrics {
    SphStage stage;
    double wallSeconds;
    int executions;
};

struct SphRunMetrics {
    double totalWallSeconds;
    int completedSteps;
    std::vector<SphStageMetrics> perStage;
};

class SphStageRunner {
public:
    virtual ~SphStageRunner() = default;

    virtual void initialize(SphFluid& fluid, const SphConfig& config) = 0;
    virtual void runStage(SphStage stage, SphFluid& fluid,
                          const SphConfig& config) = 0;
    virtual void runStep(SphFluid& fluid, const SphConfig& config) = 0;
    virtual void runAllSteps(SphFluid& fluid, const SphConfig& config) = 0;

    virtual const std::vector<SphStageMetrics>& stageMetrics() const = 0;
    virtual const SphRunMetrics& runMetrics() const = 0;
};

#endif

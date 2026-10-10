#ifndef SEQUENTIAL_SPH_SCHEME_HPP
#define SEQUENTIAL_SPH_SCHEME_HPP

#include "core/SphForces.hpp"
#include "core/SphScheme.hpp"

class SequentialSphScheme : public SphScheme, public SphStageRunner {
public:
    void initialize(SphFluid& fluid, const SphConfig& config) override;
    void runStage(SphStage stage, SphFluid& fluid,
                  const SphConfig& config) override;
    void runStep(SphFluid& fluid, const SphConfig& config) override;
    void runAllSteps(SphFluid& fluid, const SphConfig& config) override;
    const std::vector<SphStageMetrics>& stageMetrics() const override;
    const SphRunMetrics& runMetrics() const override;

    SphMetrics simulate(SphFluid& fluid, const SphConfig& config);
    SphMetrics simulateStep(SphFluid& fluid,
                            const SphConfig& config) override;
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

private:
    SphNeighborList neighbors_;
    std::vector<SphForceData> forces_;
    std::vector<SphStageMetrics> stageMetrics_;
    SphRunMetrics runMetrics_{0.0, 0, {}};
};

#endif

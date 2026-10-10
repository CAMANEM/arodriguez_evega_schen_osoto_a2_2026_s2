#ifndef COARSE_GRAINED_SPH_SCHEME_HPP
#define COARSE_GRAINED_SPH_SCHEME_HPP

#include "core/SphForces.hpp"
#include "core/SphScheme.hpp"

class CoarseGrainedSphScheme : public SphScheme, public SphStageRunner {
public:
    explicit CoarseGrainedSphScheme(int threadCount = 2,
                                    int chunkItems = 16);

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
    template <typename Task>
    void runCoarseGrained(int itemCount, Task task) const;
    bool shouldYieldOnStall(int workerId, int cursor) const;

    int threadCount_;
    int chunkItems_;
    SphNeighborList neighbors_;
    std::vector<SphForceData> forces_;
    std::vector<SphStageMetrics> stageMetrics_;
    SphRunMetrics runMetrics_{0.0, 0, {}};
};

#endif

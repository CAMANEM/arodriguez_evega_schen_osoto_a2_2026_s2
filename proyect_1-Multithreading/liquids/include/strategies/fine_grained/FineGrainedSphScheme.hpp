#ifndef FINE_GRAINED_SPH_SCHEME_HPP
#define FINE_GRAINED_SPH_SCHEME_HPP

#include <functional>

#include "core/SphForces.hpp"
#include "core/SphScheme.hpp"

class FineGrainedSphScheme : public SphScheme, public SphStageRunner {
public:
    using TraceCallback = std::function<void(int threadIndex,
                                             int particleIndex,
                                             int neighborParticleIndex,
                                             int activeThreads)>;

    explicit FineGrainedSphScheme(int threadCount = 2, int quantumItems = 1,
                                  TraceCallback traceCallback = {});
    FineGrainedSphScheme(int threadCount, TraceCallback traceCallback);

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
    void runFineGrained(int itemCount, Task task);

    int threadCount_;
    int quantumItems_;
    TraceCallback traceCallback_;
    SphNeighborList neighbors_;
    std::vector<SphForceData> forces_;
    std::vector<SphStageMetrics> stageMetrics_;
    SphRunMetrics runMetrics_{0.0, 0, {}};
};

#endif

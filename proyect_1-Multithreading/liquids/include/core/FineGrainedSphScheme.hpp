#ifndef FINE_GRAINED_SPH_SCHEME_HPP
#define FINE_GRAINED_SPH_SCHEME_HPP

#include <functional>

#include "core/SphScheme.hpp"

class FineGrainedSphScheme : public SphScheme {
public:
    using TraceCallback = std::function<void(int threadIndex,
                                             int particleIndex,
                                             int neighborParticleIndex,
                                             int activeThreads)>;

    explicit FineGrainedSphScheme(int contextCount = 2,
                                  TraceCallback traceCallback = {});

    SphMetrics simulateStep(SphFluid& fluid,
                            const SphConfig& config) override;
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

private:
    int contextCount_;
    TraceCallback traceCallback_;
};

#endif

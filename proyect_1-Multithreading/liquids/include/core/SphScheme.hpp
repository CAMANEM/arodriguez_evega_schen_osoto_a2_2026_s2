#ifndef SPH_SCHEME_HPP
#define SPH_SCHEME_HPP

#include <string>

#include "core/SphConfig.hpp"
#include "core/SphFluid.hpp"
#include "core/SphMetrics.hpp"
#include "core/SphStageRunner.hpp"

class SphScheme {
public:
    virtual ~SphScheme() = default;

    SphMetrics simulate(SphFluid& fluid, const SphConfig& config) {
        double measuredSeconds = 0.0;
        for (int step = 0; step < config.getTimeSteps(); ++step) {
            measuredSeconds += simulateStep(fluid, config).getMeasuredSeconds();
        }
        return SphMetrics(getExecutionModel(), getSchemeName(),
                          config.getThreadCount(), fluid.getParticleCount(),
                          measuredSeconds);
    }

    virtual SphMetrics simulateStep(SphFluid& fluid,
                                     const SphConfig& config) = 0;
    virtual std::string getSchemeName() const = 0;
    virtual execution_model getExecutionModel() const = 0;
};

#endif

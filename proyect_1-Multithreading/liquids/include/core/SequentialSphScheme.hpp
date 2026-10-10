#ifndef SEQUENTIAL_SPH_SCHEME_HPP
#define SEQUENTIAL_SPH_SCHEME_HPP

#include "core/SphScheme.hpp"

class SequentialSphScheme : public SphScheme {
public:
    SphMetrics simulateStep(SphFluid& fluid,
                            const SphConfig& config) override;
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;
};

#endif

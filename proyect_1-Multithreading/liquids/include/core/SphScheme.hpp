#ifndef SPH_SCHEME_HPP
#define SPH_SCHEME_HPP

#include <string>

#include "core/SphConfig.hpp"
#include "core/SphFluid.hpp"
#include "core/SphMetrics.hpp"

class SphScheme {
public:
    virtual ~SphScheme() = default;

    virtual SphMetrics simulateStep(SphFluid& fluid,
                                     const SphConfig& config) = 0;
    virtual std::string getSchemeName() const = 0;
    virtual execution_model getExecutionModel() const = 0;
};

#endif

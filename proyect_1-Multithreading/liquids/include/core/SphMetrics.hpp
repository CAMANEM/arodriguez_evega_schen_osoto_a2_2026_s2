#ifndef SPH_METRICS_HPP
#define SPH_METRICS_HPP

#include <string>

#include "metrics_interface.hpp"

class SphMetrics : public metrics_interface {
public:
    SphMetrics(execution_model model, const std::string& schemeName,
               int workers, int particlesProcessed, double elapsedSeconds);

    const std::string& getSchemeName() const;
    int getParticlesProcessed() const;

private:
    std::string schemeName_;
    int particlesProcessed_;
};

#endif

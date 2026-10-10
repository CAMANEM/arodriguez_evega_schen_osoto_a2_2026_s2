#ifndef SPH_METRICS_HPP
#define SPH_METRICS_HPP

#include <string>
#include <vector>

#include "core/SphStageRunner.hpp"
#include "metrics_interface.hpp"

class SphMetrics : public metrics_interface {
public:
    SphMetrics(execution_model model, const std::string& schemeName,
               int workers, int particlesProcessed, double elapsedSeconds,
               const SphRunMetrics& stageMetrics = {0.0, 0, {}});

    const std::string& getSchemeName() const;
    int getParticlesProcessed() const;
    double getMeasuredSeconds() const;
    const SphRunMetrics& getStageMetrics() const;

private:
    std::string schemeName_;
    int particlesProcessed_;
    double measuredSeconds_;
    SphRunMetrics stageMetrics_;
};

#endif

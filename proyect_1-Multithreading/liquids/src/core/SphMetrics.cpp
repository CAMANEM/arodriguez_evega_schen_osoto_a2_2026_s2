#include "core/SphMetrics.hpp"

SphMetrics::SphMetrics(execution_model model, const std::string& schemeName,
                       int workers, int particlesProcessed,
                       double elapsedSeconds)
    : metrics_interface(model, workers),
      schemeName_(schemeName),
      particlesProcessed_(particlesProcessed) {
    record_time(elapsedSeconds);
}

const std::string& SphMetrics::getSchemeName() const { return schemeName_; }
int SphMetrics::getParticlesProcessed() const { return particlesProcessed_; }

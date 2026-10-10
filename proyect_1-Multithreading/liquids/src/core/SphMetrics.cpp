#include "core/SphMetrics.hpp"

SphMetrics::SphMetrics(execution_model model, const std::string& schemeName,
                       int workers, int particlesProcessed,
                       double elapsedSeconds, const SphRunMetrics& stageMetrics)
    : metrics_interface(model, workers),
      schemeName_(schemeName),
      particlesProcessed_(particlesProcessed),
      measuredSeconds_(elapsedSeconds),
      stageMetrics_(stageMetrics) {
    record_time(elapsedSeconds);
}

const std::string& SphMetrics::getSchemeName() const { return schemeName_; }
int SphMetrics::getParticlesProcessed() const { return particlesProcessed_; }
double SphMetrics::getMeasuredSeconds() const { return measuredSeconds_; }
const SphRunMetrics& SphMetrics::getStageMetrics() const {
    return stageMetrics_;
}

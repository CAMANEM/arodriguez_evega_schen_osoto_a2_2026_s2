#include "core/CoarseGrainedScheme.hpp"

CoarseGrainedScheme::CoarseGrainedScheme(unsigned int threadCount)
    : threadCount_(threadCount) {
}

unsigned int CoarseGrainedScheme::computeThreadCount(const Flock& /*flock*/) const {
    return threadCount_;
}

std::string CoarseGrainedScheme::getSchemeName() const {
    return "Grano Grueso (hilos tradicionales)";
}

execution_model CoarseGrainedScheme::getExecutionModel() const {
    return execution_model::coarse_grained;
}

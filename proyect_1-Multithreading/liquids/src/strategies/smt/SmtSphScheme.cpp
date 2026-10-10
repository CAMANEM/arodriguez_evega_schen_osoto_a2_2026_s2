#include "strategies/smt/SmtSphScheme.hpp"

#include <algorithm>

SmtSphScheme::SmtSphScheme(unsigned int oversubscriptionFactor)
    : oversubscriptionFactor_(std::max(1u, oversubscriptionFactor)) {}

unsigned int SmtSphScheme::getOversubscriptionFactor() const {
    return oversubscriptionFactor_;
}

unsigned int SmtSphScheme::logicalProcessorCount() {
    return std::max(1u, std::thread::hardware_concurrency());
}

unsigned int SmtSphScheme::requestedThreadCount() const {
    return requestedThreadCountForBenchmark();
}

unsigned int SmtSphScheme::requestedThreadCountForBenchmark() const {
    return logicalProcessorCount() * oversubscriptionFactor_;
}

std::string SmtSphScheme::getSchemeName() const {
    return "SMT SPH";
}

execution_model SmtSphScheme::getExecutionModel() const {
    return execution_model::smt;
}

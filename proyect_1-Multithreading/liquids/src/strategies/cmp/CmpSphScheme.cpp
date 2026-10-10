#include "strategies/cmp/CmpSphScheme.hpp"

#include <algorithm>

unsigned int CmpSphScheme::logicalProcessorCount() {
    return std::max(1u, std::thread::hardware_concurrency());
}

unsigned int CmpSphScheme::requestedThreadCount() const {
    return logicalProcessorCount();
}

std::string CmpSphScheme::getSchemeName() const {
    return "CMP SPH";
}

execution_model CmpSphScheme::getExecutionModel() const {
    return execution_model::cmp;
}

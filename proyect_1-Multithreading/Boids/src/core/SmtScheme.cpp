#include "core/SmtScheme.hpp"

#include <algorithm>
#include <thread>

SmtScheme::SmtScheme(unsigned int oversubscriptionFactor)
    : oversubscriptionFactor_(std::max(1u, oversubscriptionFactor)) {
}

unsigned int SmtScheme::getOversubscriptionFactor() const {
    return oversubscriptionFactor_;
}

unsigned int SmtScheme::logicalProcessorCount() {
    // hardware_concurrency() puede devolver 0 si el SO no informa; tratar como 1.
    return std::max(1u, std::thread::hardware_concurrency());
}

unsigned int SmtScheme::requestedThreadCount() const {
    return logicalProcessorCount() * oversubscriptionFactor_;
}

unsigned int SmtScheme::computeThreadCount(const Flock& /*flock*/) const {
    // Aproximación software de SMT: varios hilos compiten por L lógicos.
    // No es Hyper-Threading; el contraste hardware es BIOS ON/OFF + perfilado.
    return requestedThreadCount();
}

BoidsMetrics SmtScheme::makeStepMetrics(int workersUsed, double elapsedMilliseconds,
                                        int boidsProcessed) const {
    return BoidsMetrics(getExecutionModel(), getSchemeName(), workersUsed,
                        elapsedMilliseconds, boidsProcessed,
                        /*virtualWorkers=*/false, /*isPartial=*/false,
                        /*stallCount=*/0, /*stallTimeMs=*/0.0,
                        /*computeTimeMs=*/-1.0, oversubscriptionFactor_,
                        logicalProcessorCount());
}

std::string SmtScheme::getSchemeName() const {
    return "SMT (sobre-suscripcion L x F)";
}

execution_model SmtScheme::getExecutionModel() const {
    return execution_model::smt;
}

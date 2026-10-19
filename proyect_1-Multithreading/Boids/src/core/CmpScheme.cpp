#include "core/CmpScheme.hpp"

#include <algorithm>
#include <thread>

unsigned int CmpScheme::logicalProcessorCount() {
    // hardware_concurrency() puede devolver 0 si el SO no informa; tratar como 1.
    // El valor reportado son procesadores lógicos (pueden incluir HT/SMT), no físicos.
    return std::max(1u, std::thread::hardware_concurrency());
}

unsigned int CmpScheme::computeThreadCount(const Flock& /*flock*/) const {
    // Política CMP: un hilo por procesador lógico visible (T = L), sin
    // sobre-suscripción (SMT) ni T fijo por --workers (Coarse).
    return logicalProcessorCount();
}

BoidsMetrics CmpScheme::makeStepMetrics(int workersUsed, double elapsedMilliseconds,
                                        int boidsProcessed) const {
    return BoidsMetrics(getExecutionModel(), getSchemeName(), workersUsed,
                        elapsedMilliseconds, boidsProcessed,
                        /*virtualWorkers=*/false, /*isPartial=*/false,
                        /*stallCount=*/0, /*stallTimeMs=*/0.0,
                        /*computeTimeMs=*/-1.0, /*oversubscribeFactor=*/0,
                        logicalProcessorCount());
}

std::string CmpScheme::getSchemeName() const {
    return "CMP (un hilo por procesador logico)";
}

execution_model CmpScheme::getExecutionModel() const {
    return execution_model::cmp;
}

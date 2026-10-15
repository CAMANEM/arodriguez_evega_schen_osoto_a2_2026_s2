#include "core/SequentialScheme.hpp"
#include "core/FlockingRules.hpp"
#include "core/Timer.hpp"

#include <vector>

BoidsMetrics SequentialScheme::simulateStep(Flock& flock, const FlockingConfig& config) {
    Timer timer;
    timer.start();

    // Fase 1: calcular todas las fuerzas leyendo el estado actual (sin
    // modificarlo todavía).
    std::vector<Vector2D> steeringForces;
    steeringForces.reserve(flock.getBoidCount());
    for (int i = 0; i < flock.getBoidCount(); ++i) {
        steeringForces.push_back(FlockingRules::computeSteeringForBoid(i, flock, config));
    }

    // Fase 2: aplicar todas las integraciones.
    for (int i = 0; i < flock.getBoidCount(); ++i) {
        flock.applyIntegration(i, steeringForces[i], config);
    }

    const double elapsedMs = timer.stopAndGetMilliseconds();
    return BoidsMetrics(getExecutionModel(), getSchemeName(), 1, elapsedMs,
                        flock.getBoidCount());
}

std::string SequentialScheme::getSchemeName() const {
    return "Secuencial (baseline)";
}

execution_model SequentialScheme::getExecutionModel() const {
    return execution_model::sequential;
}

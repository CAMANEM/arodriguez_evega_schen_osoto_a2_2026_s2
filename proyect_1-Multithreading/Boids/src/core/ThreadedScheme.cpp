#include "core/ThreadedScheme.hpp"
#include "core/FlockingRules.hpp"
#include "core/Timer.hpp"

#include <algorithm>
#include <functional>
#include <thread>

void ThreadedScheme::computeForceBlock(const Flock& flock, const FlockingConfig& config,
                                        int startIndex, int endIndex,
                                        std::vector<Vector2D>& steeringForces) const {
    for (int i = startIndex; i < endIndex; ++i) {
        steeringForces[i] = FlockingRules::computeSteeringForBoid(i, flock, config);
    }
}

BoidsMetrics ThreadedScheme::simulateStep(Flock& flock, const FlockingConfig& config) {
    const unsigned int threadCount = std::max(1u, computeThreadCount(flock));
    const int totalBoids = flock.getBoidCount();
    const int boidsPerThread = (totalBoids + static_cast<int>(threadCount) - 1) / static_cast<int>(threadCount);

    Timer timer;
    timer.start();

    // Fase 1 (paralela): cada hilo calcula las fuerzas de su bloque de
    // boids leyendo el enjambre completo (solo lectura, sin conflicto) y
    // escribiendo en su porción disjunta de steeringForces.
    std::vector<Vector2D> steeringForces(totalBoids);
    std::vector<std::thread> workers;
    workers.reserve(threadCount);

    for (unsigned int t = 0; t < threadCount; ++t) {
        const int startIndex = static_cast<int>(t) * boidsPerThread;
        const int endIndex = std::min(totalBoids, startIndex + boidsPerThread);
        if (startIndex >= endIndex) {
            break; // Más hilos que boids disponibles; no se lanzan hilos vacíos.
        }
        workers.emplace_back(&ThreadedScheme::computeForceBlock, this, std::cref(flock),
                              std::cref(config), startIndex, endIndex, std::ref(steeringForces));
    }

    // join() es el "evento de bloqueo costoso" que sincroniza a los hilos
    // tradicionales, tal como lo describe el modelo de grano grueso.
    for (auto& worker : workers) {
        worker.join();
    }

    // Fase 2 (secuencial, económica): se aplican todas las integraciones.
    for (int i = 0; i < totalBoids; ++i) {
        flock.applyIntegration(i, steeringForces[i], config);
    }

    const double elapsedMs = timer.stopAndGetMilliseconds();
    return BoidsMetrics(getExecutionModel(), getSchemeName(),
                        static_cast<int>(workers.size()), elapsedMs, totalBoids);
}

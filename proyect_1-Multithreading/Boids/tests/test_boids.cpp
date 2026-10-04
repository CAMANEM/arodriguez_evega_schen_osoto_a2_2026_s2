#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "object_interface.hpp"
#include "core/Boid.hpp"
#include "core/BoidsMetrics.hpp"
#include "core/CoarseGrainedScheme.hpp"
#include "core/FineGrainedScheme.hpp"
#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/SequentialScheme.hpp"
#include "core/SteeringContext.hpp"
#include "core/Vector2D.hpp"

namespace {

/**
 * @brief Indica si dos escalares coinciden dentro de una tolerancia.
 */
bool nearlyEqual(double a, double b, double tolerance = 1e-9) {
    return std::abs(a - b) <= tolerance;
}

/**
 * @brief Compara posición y velocidad de los primeros boids de dos enjambres.
 */
bool flocksMatch(const Flock& a, const Flock& b, int count,
                 double tolerance = 1e-6) {
    if (a.getBoidCount() != b.getBoidCount() || count > a.getBoidCount()) {
        return false;
    }

    for (int i = 0; i < count; ++i) {
        const Vector2D positionDifference =
            a.getBoid(i).getPosition() - b.getBoid(i).getPosition();
        const Vector2D velocityDifference =
            a.getBoid(i).getVelocity() - b.getBoid(i).getVelocity();
        if (positionDifference.magnitude() > tolerance ||
            velocityDifference.magnitude() > tolerance) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Verifica el contrato físico compartido y reset().
 */
bool testObjectInterface() {
    Boid boid(7, Vector2D(1.0, 2.0), Vector2D(3.0, 4.0));
    object_interface* object = &boid;
    object->set_force(2.0, 4.0);
    object->update(0.5);

    const bool updated = object->get_id() == 7 &&
                         nearlyEqual(object->get_acc_x(), 2.0) &&
                         nearlyEqual(object->get_acc_y(), 4.0) &&
                         nearlyEqual(object->get_speed_x(), 4.0) &&
                         nearlyEqual(object->get_speed_y(), 6.0) &&
                         nearlyEqual(object->get_pos_x(), 3.0) &&
                         nearlyEqual(object->get_pos_y(), 5.0);

    object->reset();
    const bool reset = nearlyEqual(object->get_pos_x(), 0.0) &&
                       nearlyEqual(object->get_pos_y(), 0.0) &&
                       nearlyEqual(object->get_speed_x(), 0.0) &&
                       nearlyEqual(object->get_force_x(), 0.0);
    return updated && reset;
}

FlockingConfig makeTestConfig(int boidCount) {
    return FlockingConfig(boidCount, 200.0, 200.0, 50.0, 20.0,
                          2.6, 0.18, 1.0, 1.4, 0.8, 1.0);
}

/**
 * @brief Equivalencia Fine vs Sequential sobre un subconjunto parcial.
 */
bool testFineGrainedPartialEquivalence() {
    const FlockingConfig config = makeTestConfig(12);
    const Flock initial(config, 17);
    Flock sequentialFlock = initial;
    Flock fineFlock = initial;

    SequentialScheme sequential;
    FineGrainedScheme fine(5);
    sequential.simulateStep(sequentialFlock, config);
    const BoidsMetrics metrics = fine.simulateStep(fineFlock, config);

    return metrics.get_model() == execution_model::fine_grained &&
           metrics.get_n_workers() == 5 &&
           metrics.get_run_count() == 1 &&
           metrics.uses_virtual_workers() &&
           metrics.is_partial() &&
           metrics.get_boids_processed() == 5 &&
           flocksMatch(sequentialFlock, fineFlock, 5);
}

/**
 * @brief Equivalencia Fine vs Sequential sobre el flock completo.
 */
bool testFineGrainedFullEquivalence() {
    const FlockingConfig config = makeTestConfig(12);
    const Flock initial(config, 17);
    Flock sequentialFlock = initial;
    Flock fineFlock = initial;

    SequentialScheme sequential;
    FineGrainedScheme fine(0);
    sequential.simulateStep(sequentialFlock, config);
    const BoidsMetrics metrics = fine.simulateStep(fineFlock, config);

    return !metrics.is_partial() &&
           metrics.get_n_workers() == 12 &&
           flocksMatch(sequentialFlock, fineFlock, 12);
}

/**
 * @brief El scheduler termina y cada contexto examina N-1 candidatos.
 */
bool testFineGrainedTermination() {
    const FlockingConfig config = makeTestConfig(6);
    const Flock flock(config, 3);
    std::vector<SteeringContext> contexts;
    for (int i = 0; i < flock.getBoidCount(); ++i) {
        contexts.emplace_back(i, flock, config);
    }

    FineGrainedScheme::runRoundRobinUntilDone(contexts);

    for (const auto& context : contexts) {
        if (!context.isFinished() ||
            context.getQuantumsExecuted() != flock.getBoidCount() - 1) {
            return false;
        }
    }
    return true;
}

/**
 * @brief stepOnce avanza exactamente un candidato (saltando el boid propio).
 */
bool testFineGrainedSingleQuantum() {
    const FlockingConfig config = makeTestConfig(4);
    const Flock flock(config, 9);
    SteeringContext context(0, flock, config);

    if (context.getCandidateCursor() != 1 || context.getQuantumsExecuted() != 0) {
        return false;
    }

    const bool stillActive = context.stepOnce();
    return stillActive &&
           context.getQuantumsExecuted() == 1 &&
           context.getCandidateCursor() == 2 &&
           !context.isFinished();
}

/**
 * @brief Una ronda sirve A,B,C... aunque cada contexto tenga trabajo pendiente.
 */
bool testFineGrainedRoundRobin() {
    const FlockingConfig config = makeTestConfig(3);
    const Flock flock(config, 11);
    std::vector<SteeringContext> contexts;
    for (int i = 0; i < 3; ++i) {
        contexts.emplace_back(i, flock, config);
    }

    const bool stillPending = FineGrainedScheme::runOneRound(contexts);
    for (const auto& context : contexts) {
        if (context.getQuantumsExecuted() != 1) {
            return false;
        }
    }

    const bool finishedAfterSecondRound = !FineGrainedScheme::runOneRound(contexts);
    for (const auto& context : contexts) {
        if (context.getQuantumsExecuted() != 2 || !context.isFinished()) {
            return false;
        }
    }
    return stillPending && finishedAfterSecondRound;
}

/**
 * @brief Bordes: 0 y 1 boid, partial=0, partial mayor al flock (clamp).
 */
bool testFineGrainedEdgeCases() {
    const FlockingConfig emptyConfig = makeTestConfig(0);
    Flock emptyFlock(emptyConfig, 1);
    FineGrainedScheme fineAll(0);
    const BoidsMetrics emptyMetrics = fineAll.simulateStep(emptyFlock, emptyConfig);
    if (emptyMetrics.get_boids_processed() != 0 || emptyMetrics.get_n_workers() != 0) {
        return false;
    }

    const FlockingConfig oneConfig = makeTestConfig(1);
    Flock sequentialOne(oneConfig, 4);
    Flock fineOne = sequentialOne;
    SequentialScheme sequential;
    sequential.simulateStep(sequentialOne, oneConfig);
    const BoidsMetrics oneMetrics = fineAll.simulateStep(fineOne, oneConfig);
    if (oneMetrics.is_partial() || oneMetrics.get_n_workers() != 1 ||
        !flocksMatch(sequentialOne, fineOne, 1)) {
        return false;
    }

    if (FineGrainedScheme::resolveContextCount(8, 0) != 8 ||
        FineGrainedScheme::resolveContextCount(8, 3) != 3 ||
        FineGrainedScheme::resolveContextCount(8, 20) != 8 ||
        FineGrainedScheme::resolveContextCount(0, 5) != 0) {
        return false;
    }

    const FlockingConfig config = makeTestConfig(8);
    Flock sequentialFlock(config, 21);
    Flock clampedFlock = sequentialFlock;
    sequential.simulateStep(sequentialFlock, config);
    FineGrainedScheme clamped(20);
    const BoidsMetrics clampedMetrics = clamped.simulateStep(clampedFlock, config);
    return !clampedMetrics.is_partial() &&
           clampedMetrics.get_n_workers() == 8 &&
           flocksMatch(sequentialFlock, clampedFlock, 8);
}

/**
 * @brief Verifica que coarse-grained produzca el baseline completo.
 */
bool testCoarseGrainedEquivalence() {
    const FlockingConfig config(12, 200.0, 200.0, 50.0, 20.0,
                                2.6, 0.18, 1.0, 1.4, 0.8, 1.0);
    const Flock initial(config, 29);
    Flock sequentialFlock = initial;
    Flock coarseFlock = initial;

    SequentialScheme sequential;
    CoarseGrainedScheme coarse(2);
    sequential.simulateStep(sequentialFlock, config);
    const BoidsMetrics metrics = coarse.simulateStep(coarseFlock, config);

    return metrics.get_model() == execution_model::coarse_grained &&
           metrics.get_n_workers() == 2 &&
           !metrics.uses_virtual_workers() &&
           flocksMatch(sequentialFlock, coarseFlock, config.getBoidCount());
}

} // namespace

/**
 * @brief Ejecuta las pruebas mínimas sin depender de un framework externo.
 */
int main() {
    int failures = 0;
    const auto check = [&failures](bool condition, const std::string& name) {
        if (!condition) {
            std::cerr << "FALLO: " << name << '\n';
            ++failures;
        }
    };

    check(testObjectInterface(), "contrato object_interface");
    check(testFineGrainedPartialEquivalence(), "equivalencia fine-grained parcial");
    check(testFineGrainedFullEquivalence(), "equivalencia fine-grained completa");
    check(testFineGrainedTermination(), "terminacion del scheduler fine-grained");
    check(testFineGrainedSingleQuantum(), "quantum de un candidato");
    check(testFineGrainedRoundRobin(), "round-robin entre contextos");
    check(testFineGrainedEdgeCases(), "bordes fine-grained");
    check(testCoarseGrainedEquivalence(), "equivalencia coarse-grained");

    if (failures == 0) {
        std::cout << "Todas las pruebas de Boids pasaron.\n";
    }
    return failures == 0 ? 0 : 1;
}

#include <cmath>
#include <iostream>
#include <string>

#include "object_interface.hpp"
#include "core/Boid.hpp"
#include "core/BoidsMetrics.hpp"
#include "core/CoarseGrainedScheme.hpp"
#include "core/FineGrainedScheme.hpp"
#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/SequentialScheme.hpp"
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

/**
 * @brief Verifica que fine-grained produzca el baseline en su subconjunto.
 */
bool testFineGrainedEquivalence() {
    const FlockingConfig config(12, 200.0, 200.0, 50.0, 20.0,
                                2.6, 0.18, 1.0, 1.4, 0.8, 1.0);
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
           flocksMatch(sequentialFlock, fineFlock, 5);
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
    check(testFineGrainedEquivalence(), "equivalencia fine-grained");
    check(testCoarseGrainedEquivalence(), "equivalencia coarse-grained");

    if (failures == 0) {
        std::cout << "Todas las pruebas de Boids pasaron.\n";
    }
    return failures == 0 ? 0 : 1;
}

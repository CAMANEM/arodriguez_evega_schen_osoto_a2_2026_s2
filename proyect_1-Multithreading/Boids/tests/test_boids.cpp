#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "cli/CliOptions.hpp"
#include "object_interface.hpp"
#include "core/Boid.hpp"
#include "core/BoidsMetrics.hpp"
#include "core/CoarseGrainedScheme.hpp"
#include "core/FineGrainedScheme.hpp"
#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/SequentialScheme.hpp"
#include "core/SmtScheme.hpp"
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

CliOptions parseArgs(const std::vector<std::string>& args) {
    std::vector<std::string> storage;
    storage.reserve(args.size() + 1);
    storage.emplace_back("boids");
    storage.insert(storage.end(), args.begin(), args.end());

    std::vector<char*> argv;
    argv.reserve(storage.size());
    for (auto& token : storage) {
        argv.push_back(token.data());
    }
    return parseCli(static_cast<int>(argv.size()), argv.data());
}

/**
 * @brief El CLI selecciona N boids y el resto de variables del problema.
 */
bool testCliProblemParameters() {
    const CliOptions headless = parseArgs(
        {"--scheme", "fine", "--bodies", "33", "--perception", "70", "--separation", "22",
         "--seed", "9"});
    if (headless.scheme != RunScheme::Fine || headless.gui || headless.boidCount != 33 ||
        !nearlyEqual(headless.perceptionRadius, 70.0) ||
        !nearlyEqual(headless.separationRadius, 22.0) || headless.seed != 9) {
        return false;
    }

    const CliOptions equalsForm = parseArgs({"--scheme=fine", "--boids=41", "--partial=12"});
    if (equalsForm.boidCount != 41 || equalsForm.finePartialBoids != 12) {
        return false;
    }

    const CliOptions shortN = parseArgs({"-n", "15", "--scheme", "fine", "--gui",
                                         "--separation", "20", "--sep-weight", "1.0"});
    return shortN.gui && shortN.boidCount == 15 &&
           nearlyEqual(shortN.separationRadius, 20.0) &&
           nearlyEqual(shortN.separationWeight, 1.0);
}

/**
 * @brief Verifica que coarse-grained produzca el baseline completo (sin stalls).
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
           metrics.get_stall_count() == 0 &&
           flocksMatch(sequentialFlock, coarseFlock, config.getBoidCount());
}

/**
 * @brief Stalls inyectados no cambian el resultado numérico vs secuencial.
 */
bool testCoarseGrainedEquivalenceWithStalls() {
    const FlockingConfig config = makeTestConfig(16);
    const Flock initial(config, 41);

    Flock sequentialFlock = initial;
    SequentialScheme sequential;
    sequential.simulateStep(sequentialFlock, config);

    CoarseGrainedScheme::Options options;
    options.threadCount = 4;
    options.stallEveryBoids = 3;
    options.stallMilliseconds = 1.0;
    options.seed = 41;

    Flock coarseFlock = initial;
    CoarseGrainedScheme coarse(options);
    const BoidsMetrics metrics = coarse.simulateStep(coarseFlock, config);

    return metrics.get_stall_count() > 0 &&
           metrics.get_stall_time_ms() > 0.0 &&
           flocksMatch(sequentialFlock, coarseFlock, config.getBoidCount());
}

/**
 * @brief Stall aleatorio con semilla fija sigue siendo equivalente al secuencial.
 */
bool testCoarseGrainedRandomStallEquivalence() {
    const FlockingConfig config = makeTestConfig(10);
    const Flock initial(config, 7);

    Flock sequentialFlock = initial;
    SequentialScheme sequential;
    sequential.simulateStep(sequentialFlock, config);

    CoarseGrainedScheme::Options options;
    options.threadCount = 2;
    options.stallProbability = 0.5;
    options.stallMillisecondsMin = 0.5;
    options.stallMillisecondsMax = 1.5;
    options.seed = 99;

    Flock coarseFlock = initial;
    CoarseGrainedScheme coarse(options);
    const BoidsMetrics metrics = coarse.simulateStep(coarseFlock, config);

    return flocksMatch(sequentialFlock, coarseFlock, config.getBoidCount()) &&
           metrics.get_model() == execution_model::coarse_grained;
}

/**
 * @brief Bordes: 1 worker, más workers que boids, stalls desactivados.
 */
bool testCoarseGrainedEdgeCases() {
    const FlockingConfig config = makeTestConfig(5);
    const Flock initial(config, 13);

    Flock sequentialFlock = initial;
    SequentialScheme sequential;
    sequential.simulateStep(sequentialFlock, config);

    // 1 worker
    {
        Flock flock = initial;
        CoarseGrainedScheme coarse(1);
        const BoidsMetrics metrics = coarse.simulateStep(flock, config);
        if (metrics.get_n_workers() != 1 ||
            !flocksMatch(sequentialFlock, flock, config.getBoidCount())) {
            return false;
        }
    }

    // Más workers que boids
    {
        Flock flock = initial;
        CoarseGrainedScheme coarse(32);
        const BoidsMetrics metrics = coarse.simulateStep(flock, config);
        if (metrics.get_n_workers() != config.getBoidCount() ||
            !flocksMatch(sequentialFlock, flock, config.getBoidCount())) {
            return false;
        }
    }

    // Un solo boid
    {
        const FlockingConfig oneConfig = makeTestConfig(1);
        Flock sequentialOne(oneConfig, 4);
        Flock coarseOne = sequentialOne;
        sequential.simulateStep(sequentialOne, oneConfig);
        CoarseGrainedScheme::Options options;
        options.threadCount = 4;
        options.stallEveryBoids = 1;
        options.stallMilliseconds = 1.0;
        CoarseGrainedScheme coarse(options);
        const BoidsMetrics metrics = coarse.simulateStep(coarseOne, oneConfig);
        if (!flocksMatch(sequentialOne, coarseOne, 1) || metrics.get_stall_count() != 1) {
            return false;
        }
    }

    return true;
}

/**
 * @brief CLI parsea parámetros de stall de coarse.
 */
bool testCliCoarseStallParameters() {
    const CliOptions options = parseArgs(
        {"--scheme", "coarse", "--workers", "6", "--stall-every", "5",
         "--stall-probability", "0.25", "--stall-ms", "2.5", "--seed", "11",
         "--log-checkpoints"});
    if (options.scheme != RunScheme::Coarse || options.workers != 6 ||
        options.stallEvery != 5 || !nearlyEqual(options.stallProbability, 0.25) ||
        !nearlyEqual(options.stallMs, 2.5) || options.seed != 11 ||
        !options.logCoarseCheckpoints) {
        return false;
    }

    const CliOptions ranged = parseArgs(
        {"--scheme=coarse", "--stall-ms-min=1", "--stall-ms-max=3"});
    return nearlyEqual(ranged.stallMsMin, 1.0) && nearlyEqual(ranged.stallMsMax, 3.0);
}

/**
 * @brief Equivalencia SMT vs Sequential (misma seed/config).
 */
bool testSmtEquivalence() {
    const FlockingConfig config = makeTestConfig(24);
    const Flock initial(config, 53);
    Flock sequentialFlock = initial;
    Flock smtFlock = initial;

    SequentialScheme sequential;
    SmtScheme smt(2);
    sequential.simulateStep(sequentialFlock, config);
    const BoidsMetrics metrics = smt.simulateStep(smtFlock, config);

    return metrics.get_model() == execution_model::smt &&
           metrics.get_oversubscribe_factor() == 2u &&
           metrics.get_logical_processors() == SmtScheme::logicalProcessorCount() &&
           !metrics.uses_virtual_workers() &&
           metrics.get_stall_count() == 0 &&
           flocksMatch(sequentialFlock, smtFlock, config.getBoidCount());
}

/**
 * @brief Default F=2 pide T = 2*L; F=1 pide T = L (contraste vs CMP).
 */
bool testSmtOversubscribePolicy() {
    const unsigned int logical = SmtScheme::logicalProcessorCount();

    SmtScheme defaultFactor;
    if (defaultFactor.getOversubscriptionFactor() != 2u ||
        defaultFactor.requestedThreadCount() != logical * 2u) {
        return false;
    }

    SmtScheme factorOne(1);
    if (factorOne.getOversubscriptionFactor() != 1u ||
        factorOne.requestedThreadCount() != logical) {
        return false;
    }

    // Clamp de F=0 → 1
    SmtScheme clamped(0);
    if (clamped.getOversubscriptionFactor() != 1u ||
        clamped.requestedThreadCount() != logical) {
        return false;
    }

    // Con suficientes boids, workers efectivos = L*F.
    const int enoughBoids = static_cast<int>(logical * 4u);
    const FlockingConfig config = makeTestConfig(enoughBoids);
    Flock flock(config, 19);
    const BoidsMetrics metrics = defaultFactor.simulateStep(flock, config);
    return metrics.get_n_workers() == static_cast<int>(logical * 2u) &&
           metrics.get_oversubscribe_factor() == 2u;
}

/**
 * @brief Bordes: más hilos que boids; N=1; F=4 sigue equivalente.
 */
bool testSmtEdgeCases() {
    const FlockingConfig config = makeTestConfig(5);
    const Flock initial(config, 31);

    Flock sequentialFlock = initial;
    SequentialScheme sequential;
    sequential.simulateStep(sequentialFlock, config);

    // Más hilos pedidos que boids → no se lanzan workers vacíos.
    {
        Flock flock = initial;
        SmtScheme smt(64);
        const BoidsMetrics metrics = smt.simulateStep(flock, config);
        if (metrics.get_n_workers() != config.getBoidCount() ||
            !flocksMatch(sequentialFlock, flock, config.getBoidCount())) {
            return false;
        }
    }

    // Un solo boid
    {
        const FlockingConfig oneConfig = makeTestConfig(1);
        Flock sequentialOne(oneConfig, 4);
        Flock smtOne = sequentialOne;
        sequential.simulateStep(sequentialOne, oneConfig);
        SmtScheme smt(2);
        const BoidsMetrics metrics = smt.simulateStep(smtOne, oneConfig);
        if (metrics.get_n_workers() != 1 ||
            !flocksMatch(sequentialOne, smtOne, 1)) {
            return false;
        }
    }

    // F=4 sigue alineado al secuencial
    {
        Flock flock = initial;
        SmtScheme smt(4);
        const BoidsMetrics metrics = smt.simulateStep(flock, config);
        if (metrics.get_oversubscribe_factor() != 4u ||
            !flocksMatch(sequentialFlock, flock, config.getBoidCount())) {
            return false;
        }
    }

    // F=1: sin sobre-suscripción extra → T pedido = L (contraste conceptual vs CMP).
    {
        SmtScheme smt(1);
        if (smt.requestedThreadCount() != SmtScheme::logicalProcessorCount()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief CLI parsea --scheme smt y --oversubscribe (incluye clamp F=0).
 */
bool testCliSmtParameters() {
    const CliOptions defaults = parseArgs({"--scheme", "smt"});
    if (defaults.scheme != RunScheme::Smt || defaults.smtOversubscribe != 2u ||
        defaults.gui) {
        return false;
    }

    const CliOptions custom = parseArgs(
        {"--scheme=smt", "--oversubscribe=4", "--boids=80", "--no-gui"});
    if (custom.smtOversubscribe != 4u || custom.boidCount != 80 || custom.gui) {
        return false;
    }

    const CliOptions clamped = parseArgs({"--scheme", "smt", "--oversubscribe", "0"});
    return clamped.smtOversubscribe == 1u;
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
    check(testCliProblemParameters(), "CLI parametros del problema");
    check(testCoarseGrainedEquivalence(), "equivalencia coarse-grained");
    check(testCoarseGrainedEquivalenceWithStalls(), "equivalencia coarse con stalls");
    check(testCoarseGrainedRandomStallEquivalence(), "equivalencia coarse stall aleatorio");
    check(testCoarseGrainedEdgeCases(), "bordes coarse-grained");
    check(testCliCoarseStallParameters(), "CLI parametros stall coarse");
    check(testSmtEquivalence(), "equivalencia smt");
    check(testSmtOversubscribePolicy(), "politica oversubscribe smt");
    check(testSmtEdgeCases(), "bordes smt");
    check(testCliSmtParameters(), "CLI parametros smt");

    if (failures == 0) {
        std::cout << "Todas las pruebas de Boids pasaron.\n";
    }
    return failures == 0 ? 0 : 1;
}

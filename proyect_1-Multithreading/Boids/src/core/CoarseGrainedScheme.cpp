#include "core/CoarseGrainedScheme.hpp"

#include "core/FlockingRules.hpp"
#include "core/Timer.hpp"

#include <algorithm>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

CoarseGrainedScheme::CoarseGrainedScheme(unsigned int threadCount)
    : options_() {
    options_.threadCount = std::max(1u, threadCount);
}

CoarseGrainedScheme::CoarseGrainedScheme(const Options& options)
    : options_(options) {
    options_.threadCount = std::max(1u, options_.threadCount);
    StallPolicy::validateAndClamp(options_.stallEveryBoids, options_.stallProbability,
                                  options_.stallMilliseconds, options_.stallMillisecondsMin,
                                  options_.stallMillisecondsMax);
}

unsigned int CoarseGrainedScheme::computeThreadCount(const Flock& /*flock*/) const {
    return options_.threadCount;
}

void CoarseGrainedScheme::runWorker(int workerId, int startIndex, int endIndex,
                                    const Flock& flock, const FlockingConfig& config,
                                    std::vector<Vector2D>& steeringForces,
                                    WorkerStallStats& stats,
                                    CoarseWorkerCheckpoint* lastCheckpoint) const {
    // Semilla distinta por worker: paralelismo determinista con --seed.
    const std::uint32_t workerSeed =
        options_.seed + static_cast<std::uint32_t>(workerId) * 2654435761u;
    StallPolicy stallPolicy(options_.stallEveryBoids, options_.stallProbability,
                            options_.stallMilliseconds, options_.stallMillisecondsMin,
                            options_.stallMillisecondsMax, workerSeed);

    CoarseWorkerCheckpoint ctx;
    ctx.workerId = workerId;
    ctx.startIndex = startIndex;
    ctx.endIndex = endIndex;
    ctx.nextBoidIndex = startIndex;
    ctx.boidsCompleted = 0;
    ctx.stallCount = 0;
    ctx.rngState = stallPolicy.captureRngState();

    while (ctx.nextBoidIndex < ctx.endIndex) {
        const int boidIndex = ctx.nextBoidIndex;

        // el boid se calcula completo; el stall nunca parte vecinos.
        steeringForces[static_cast<std::size_t>(boidIndex)] =
            FlockingRules::computeSteeringForBoid(boidIndex, flock, config);

        ctx.nextBoidIndex = boidIndex + 1;
        ++ctx.boidsCompleted;

        if (!stallPolicy.shouldStallAfterBoid(ctx.boidsCompleted)) {
            continue;
        }

        // Duración decidida antes del snapshot para que el restore deje el RNG
        // en el estado post-decisión (sin re-sortear ni perder avances).
        const auto stallDuration = stallPolicy.nextStallDuration();

        // 1) Checkpoint MANUAL (didáctico).
        // En un sistema real, al bloquearse el hilo el sistema operativo conserva
        // automáticamente su contexto (stack, registros, PCB). Este checkpoint
        // manual no sustituye ese mecanismo: lo hace visible para demostración
        // académica del save/resume ante un stall costoso.
        ctx.rngState = stallPolicy.captureRngState();
        const CoarseWorkerCheckpoint checkpoint = ctx.snapshot();
        if (lastCheckpoint != nullptr) {
            *lastCheckpoint = checkpoint;
        }
        ++stats.checkpointsRecorded;

        if (options_.logCheckpoints) {
            std::cout << "[coarse-checkpoint] " << checkpoint.toString() << "\n";
        }

        // 2) Bloqueo costoso: el SO puede programar otro hilo listo.
        Timer stallTimer;
        stallTimer.start();
        std::this_thread::sleep_for(
            std::chrono::duration_cast<std::chrono::nanoseconds>(stallDuration));
        const double stalledMs = stallTimer.stopAndGetMilliseconds();

        // 3) Restore MANUAL del espejo académico; el SO ya reanudó el hilo.
        checkpoint.restoreInto(ctx);
        stallPolicy.restoreRngState(ctx.rngState);

        ++ctx.stallCount;
        ++stats.stallCount;
        stats.stallTimeMs += stalledMs;
    }
}

BoidsMetrics CoarseGrainedScheme::simulateStep(Flock& flock,
                                               const FlockingConfig& config) {
    const unsigned int threadCount = std::max(1u, computeThreadCount(flock));
    const int totalBoids = flock.getBoidCount();
    const int boidsPerThread =
        totalBoids == 0
            ? 0
            : (totalBoids + static_cast<int>(threadCount) - 1) / static_cast<int>(threadCount);

    Timer timer;
    timer.start();

    std::vector<Vector2D> steeringForces(static_cast<std::size_t>(totalBoids));
    std::vector<std::thread> workers;
    std::vector<WorkerStallStats> workerStats;
    std::vector<CoarseWorkerCheckpoint> lastCheckpoints;

    workers.reserve(threadCount);
    workerStats.resize(threadCount);
    if (options_.logCheckpoints) {
        lastCheckpoints.resize(threadCount);
    }

    for (unsigned int t = 0; t < threadCount; ++t) {
        const int startIndex = static_cast<int>(t) * boidsPerThread;
        const int endIndex = std::min(totalBoids, startIndex + boidsPerThread);
        if (startIndex >= endIndex) {
            break;
        }

        CoarseWorkerCheckpoint* checkpointSlot =
            options_.logCheckpoints ? &lastCheckpoints[t] : nullptr;

        workers.emplace_back(&CoarseGrainedScheme::runWorker, this,
                             static_cast<int>(t), startIndex, endIndex,
                             std::cref(flock), std::cref(config),
                             std::ref(steeringForces), std::ref(workerStats[t]),
                             checkpointSlot);
    }

    // join() es sincronización costosa del modelo de grano grueso.
    for (auto& worker : workers) {
        worker.join();
    }

    for (int i = 0; i < totalBoids; ++i) {
        flock.applyIntegration(i, steeringForces[static_cast<std::size_t>(i)], config);
    }

    const double elapsedMs = timer.stopAndGetMilliseconds();

    int totalStalls = 0;
    double totalStallTimeMs = 0.0;
    double maxWorkerStallTimeMs = 0.0;
    for (const WorkerStallStats& stats : workerStats) {
        totalStalls += stats.stallCount;
        totalStallTimeMs += stats.stallTimeMs;
        maxWorkerStallTimeMs = std::max(maxWorkerStallTimeMs, stats.stallTimeMs);
    }
    // computeTimeMs aproxima el cómputo en pared: los stalls de workers
    // distintos se solapan, así que se resta el máximo por worker, no la suma.
    const double computeTimeMs = std::max(0.0, elapsedMs - maxWorkerStallTimeMs);

    return BoidsMetrics(getExecutionModel(), getSchemeName(),
                        static_cast<int>(workers.size()), elapsedMs, totalBoids,
                        false, false, totalStalls, totalStallTimeMs, computeTimeMs);
}

std::string CoarseGrainedScheme::getSchemeName() const {
    if (options_.stallEveryBoids > 0 || options_.stallProbability > 0.0) {
        return "Grano Grueso (hilos reales + stalls didacticos)";
    }
    return "Grano Grueso (hilos tradicionales)";
}

execution_model CoarseGrainedScheme::getExecutionModel() const {
    return execution_model::coarse_grained;
}

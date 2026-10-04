#include "core/FineGrainedScheme.hpp"
#include "core/Timer.hpp"

#include <algorithm>

FineGrainedScheme::FineGrainedScheme(int partialBoidCount)
    : partialBoidCount_(partialBoidCount) {
}

int FineGrainedScheme::resolveContextCount(int flockSize, int partialBoidCount) {
    if (flockSize <= 0) {
        return 0;
    }
    if (partialBoidCount <= 0) {
        return flockSize;
    }
    return std::min(partialBoidCount, flockSize);
}

std::vector<SteeringContext> FineGrainedScheme::createContexts(const Flock& flock,
                                                              const FlockingConfig& config,
                                                              int contextCount) {
    std::vector<SteeringContext> contexts;
    contexts.reserve(contextCount);
    for (int i = 0; i < contextCount; ++i) {
        contexts.emplace_back(i, flock, config);
    }
    return contexts;
}

bool FineGrainedScheme::runOneRound(std::vector<SteeringContext>& contexts) {
    bool anyContextStillPending = false;
    for (auto& context : contexts) {
        if (!context.isFinished()) {
            context.stepOnce();
            if (context.hasPendingWork()) {
                anyContextStillPending = true;
            }
        }
    }
    return anyContextStillPending;
}

void FineGrainedScheme::runRoundRobinUntilDone(std::vector<SteeringContext>& contexts) {
    bool anyContextActive = !contexts.empty();
    while (anyContextActive) {
        anyContextActive = runOneRound(contexts);
    }
}

void FineGrainedScheme::applyIntegrations(Flock& flock, const FlockingConfig& config,
                                          const std::vector<SteeringContext>& contexts) {
    for (const auto& context : contexts) {
        flock.applyIntegration(context.getBoidIndex(), context.computeFinalSteering(), config);
    }
}

BoidsMetrics FineGrainedScheme::simulateStep(Flock& flock, const FlockingConfig& config) {
    Timer timer;
    timer.start();

    const int contextCount = resolveContextCount(flock.getBoidCount(), partialBoidCount_);
    std::vector<SteeringContext> contexts = createContexts(flock, config, contextCount);

    runRoundRobinUntilDone(contexts);
    applyIntegrations(flock, config, contexts);

    const double elapsedMs = timer.stopAndGetMilliseconds();
    const bool isPartial = contextCount < flock.getBoidCount();
    return BoidsMetrics(getExecutionModel(), getSchemeName(), contextCount, elapsedMs,
                        contextCount, true, isPartial);
}

std::string FineGrainedScheme::getSchemeName() const {
    return "Grano Fino (round-robin por vecino, 1 hilo SO)";
}

execution_model FineGrainedScheme::getExecutionModel() const {
    return execution_model::fine_grained;
}

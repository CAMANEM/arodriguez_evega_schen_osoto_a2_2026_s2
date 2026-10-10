#include "core/FineGrainedSphScheme.hpp"

#include "core/SphForces.hpp"

#include <algorithm>
#include <chrono>
#include <utility>
#include <vector>
#include <tuple>

namespace {

struct DensityThread {
    int particleIndex;
    int nextNeighbor;
    double density;
    bool finished;
};

} // namespace

FineGrainedSphScheme::FineGrainedSphScheme(int contextCount,
                                           TraceCallback traceCallback)
    : contextCount_(std::max(1, contextCount)),
      traceCallback_(std::move(traceCallback)) {}

SphMetrics FineGrainedSphScheme::simulateStep(SphFluid& fluid,
                                               const SphConfig& config) {
    const int particleCount = fluid.getParticleCount();
    const SphNeighborList neighbors = SphForces::findNeighbors(fluid, config);
    const auto densityStart = std::chrono::steady_clock::now();
    const int activeContextCount = std::min(contextCount_, particleCount);
    std::vector<DensityThread> threads;
    threads.reserve(static_cast<std::size_t>(activeContextCount));

    for (int contextIndex = 0; contextIndex < activeContextCount;
         ++contextIndex) {
        threads.push_back({contextIndex, 0, 0.0, false});
    }

    int remainingContexts = activeContextCount;
    int nextParticle = activeContextCount;
    std::vector<std::tuple<int, int, int, int>> traceEvents;
    while (remainingContexts > 0) {
        for (int contextIndex = 0; contextIndex < activeContextCount;
             ++contextIndex) {
            DensityThread& thread = threads[static_cast<std::size_t>(
                contextIndex)];
            if (thread.finished) {
                continue;
            }

            const std::vector<int>& particleNeighbors =
                neighbors.at(static_cast<std::size_t>(thread.particleIndex));
            const int neighborParticleIndex = particleNeighbors.at(
                static_cast<std::size_t>(thread.nextNeighbor));
            thread.density += SphForces::computeDensityContribution(
                thread.particleIndex,
                particleNeighbors.at(static_cast<std::size_t>(
                    thread.nextNeighbor)),
                fluid, config);
            if (traceCallback_) {
                traceEvents.emplace_back(contextIndex, thread.particleIndex,
                                         neighborParticleIndex,
                                         remainingContexts);
            }
            ++thread.nextNeighbor;
            if (thread.nextNeighbor >= static_cast<int>(
                                            particleNeighbors.size())) {
                thread.density = std::max(thread.density, 1e-12);
                fluid.getParticle(thread.particleIndex).setDensity(
                    thread.density);
                if (nextParticle < particleCount) {
                    thread.particleIndex = nextParticle++;
                    thread.nextNeighbor = 0;
                    thread.density = 0.0;
                } else {
                    thread.finished = true;
                    --remainingContexts;
                }
            }
        }
    }

    const auto densityEnd = std::chrono::steady_clock::now();
    if (traceCallback_) {
        for (const auto& event : traceEvents) {
            traceCallback_(std::get<0>(event), std::get<1>(event),
                           std::get<2>(event), std::get<3>(event));
        }
    }
    // These phases are intentionally sequential in this iteration. The assigned
    // fine-grained work is the density phase, while later phases remain visible
    // and comparable with the sequential implementation.
    SphForces::computeAllPressures(fluid, config);
    const std::vector<SphForceData> forces =
        SphForces::computeAllForces(fluid, config, neighbors);
    SphForces::applyForcesAndIntegrate(fluid, config, forces);

    const double elapsedSeconds = std::chrono::duration<double>(
        densityEnd - densityStart).count();
    return SphMetrics(getExecutionModel(), getSchemeName(), activeContextCount,
                      particleCount, elapsedSeconds);
}

std::string FineGrainedSphScheme::getSchemeName() const {
    return "Fine-grained SPH (density)";
}

execution_model FineGrainedSphScheme::getExecutionModel() const {
    return execution_model::fine_grained;
}

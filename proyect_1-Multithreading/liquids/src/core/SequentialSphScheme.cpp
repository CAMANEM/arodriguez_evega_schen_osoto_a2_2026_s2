#include "core/SequentialSphScheme.hpp"

#include "core/SphForces.hpp"

#include <chrono>
#include <vector>

SphMetrics SequentialSphScheme::simulateStep(SphFluid& fluid,
                                              const SphConfig& config) {
    const SphNeighborList neighbors = SphForces::findNeighbors(fluid, config);
    const auto densityStart = std::chrono::steady_clock::now();
    for (int index = 0; index < fluid.getParticleCount(); ++index) {
        fluid.getParticle(index).setDensity(
            SphForces::computeDensityForParticle(index, fluid, config,
                                                 neighbors));
    }
    const auto densityEnd = std::chrono::steady_clock::now();
    SphForces::computeAllPressures(fluid, config);
    const std::vector<SphForceData> forces =
        SphForces::computeAllForces(fluid, config, neighbors);
    SphForces::applyForcesAndIntegrate(fluid, config, forces);
    const double elapsedSeconds = std::chrono::duration<double>(
        densityEnd - densityStart).count();
    return SphMetrics(getExecutionModel(), getSchemeName(), 1,
                      fluid.getParticleCount(), elapsedSeconds);
}

std::string SequentialSphScheme::getSchemeName() const {
    return "Sequential SPH";
}

execution_model SequentialSphScheme::getExecutionModel() const {
    return execution_model::sequential;
}

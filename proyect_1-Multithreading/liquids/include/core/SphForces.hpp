#ifndef SPH_FORCES_HPP
#define SPH_FORCES_HPP

#include <vector>

#include "core/SphConfig.hpp"
#include "core/SphFluid.hpp"

struct SphForceData {
    double forceX;
    double forceY;
};

using SphNeighborList = std::vector<std::vector<int>>;

class SphForces {
public:
    static SphNeighborList findNeighbors(const SphFluid& fluid,
                                         const SphConfig& config);
    static double computeDensityContribution(int particleIndex,
                                             int neighborIndex,
                                             const SphFluid& fluid,
                                             const SphConfig& config);
    static double computeDensityForParticle(int particleIndex,
                                            const SphFluid& fluid,
                                            const SphConfig& config,
                                            const SphNeighborList& neighbors);
    static void computeAllDensities(SphFluid& fluid, const SphConfig& config,
                                    const SphNeighborList& neighbors);
    static void computeAllPressures(SphFluid& fluid, const SphConfig& config);
    static std::vector<SphForceData> computeAllForces(const SphFluid& fluid,
                                                      const SphConfig& config,
                                                      const SphNeighborList& neighbors);
    static void applyForcesAndIntegrate(SphFluid& fluid,
                                        const SphConfig& config,
                                        const std::vector<SphForceData>& forces);
    static void applyBoundary(SphFluid& fluid, const SphConfig& config);
};

#endif

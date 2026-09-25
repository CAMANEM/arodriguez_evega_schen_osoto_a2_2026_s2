#include <cmath>
#include <iostream>

#include "core/SphConfig.hpp"
#include "core/SphFluid.hpp"
#include "core/SphParticle.hpp"
#include "core/SphMetrics.hpp"
#include "object_interface.hpp"

int main() {
    const SphConfig config(8, 10, 0.04, 0.001, 1.0, 1000.0, 2000.0,
                           0.1, -9.81, 2.0, 3.0,
                           execution_model::coarse_grained, 4, 17);
    SphFluid fluid(config);

    if (fluid.getParticleCount() != 8) {
        return 1;
    }

    object_interface* particle = &fluid.getParticle(0);
    particle->set_force(1.0, 0.0);
    particle->update(config.getDeltaTime());
    fluid.reset();

    if (std::abs(particle->get_force_x()) > 1e-12 ||
        std::abs(particle->get_force_y()) > 1e-12) {
        return 1;
    }

    const SphMetrics metrics(execution_model::sequential, "sequential", 1, 8, 0.5);
    if (metrics.get_run_count() != 1 || metrics.getParticlesProcessed() != 8) {
        return 1;
    }

    std::cout << "Liquids interface tests passed.\n";
    return 0;
}

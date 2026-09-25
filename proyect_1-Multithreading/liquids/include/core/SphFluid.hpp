#ifndef SPH_FLUID_HPP
#define SPH_FLUID_HPP

#include <vector>

#include "core/SphConfig.hpp"
#include "core/SphParticle.hpp"

class SphFluid {
public:
    explicit SphFluid(const SphConfig& config);

    int getParticleCount() const;
    const SphParticle& getParticle(int index) const;
    SphParticle& getParticle(int index);
    void reset();

private:
    std::vector<SphParticle> particles_;
};

#endif

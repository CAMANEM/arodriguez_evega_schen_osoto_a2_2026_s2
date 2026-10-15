#include "core/StallPolicy.hpp"

#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace {

double unitFromEngine(std::mt19937& rng) {
    constexpr double scale =
        1.0 / (static_cast<double>(std::mt19937::max()) + 1.0);
    return static_cast<double>(rng()) * scale;
}

} // namespace

StallPolicy::StallPolicy(int stallEveryBoids,
                         double stallProbability,
                         double stallMilliseconds,
                         double stallMillisecondsMin,
                         double stallMillisecondsMax,
                         std::uint32_t seed)
    : stallEveryBoids_(stallEveryBoids),
      stallProbability_(stallProbability),
      stallMilliseconds_(stallMilliseconds),
      stallMillisecondsMin_(stallMillisecondsMin),
      stallMillisecondsMax_(stallMillisecondsMax),
      rng_(seed),
      useRandomDuration_(false),
      seed_(seed),
      advanceCount_(0) {
    validateAndClamp(stallEveryBoids_, stallProbability_, stallMilliseconds_,
                     stallMillisecondsMin_, stallMillisecondsMax_);
    useRandomDuration_ = hasRandomDuration();
}

bool StallPolicy::hasRandomDuration() const {
    return stallMillisecondsMin_ >= 0.0 && stallMillisecondsMax_ >= 0.0;
}

bool StallPolicy::isEnabled() const {
    return stallEveryBoids_ > 0 || stallProbability_ > 0.0;
}

double StallPolicy::drawUnitInterval() {
    ++advanceCount_;
    return unitFromEngine(rng_);
}

bool StallPolicy::shouldStallAfterBoid(int boidsCompleted) {
    if (boidsCompleted <= 0) {
        return false;
    }

    bool triggered = false;
    if (stallEveryBoids_ > 0 && (boidsCompleted % stallEveryBoids_) == 0) {
        triggered = true;
    }
    if (stallProbability_ > 0.0 && drawUnitInterval() < stallProbability_) {
        triggered = true;
    }
    return triggered;
}

StallPolicy::ClockDuration StallPolicy::nextStallDuration() {
    if (useRandomDuration_) {
        const double unit = drawUnitInterval();
        const double span = stallMillisecondsMax_ - stallMillisecondsMin_;
        return ClockDuration(stallMillisecondsMin_ + unit * span);
    }
    return ClockDuration(stallMilliseconds_);
}

std::uint64_t StallPolicy::captureRngState() const {
    return advanceCount_;
}

void StallPolicy::restoreRngState(std::uint64_t state) {
    rng_.seed(seed_);
    if (state > 0) {
        rng_.discard(state);
    }
    advanceCount_ = state;
}

void StallPolicy::validateAndClamp(int& stallEveryBoids,
                                   double& stallProbability,
                                   double& stallMilliseconds,
                                   double& stallMillisecondsMin,
                                   double& stallMillisecondsMax) {
    if (stallEveryBoids < 0) {
        stallEveryBoids = 0;
    }
    stallProbability = std::clamp(stallProbability, 0.0, 1.0);
    if (stallMilliseconds < 0.0) {
        stallMilliseconds = 0.0;
    }

    const bool minSet = stallMillisecondsMin >= 0.0;
    const bool maxSet = stallMillisecondsMax >= 0.0;
    if (minSet != maxSet) {
        throw std::runtime_error(
            "Debe indicar ambos --stall-ms-min y --stall-ms-max, o ninguno");
    }
    if (minSet && maxSet && stallMillisecondsMin > stallMillisecondsMax) {
        throw std::runtime_error(
            "--stall-ms-min no puede ser mayor que --stall-ms-max");
    }
}

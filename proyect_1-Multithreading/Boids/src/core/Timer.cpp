#include "core/Timer.hpp"

void Timer::start() {
    startTime_ = std::chrono::high_resolution_clock::now();
}

double Timer::stopAndGetMilliseconds() {
    const auto endTime = std::chrono::high_resolution_clock::now();
    const std::chrono::duration<double, std::milli> elapsed = endTime - startTime_;
    return elapsed.count();
}

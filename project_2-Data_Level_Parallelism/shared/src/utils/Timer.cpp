#include "utils/Timer.hpp"

void Timer::start() {
    startTime_ = std::chrono::high_resolution_clock::now();
}

double Timer::stopAndGetMilliseconds() {
    const auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - startTime_).count();
}

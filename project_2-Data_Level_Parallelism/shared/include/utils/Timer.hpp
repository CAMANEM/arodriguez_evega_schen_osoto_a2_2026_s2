#pragma once
#include <chrono>

/** @brief Cronómetro de pared de alta resolución. */
class Timer {
public:
    void   start();
    /** @return Milisegundos transcurridos desde start(). */
    double stopAndGetMilliseconds();
private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

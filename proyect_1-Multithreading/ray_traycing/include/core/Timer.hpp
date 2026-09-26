#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

class Timer {
public:
    void start();
    double stopAndGetMilliseconds();

private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

#endif // TIMER_HPP
#include "../include/stall_policy.hpp"
#include <random>

StallPolicy& StallPolicy::instance() {
    static StallPolicy instance;
    return instance;
}

bool StallPolicy::check() const {
    // Every thread has its own random number generator.
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng) < STALL_PROBABILITY;
}

long long StallPolicy::get_stall_penalization() const {
    return STALL_PENALIZATION_NS;
}

#include "../include/stall_policy.hpp"
#include <random>

StallPolicy& StallPolicy::instance() {
    static StallPolicy instance;
    return instance;
}

bool StallPolicy::check() {
    // Every thread has its own random number generator.
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
    bool stall = dist(rng) < STALL_PROBABILITY;
    if (stall) {
        current_stall_count.fetch_add(1, std::memory_order_relaxed);
    }
    return stall;
}

long long StallPolicy::get_stall_penalization() const {
    return STALL_PENALIZATION_NS;
}

int StallPolicy::get_stall_count() const {
    return current_stall_count.load(std::memory_order_relaxed);
}

void StallPolicy::reset_stall_count() {
    current_stall_count.store(0, std::memory_order_relaxed);
}

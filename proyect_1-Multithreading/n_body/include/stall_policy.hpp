#pragma once
#include <atomic>

// Singleton that models probabilistic memory stalls shared across all execution modes.
class StallPolicy {
public:
    static StallPolicy& instance();

    bool check();
    long long get_stall_penalization() const;
    int get_stall_count() const;
    void reset_stall_count();

private:
    StallPolicy() = default;

    static constexpr double STALL_PROBABILITY = 0.05;
    static constexpr long long STALL_PENALIZATION_NS = 3200;

    std::atomic<int> current_stall_count{0};
};

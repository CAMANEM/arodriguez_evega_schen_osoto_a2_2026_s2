#pragma once

// Singleton that models probabilistic memory stalls shared across all execution modes.
class StallPolicy {
public:
    static StallPolicy& instance();

    // Returns true if a stall should happen.
    bool check() const;

    // Returns the stall penalization in nanoseconds.
    long long get_stall_penalization() const;

private:
    StallPolicy() = default;

    static constexpr double STALL_PROBABILITY = 0.05;
    static constexpr long long STALL_PENALIZATION_NS = 3200;
};

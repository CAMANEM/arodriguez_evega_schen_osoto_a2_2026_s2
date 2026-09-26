#include "../include/metrics_interface.hpp"

#include <cmath>
#include <cstddef>

/* ===== CONSTRUCTOR =========================================================================== */
metrics_interface::metrics_interface(execution_model model, int n_workers)
    : model(model), n_workers(n_workers), sequential_time(0.0)
{}

/* ===== OTHER METHODS ========================================================================= */
void metrics_interface::record_time(double seconds) {
    run_times.push_back(seconds);
}

void metrics_interface::reset() {
    run_times.clear();
}

double metrics_interface::mean_time() const {
    if (run_times.empty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (std::size_t i = 0; i < run_times.size(); ++i) {
        sum += run_times[i];
    }
    return sum / static_cast<double>(run_times.size());
}

double metrics_interface::stddev_time() const {
    const std::size_t n = run_times.size();
    if (n < 2) {
        return 0.0;
    }
    const double mean = mean_time();
    double acc = 0.0;
    for (std::size_t i = 0; i < n; ++i) {
        const double d = run_times[i] - mean;
        acc += d * d;
    }
    return std::sqrt(acc / static_cast<double>(n - 1));
}

double metrics_interface::speedup() const {
    const double parallel = mean_time();
    if (parallel <= 0.0 || sequential_time <= 0.0) {
        return 0.0;
    }
    return sequential_time / parallel;
}

double metrics_interface::efficiency() const {
    if (n_workers <= 0) {
        return 0.0;
    }
    return speedup() / static_cast<double>(n_workers);
}

double metrics_interface::ci95_lower() const {
    return mean_time() - ci95_half_width();
}

double metrics_interface::ci95_upper() const {
    return mean_time() + ci95_half_width();
}

double metrics_interface::ci95_half_width() const {
    const std::size_t n = run_times.size();
    if (n < 2) {
        return 0.0;
    }
    return 1.96 * stddev_time() / std::sqrt(static_cast<double>(n));
}

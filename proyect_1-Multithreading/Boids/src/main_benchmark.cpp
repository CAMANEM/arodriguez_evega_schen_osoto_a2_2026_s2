#include "cli/BenchmarkCli.hpp"
#include "cli/SchemeFactory.hpp"
#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/FlockingScheme.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

const std::vector<std::string> allModels = {
    "sequential", "fine", "coarse", "smt", "cmp"};

struct RawSample {
    std::string model;
    std::string phase;
    int run = 0;
    int effectiveWorkers = 1;
    double seconds = 0.0;
};

/**
 * @brief Runs one flock for options.steps and returns wall seconds plus workers.
 * @param options Campaign CLI options.
 * @param model Canonical model name.
 * @return Pair of (wall_seconds, effective_workers) for the whole run.
 */
std::pair<double, int> executeOnce(const BenchmarkCliOptions& options,
                                   const std::string& model) {
    const CliOptions schemeOptions = toSchemeCliOptions(options, model);
    const FlockingConfig config = schemeOptions.toConfig();
    Flock flock(config, static_cast<unsigned int>(options.seed));
    std::unique_ptr<FlockingScheme> scheme = createScheme(schemeOptions);

    double totalSeconds = 0.0;
    int workers = 1;
    for (int step = 0; step < options.steps; ++step) {
        const auto metrics = scheme->simulateStep(flock, config);
        totalSeconds += metrics.mean_time();
        workers = metrics.get_n_workers();
    }
    return {totalSeconds, workers};
}

/**
 * @brief Warmup then measured runs; only measured samples enter summary stats.
 * @param model Canonical model name.
 * @param options Campaign CLI options.
 * @param rawSamples Optional raw CSV sink.
 * @return Aggregated metrics_interface for measured runs only.
 */
metrics_interface runModel(const std::string& model,
                           const BenchmarkCliOptions& options,
                           std::vector<RawSample>* rawSamples) {
    metrics_interface samples(parseBenchmarkModel(model), options.workers);

    for (int run = 0; run < options.warmup; ++run) {
        const auto timed = executeOnce(options, model);
        if (rawSamples != nullptr) {
            rawSamples->push_back(
                {model, "warmup", run, timed.second, timed.first});
        }
    }

    for (int run = 0; run < options.runs; ++run) {
        const auto timed = executeOnce(options, model);
        samples.set_n_workers(timed.second);
        samples.record_time(timed.first);
        if (rawSamples != nullptr) {
            rawSamples->push_back(
                {model, "measured", run, timed.second, timed.first});
        }
    }
    return samples;
}

void writeCsv(const std::string& path, const BenchmarkCliOptions& options,
              const std::vector<std::pair<std::string, metrics_interface>>& results) {
    const std::filesystem::path output(path);
    if (output.has_parent_path()) {
        std::filesystem::create_directories(output.parent_path());
    }
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("could not open CSV output: " + path);
    }

    file << "model,workload_n,steps,effective_workers,quantum,smt_factor,seed,"
            "runs,warmup,mean_s,stddev_s,ci95_lo,ci95_hi,speedup,efficiency\n";
    for (const auto& result : results) {
        const metrics_interface& metrics = result.second;
        file << result.first << ',' << options.boids << ',' << options.steps
             << ',' << metrics.get_n_workers() << ",1," << options.smtFactor
             << ',' << options.seed << ',' << metrics.get_run_count() << ','
             << options.warmup << ',' << std::setprecision(12)
             << metrics.mean_time() << ',' << metrics.stddev_time() << ','
             << metrics.ci95_lower() << ',' << metrics.ci95_upper() << ','
             << metrics.speedup() << ',' << metrics.efficiency() << '\n';
    }
}

void writeRawCsv(const std::string& path, const BenchmarkCliOptions& options,
                 const std::vector<RawSample>& samples) {
    if (path.empty()) {
        return;
    }
    const std::filesystem::path output(path);
    if (output.has_parent_path()) {
        std::filesystem::create_directories(output.parent_path());
    }
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("could not open raw CSV output: " + path);
    }
    file << "model,run_index,phase,workload_n,steps,effective_workers,seconds\n";
    for (const RawSample& sample : samples) {
        file << sample.model << ',' << sample.run << ',' << sample.phase << ','
             << options.boids << ',' << options.steps << ','
             << sample.effectiveWorkers << ',' << std::setprecision(12)
             << sample.seconds << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const BenchmarkCliOptions options = parseBenchmarkCli(argc, argv);
        const std::vector<std::string> selected =
            options.model == "all" ? allModels
                                   : std::vector<std::string>{options.model};

        std::vector<RawSample> rawSamples;
        const metrics_interface baseline =
            runModel("sequential", options, &rawSamples);

        std::vector<std::pair<std::string, metrics_interface>> results;
        for (const std::string& model : selected) {
            metrics_interface samples =
                model == "sequential"
                    ? baseline
                    : runModel(model, options, &rawSamples);
            samples.set_sequential_time(baseline.mean_time());
            results.emplace_back(model, std::move(samples));
        }

        writeCsv(options.output, options, results);
        writeRawCsv(options.rawOutput, options, rawSamples);
        std::cout << "Boids benchmark completed\n"
                  << "models: " << options.model << "\n"
                  << "warmup: " << options.warmup << "\n"
                  << "runs: " << options.runs << "\n"
                  << "CSV: " << options.output << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

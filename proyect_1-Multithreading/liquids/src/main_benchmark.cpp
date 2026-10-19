#include "cli/SphCli.hpp"
#include "cli/SphSchemeFactory.hpp"
#include "core/SphConfig.hpp"
#include "core/SphFluid.hpp"
#include "core/SphMetrics.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

const std::vector<std::string> allModels = {
    "sequential", "fine", "coarse", "smt", "cmp"};

struct StageSamples {
    metrics_interface total;
    std::map<SphStage, metrics_interface> stages;
};

struct RawSample {
    std::string model;
    std::string stage;
    std::string phase;
    int run = 0;
    int effectiveWorkers = 1;
    double seconds = 0.0;
};

/**
 * @brief Builds an SphConfig for one execution model from campaign CLI options.
 * @param options Parsed benchmark CLI options.
 * @param model Execution model to embed in the config.
 * @return Config with sequential forced to a single worker thread.
 */
SphConfig makeConfig(const SphCliOptions& options, execution_model model) {
    const int threadCount =
        model == execution_model::sequential ? 1 : options.threads;
    return SphConfig(options.particles, options.steps, options.smoothingLength,
                     options.deltaTime, 1.0, 1000.0, 2000.0, 0.1, -9.81,
                     1.0, 1.0, model, threadCount, options.seed);
}

/**
 * @brief Runs one simulation and appends raw stage samples for the given phase.
 * @param modelName Canonical model name (sequential|fine|...).
 * @param options Campaign CLI options.
 * @param config Simulation config for this model.
 * @param runIndex Zero-based index within the warmup or measured series.
 * @param phase Either "warmup" or "measured".
 * @param rawSamples Optional sink for per-run CSV rows.
 * @return Aggregated SphMetrics for this single run.
 */
SphMetrics executeOnce(const std::string& modelName,
                       const SphCliOptions& options,
                       const SphConfig& config,
                       int runIndex,
                       const std::string& phase,
                       std::vector<RawSample>* rawSamples) {
    SphFluid fluid(config);
    std::unique_ptr<SphScheme> scheme = makeSphScheme(modelName, options);
    const SphMetrics metrics = scheme->simulate(fluid, config);
    if (rawSamples != nullptr) {
        rawSamples->push_back(
            {modelName, "total", phase, runIndex, metrics.get_n_workers(),
             metrics.getStageMetrics().totalWallSeconds});
        for (const SphStageMetrics& stage : metrics.getStageMetrics().perStage) {
            rawSamples->push_back(
                {modelName, sphStageName(stage.stage), phase, runIndex,
                 metrics.get_n_workers(), stage.wallSeconds});
        }
    }
    return metrics;
}

/**
 * @brief Runs warmup (discarded from summary) then measured repetitions.
 * @param modelName Canonical model name.
 * @param options Campaign CLI options including warmup and runs.
 * @param config Simulation config for this model.
 * @param rawSamples Optional sink for raw CSV rows (warmup and measured).
 * @return StageSamples with only measured runs recorded for statistics.
 */
StageSamples runModel(const std::string& modelName,
                      const SphCliOptions& options,
                      const SphConfig& config,
                      std::vector<RawSample>* rawSamples) {
    StageSamples samples{
        metrics_interface(parseSphModel(modelName), options.threads), {}};

    for (int run = 0; run < options.warmup; ++run) {
        (void)executeOnce(modelName, options, config, run, "warmup", rawSamples);
    }

    for (int run = 0; run < options.runs; ++run) {
        const SphMetrics metrics =
            executeOnce(modelName, options, config, run, "measured", rawSamples);
        samples.total.set_n_workers(metrics.get_n_workers());
        samples.total.record_time(metrics.getStageMetrics().totalWallSeconds);
        for (const SphStageMetrics& stage : metrics.getStageMetrics().perStage) {
            auto found = samples.stages.find(stage.stage);
            if (found == samples.stages.end()) {
                found = samples.stages.emplace(
                    stage.stage,
                    metrics_interface(parseSphModel(modelName),
                                       metrics.get_n_workers())).first;
            }
            found->second.record_time(stage.wallSeconds);
        }
    }
    return samples;
}

void setBaseline(StageSamples& samples, const StageSamples& baseline) {
    samples.total.set_sequential_time(baseline.total.mean_time());
    for (auto& entry : samples.stages) {
        const auto found = baseline.stages.find(entry.first);
        if (found != baseline.stages.end()) {
            entry.second.set_sequential_time(found->second.mean_time());
        }
    }
}

void writeCsv(const std::string& path, const SphCliOptions& options,
              const std::vector<std::pair<std::string, StageSamples>>& results) {
    const std::filesystem::path output(path);
    if (output.has_parent_path()) {
        std::filesystem::create_directories(output.parent_path());
    }
    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error("could not open CSV output: " + path);
    }

    file << "model,stage,workload_n,steps,effective_workers,quantum,smt_factor,"
            "seed,runs,warmup,mean_s,stddev_s,ci95_lo,ci95_hi,speedup,"
            "efficiency\n";
    for (const auto& result : results) {
        const StageSamples& samples = result.second;
        auto writeRow = [&](const std::string& stage,
                            const metrics_interface& metrics) {
            file << result.first << ',' << stage << ',' << options.particles
                 << ',' << options.steps << ',' << metrics.get_n_workers()
                 << ',' << options.quantum << ',' << options.smtFactor << ','
                 << options.seed << ',' << metrics.get_run_count() << ','
                 << options.warmup << ','
                 << std::setprecision(12) << metrics.mean_time() << ','
                 << metrics.stddev_time() << ',' << metrics.ci95_lower()
                 << ',' << metrics.ci95_upper() << ',' << metrics.speedup()
                 << ',' << metrics.efficiency() << '\n';
        };
        writeRow("total", samples.total);
        for (const auto& stage : samples.stages) {
            writeRow(sphStageName(stage.first), stage.second);
        }
    }
}

void writeRawCsv(const std::string& path, const SphCliOptions& options,
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
    file << "model,stage,run_index,phase,workload_n,steps,requested_threads,"
            "effective_workers,quantum,chunk,"
            "smt_factor,seed,seconds\n";
    for (const RawSample& sample : samples) {
        file << sample.model << ',' << sample.stage << ',' << sample.run << ','
             << sample.phase << ',' << options.particles << ',' << options.steps
             << ',' << options.threads << ',' << sample.effectiveWorkers << ','
             << options.quantum << ',' << options.chunk << ','
             << options.smtFactor << ',' << options.seed << ','
             << std::setprecision(12) << sample.seconds << '\n';
    }
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const SphCliOptions options = parseSphCli(argc, argv);
        const std::vector<std::string> selected =
            options.model == "all" ? allModels
                                   : std::vector<std::string>{options.model};

        const SphConfig baselineConfig =
            makeConfig(options, execution_model::sequential);
        std::vector<RawSample> rawSamples;
        const StageSamples baseline =
            runModel("sequential", options, baselineConfig, &rawSamples);

        std::vector<std::pair<std::string, StageSamples>> results;
        for (const std::string& model : selected) {
            const execution_model executionModel = parseSphModel(model);
            const SphConfig config = makeConfig(options, executionModel);
            StageSamples samples = model == "sequential"
                                       ? baseline
                                       : runModel(model, options, config,
                                                  &rawSamples);
            setBaseline(samples, baseline);
            results.emplace_back(model, std::move(samples));
        }

        writeCsv(options.output, options, results);
        writeRawCsv(options.rawOutput, options, rawSamples);
        std::cout << "SPH benchmark completed\n"
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

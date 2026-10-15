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

SphConfig makeConfig(const SphCliOptions& options, execution_model model) {
    const int threadCount =
        model == execution_model::sequential ? 1 : options.threads;
    return SphConfig(options.particles, options.steps, options.smoothingLength,
                     options.deltaTime, 1.0, 1000.0, 2000.0, 0.1, -9.81,
                     1.0, 1.0, model, threadCount, options.seed);
}

StageSamples runModel(const std::string& modelName,
                      const SphCliOptions& options,
                      const SphConfig& config) {
    StageSamples samples{
        metrics_interface(parseSphModel(modelName), options.threads), {}};

    for (int run = 0; run < options.runs; ++run) {
        SphFluid fluid(config);
        std::unique_ptr<SphScheme> scheme =
            makeSphScheme(modelName, options);
        const SphMetrics metrics = scheme->simulate(fluid, config);
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

    file << "model,stage,particles,steps,threads,runs,mean_seconds,"
            "stddev_seconds,ci95_lower_seconds,ci95_upper_seconds,speedup,"
            "efficiency\n";
    for (const auto& result : results) {
        const StageSamples& samples = result.second;
        auto writeRow = [&](const std::string& stage,
                            const metrics_interface& metrics) {
            file << result.first << ',' << stage << ',' << options.particles
                 << ',' << options.steps << ',' << metrics.get_n_workers()
                 << ',' << metrics.get_run_count() << ','
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

} // namespace

int main(int argc, char* argv[]) {
    try {
        const SphCliOptions options = parseSphCli(argc, argv);
        const std::vector<std::string> selected =
            options.model == "all" ? allModels
                                   : std::vector<std::string>{options.model};

        const SphConfig baselineConfig =
            makeConfig(options, execution_model::sequential);
        const StageSamples baseline =
            runModel("sequential", options, baselineConfig);

        std::vector<std::pair<std::string, StageSamples>> results;
        for (const std::string& model : selected) {
            const execution_model executionModel = parseSphModel(model);
            const SphConfig config = makeConfig(options, executionModel);
            StageSamples samples = model == "sequential"
                                       ? baseline
                                       : runModel(model, options, config);
            setBaseline(samples, baseline);
            results.emplace_back(model, std::move(samples));
        }

        writeCsv(options.output, options, results);
        std::cout << "SPH benchmark completed\n"
                  << "models: " << options.model << "\n"
                  << "runs: " << options.runs << "\n"
                  << "CSV: " << options.output << "\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

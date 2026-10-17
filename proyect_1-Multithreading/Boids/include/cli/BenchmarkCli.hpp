#ifndef BOIDS_BENCHMARK_CLI_HPP
#define BOIDS_BENCHMARK_CLI_HPP

#include <string>

#include "cli/CliOptions.hpp"
#include "metrics_interface.hpp"

/**
 * @brief Campaign CLI options for boids_benchmark (Liquids-aligned contract).
 *
 * Kept separate from CliOptions so the demo binary mapping of "all" → compare
 * is never overloaded by campaign semantics.
 */
struct BenchmarkCliOptions {
    std::string model = "all";
    int boids = 100;
    int steps = 10;
    int workers = 4;
    int finePartialBoids = 0;
    unsigned int smtFactor = 2;
    int runs = 1;
    int warmup = 0;
    int seed = 42;
    std::string output = "data/boids_benchmark.csv";
    std::string rawOutput;
};

/**
 * @brief Parses campaign flags for boids_benchmark.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Parsed options; may call exit(0) on --help.
 * @throws std::invalid_argument on unknown or invalid flags.
 */
BenchmarkCliOptions parseBenchmarkCli(int argc, char* argv[]);

/** @brief Prints boids_benchmark usage to stdout. */
void printBenchmarkCliHelp();

/**
 * @brief Maps a campaign model name to execution_model.
 * @param name Canonical or alias name (fine|fgmt|...).
 * @return Matching execution_model.
 * @throws std::invalid_argument if the name is unknown.
 */
execution_model parseBenchmarkModel(const std::string& name);

/**
 * @brief Maps a campaign model name to the demo RunScheme used by createScheme.
 * @param name Canonical or alias model name.
 * @return RunScheme suitable for SchemeFactory (never Compare).
 * @throws std::invalid_argument if the name is unknown or "all".
 */
RunScheme benchmarkModelToRunScheme(const std::string& name);

/**
 * @brief Builds a CliOptions view for SchemeFactory from campaign options.
 * @param options Campaign CLI options.
 * @param model Canonical model name for this run.
 * @return CliOptions with stalls disabled and scheme set for createScheme.
 */
CliOptions toSchemeCliOptions(const BenchmarkCliOptions& options,
                              const std::string& model);

#endif // BOIDS_BENCHMARK_CLI_HPP

#include "cli/BenchmarkCli.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <type_traits>

namespace {

template <typename T>
T parseValue(const char* option, const char* value) {
    try {
        std::size_t consumed = 0;
        const std::string text(value);
        if constexpr (std::is_same_v<T, int>) {
            const int parsed = std::stoi(text, &consumed);
            if (consumed != text.size()) {
                throw std::invalid_argument("not an integer");
            }
            return parsed;
        } else {
            const unsigned long parsed = std::stoul(text, &consumed);
            if (consumed != text.size()) {
                throw std::invalid_argument("not an unsigned integer");
            }
            return static_cast<T>(parsed);
        }
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string("invalid value for ") + option);
    }
}

const char* requireValue(int& index, int argc, char* argv[]) {
    if (index + 1 >= argc) {
        throw std::invalid_argument(std::string("missing value for ") + argv[index]);
    }
    return argv[++index];
}

void requirePositive(const char* option, int value) {
    if (value <= 0) {
        throw std::invalid_argument(std::string(option) + " must be positive");
    }
}

} // namespace

execution_model parseBenchmarkModel(const std::string& name) {
    if (name == "sequential") {
        return execution_model::sequential;
    }
    if (name == "fine" || name == "fgmt") {
        return execution_model::fine_grained;
    }
    if (name == "coarse" || name == "cgmt") {
        return execution_model::coarse_grained;
    }
    if (name == "smt") {
        return execution_model::smt;
    }
    if (name == "cmp") {
        return execution_model::cmp;
    }
    throw std::invalid_argument("unknown model: " + name);
}

RunScheme benchmarkModelToRunScheme(const std::string& name) {
    if (name == "sequential") {
        return RunScheme::Sequential;
    }
    if (name == "fine" || name == "fgmt") {
        return RunScheme::Fine;
    }
    if (name == "coarse" || name == "cgmt") {
        return RunScheme::Coarse;
    }
    if (name == "smt") {
        return RunScheme::Smt;
    }
    if (name == "cmp") {
        return RunScheme::Cmp;
    }
    throw std::invalid_argument("unknown model: " + name);
}

CliOptions toSchemeCliOptions(const BenchmarkCliOptions& options,
                              const std::string& model) {
    CliOptions cli;
    cli.scheme = benchmarkModelToRunScheme(model);
    cli.boidCount = options.boids;
    cli.steps = options.steps;
    cli.workers = options.workers;
    cli.finePartialBoids = options.finePartialBoids;
    cli.smtOversubscribe = options.smtFactor;
    cli.seed = options.seed;
    cli.stallEvery = 0;
    cli.stallProbability = 0.0;
    cli.logCoarseCheckpoints = false;
    cli.gui = false;
    cli.validate = false;
    return cli;
}

BenchmarkCliOptions parseBenchmarkCli(int argc, char* argv[]) {
    BenchmarkCliOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            printBenchmarkCliHelp();
            std::exit(0);
        } else if (argument == "--model" || argument == "--scheme") {
            options.model = requireValue(index, argc, argv);
            if (options.model != "all") {
                (void)parseBenchmarkModel(options.model);
            }
        } else if (argument == "--boids" || argument == "--bodies" ||
                   argument == "-n") {
            options.boids = parseValue<int>(argument.c_str(),
                                            requireValue(index, argc, argv));
            requirePositive(argument.c_str(), options.boids);
        } else if (argument == "--steps") {
            options.steps = parseValue<int>(argument.c_str(),
                                            requireValue(index, argc, argv));
            requirePositive("--steps", options.steps);
        } else if (argument == "--workers" || argument == "--threads") {
            options.workers = parseValue<int>(argument.c_str(),
                                              requireValue(index, argc, argv));
            requirePositive(argument.c_str(), options.workers);
        } else if (argument == "--partial") {
            options.finePartialBoids = parseValue<int>(
                argument.c_str(), requireValue(index, argc, argv));
            if (options.finePartialBoids < 0) {
                throw std::invalid_argument("--partial must be >= 0");
            }
        } else if (argument == "--quantum") {
            // Fine quantum is fixed at one neighbor; accept the flag for
            // campaign parity and reject values other than 1.
            const int quantum = parseValue<int>(
                argument.c_str(), requireValue(index, argc, argv));
            if (quantum != 1) {
                throw std::invalid_argument(
                    "--quantum for boids must be 1 (one neighbor per turn)");
            }
        } else if (argument == "--smt-factor" || argument == "--oversubscribe") {
            options.smtFactor = parseValue<unsigned int>(
                argument.c_str(), requireValue(index, argc, argv));
            if (options.smtFactor == 0) {
                throw std::invalid_argument(
                    std::string(argument) + " must be positive");
            }
        } else if (argument == "--runs") {
            options.runs = parseValue<int>(argument.c_str(),
                                           requireValue(index, argc, argv));
            requirePositive("--runs", options.runs);
        } else if (argument == "--warmup") {
            options.warmup = parseValue<int>(argument.c_str(),
                                             requireValue(index, argc, argv));
            if (options.warmup < 0) {
                throw std::invalid_argument("--warmup must be >= 0");
            }
        } else if (argument == "--seed") {
            options.seed = parseValue<int>(argument.c_str(),
                                           requireValue(index, argc, argv));
        } else if (argument == "--output") {
            options.output = requireValue(index, argc, argv);
        } else if (argument == "--raw-output") {
            options.rawOutput = requireValue(index, argc, argv);
        } else {
            throw std::invalid_argument("unknown argument: " + argument);
        }
    }
    return options;
}

void printBenchmarkCliHelp() {
    std::cout
        << "Usage: boids_benchmark [options]\n"
        << "  --model all|sequential|fine|coarse|smt|cmp\n"
        << "  --boids N, --bodies N, -n N   Flock size (default: 100)\n"
        << "  --steps N                    Steps per run (default: 10)\n"
        << "  --workers N, --threads N     Coarse workers (default: 4)\n"
        << "  --partial N                  Fine context limit (0 = all)\n"
        << "  --quantum 1                  Fine quantum (fixed: 1 neighbor)\n"
        << "  --smt-factor N, --oversubscribe N\n"
        << "                               SMT factor F (default: 2)\n"
        << "  --runs N                     Measured repetitions (default: 1)\n"
        << "  --warmup N                   Cold-start runs discarded (default: 0)\n"
        << "  --seed N                     Deterministic seed (default: 42)\n"
        << "  --output CSV                 Summary CSV path\n"
        << "  --raw-output CSV             Per-run CSV path\n"
        << "  --help                       Show this help\n";
}

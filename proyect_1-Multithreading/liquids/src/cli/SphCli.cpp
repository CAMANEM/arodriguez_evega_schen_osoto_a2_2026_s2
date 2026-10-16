#include "cli/SphCli.hpp"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
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

execution_model parseSphModel(const std::string& name) {
    if (name == "sequential") return execution_model::sequential;
    if (name == "fine" || name == "fgmt") return execution_model::fine_grained;
    if (name == "coarse" || name == "cgmt") return execution_model::coarse_grained;
    if (name == "smt") return execution_model::smt;
    if (name == "cmp") return execution_model::cmp;
    throw std::invalid_argument("unknown model: " + name);
}

SphCliOptions parseSphCli(int argc, char* argv[]) {
    SphCliOptions options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--help" || argument == "-h") {
            printSphCliHelp();
            std::exit(0);
        } else if (argument == "--model") {
            options.model = requireValue(index, argc, argv);
            if (options.model != "all") {
                (void)parseSphModel(options.model);
            }
        } else if (argument == "--particles") {
            options.particles = parseValue<int>(argument.c_str(),
                                                 requireValue(index, argc, argv));
            requirePositive("--particles", options.particles);
        } else if (argument == "--steps") {
            options.steps = parseValue<int>(argument.c_str(),
                                            requireValue(index, argc, argv));
            requirePositive("--steps", options.steps);
        } else if (argument == "--threads") {
            options.threads = parseValue<int>(argument.c_str(),
                                              requireValue(index, argc, argv));
            requirePositive("--threads", options.threads);
        } else if (argument == "--quantum") {
            options.quantum = parseValue<int>(argument.c_str(),
                                              requireValue(index, argc, argv));
            requirePositive("--quantum", options.quantum);
        } else if (argument == "--chunk") {
            options.chunk = parseValue<int>(argument.c_str(),
                                            requireValue(index, argc, argv));
            requirePositive("--chunk", options.chunk);
        } else if (argument == "--smt-factor") {
            options.smtFactor = parseValue<unsigned int>(
                argument.c_str(), requireValue(index, argc, argv));
            if (options.smtFactor == 0) {
                throw std::invalid_argument("--smt-factor must be positive");
            }
        } else if (argument == "--runs") {
            options.runs = parseValue<int>(argument.c_str(),
                                           requireValue(index, argc, argv));
            requirePositive("--runs", options.runs);
        } else if (argument == "--seed") {
            options.seed = parseValue<unsigned int>(
                argument.c_str(), requireValue(index, argc, argv));
        } else if (argument == "--dt") {
            options.deltaTime = std::stod(
                requireValue(index, argc, argv));
            if (options.deltaTime <= 0.0) {
                throw std::invalid_argument("--dt must be positive");
            }
        } else if (argument == "--smoothing-length") {
            options.smoothingLength = std::stod(
                requireValue(index, argc, argv));
            if (options.smoothingLength <= 0.0) {
                throw std::invalid_argument(
                    "--smoothing-length must be positive");
            }
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

void printSphCliHelp() {
    std::cout
        << "Usage: liquids_benchmark [options]\n"
        << "  --model all|sequential|fine|coarse|smt|cmp\n"
        << "  --particles N             Particle count (default: 1000)\n"
        << "  --steps N                 Timestep count (default: 10)\n"
        << "  --threads N               Fine/coarse worker count (default: 4)\n"
        << "  --quantum N               Fine-grained quantum (default: 1)\n"
        << "  --chunk N                 Coarse-grained chunk (default: 16)\n"
        << "  --smt-factor N             SMT oversubscription factor (default: 2)\n"
        << "  --runs N                  Repetitions (default: 1)\n"
        << "  --seed N                  Deterministic input seed (default: 42)\n"
        << "  --dt VALUE                Timestep duration (default: 0.001)\n"
        << "  --smoothing-length VALUE  SPH smoothing length (default: 0.04)\n"
        << "  --output CSV              Output CSV path\n"
        << "  --raw-output CSV          Per-run and per-stage CSV path\n"
        << "  --help                    Show this help\n";
}

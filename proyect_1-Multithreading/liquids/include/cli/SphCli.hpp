#ifndef SPH_CLI_HPP
#define SPH_CLI_HPP

#include <string>

#include "metrics_interface.hpp"

struct SphCliOptions {
    std::string model = "all";
    int particles = 1000;
    int steps = 10;
    int threads = 4;
    int quantum = 1;
    int chunk = 16;
    unsigned int smtFactor = 2;
    int runs = 1;
    int warmup = 0;
    unsigned int seed = 42;
    double deltaTime = 0.001;
    double smoothingLength = 0.04;
    std::string output = "data/liquids_benchmark.csv";
    std::string rawOutput;
};

SphCliOptions parseSphCli(int argc, char* argv[]);
void printSphCliHelp();
execution_model parseSphModel(const std::string& name);

#endif

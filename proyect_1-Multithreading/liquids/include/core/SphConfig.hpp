#ifndef SPH_CONFIG_HPP
#define SPH_CONFIG_HPP

#include "metrics_interface.hpp"

class SphConfig {
public:
    SphConfig(int particleCount = 10000,
              int timeSteps = 100,
              double smoothingLength = 0.04,
              double deltaTime = 0.001,
              double particleMass = 1.0,
              double restDensity = 1000.0,
              double gasStiffness = 2000.0,
              double viscosity = 0.1,
              double gravity = -9.81,
              double domainWidth = 1.0,
              double domainHeight = 1.0,
              execution_model executionModel = execution_model::sequential,
              int threadCount = 1,
              unsigned int seed = 42);

    int getParticleCount() const;
    int getTimeSteps() const;
    double getSmoothingLength() const;
    double getDeltaTime() const;
    double getParticleMass() const;
    double getRestDensity() const;
    double getGasStiffness() const;
    double getViscosity() const;
    double getGravity() const;
    double getDomainWidth() const;
    double getDomainHeight() const;
    execution_model getExecutionModel() const;
    int getThreadCount() const;
    unsigned int getSeed() const;

private:
    int particleCount_;
    int timeSteps_;
    double smoothingLength_;
    double deltaTime_;
    double particleMass_;
    double restDensity_;
    double gasStiffness_;
    double viscosity_;
    double gravity_;
    double domainWidth_;
    double domainHeight_;
    execution_model executionModel_;
    int threadCount_;
    unsigned int seed_;
};

#endif

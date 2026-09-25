#include "core/SphConfig.hpp"

#include <stdexcept>

SphConfig::SphConfig(int particleCount, int timeSteps, double smoothingLength,
                     double deltaTime, double particleMass, double restDensity,
                     double gasStiffness, double viscosity, double gravity,
                     double domainWidth, double domainHeight,
                     execution_model executionModel, int threadCount,
                     unsigned int seed)
    : particleCount_(particleCount),
      timeSteps_(timeSteps),
      smoothingLength_(smoothingLength),
      deltaTime_(deltaTime),
      particleMass_(particleMass),
      restDensity_(restDensity),
      gasStiffness_(gasStiffness),
      viscosity_(viscosity),
      gravity_(gravity),
      domainWidth_(domainWidth),
      domainHeight_(domainHeight),
      executionModel_(executionModel),
      threadCount_(threadCount),
      seed_(seed) {
    if (particleCount_ <= 0 || timeSteps_ <= 0 || threadCount_ <= 0 ||
        smoothingLength_ <= 0.0 || deltaTime_ <= 0.0 || particleMass_ <= 0.0 ||
        restDensity_ <= 0.0 || gasStiffness_ < 0.0 || viscosity_ < 0.0 ||
        domainWidth_ <= 0.0 || domainHeight_ <= 0.0) {
        throw std::invalid_argument("Invalid SPH configuration");
    }
}

int SphConfig::getParticleCount() const { return particleCount_; }
int SphConfig::getTimeSteps() const { return timeSteps_; }
double SphConfig::getSmoothingLength() const { return smoothingLength_; }
double SphConfig::getDeltaTime() const { return deltaTime_; }
double SphConfig::getParticleMass() const { return particleMass_; }
double SphConfig::getRestDensity() const { return restDensity_; }
double SphConfig::getGasStiffness() const { return gasStiffness_; }
double SphConfig::getViscosity() const { return viscosity_; }
double SphConfig::getGravity() const { return gravity_; }
double SphConfig::getDomainWidth() const { return domainWidth_; }
double SphConfig::getDomainHeight() const { return domainHeight_; }
execution_model SphConfig::getExecutionModel() const { return executionModel_; }
int SphConfig::getThreadCount() const { return threadCount_; }
unsigned int SphConfig::getSeed() const { return seed_; }

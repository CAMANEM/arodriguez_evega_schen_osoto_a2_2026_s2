#include "core/FlockingConfig.hpp"

FlockingConfig::FlockingConfig(int boidCount, double worldWidth, double worldHeight,
                                double perceptionRadius, double separationRadius,
                                double maxSpeed, double maxForce,
                                double separationWeight, double alignmentWeight,
                                double cohesionWeight, double deltaTime)
    : boidCount_(boidCount),
      worldWidth_(worldWidth),
      worldHeight_(worldHeight),
      perceptionRadius_(perceptionRadius),
      separationRadius_(separationRadius),
      maxSpeed_(maxSpeed),
      maxForce_(maxForce),
      separationWeight_(separationWeight),
      alignmentWeight_(alignmentWeight),
      cohesionWeight_(cohesionWeight),
      deltaTime_(deltaTime) {
}

int FlockingConfig::getBoidCount() const { return boidCount_; }
double FlockingConfig::getWorldWidth() const { return worldWidth_; }
double FlockingConfig::getWorldHeight() const { return worldHeight_; }
double FlockingConfig::getPerceptionRadius() const { return perceptionRadius_; }
double FlockingConfig::getSeparationRadius() const { return separationRadius_; }
double FlockingConfig::getMaxSpeed() const { return maxSpeed_; }
double FlockingConfig::getMaxForce() const { return maxForce_; }
double FlockingConfig::getSeparationWeight() const { return separationWeight_; }
double FlockingConfig::getAlignmentWeight() const { return alignmentWeight_; }
double FlockingConfig::getCohesionWeight() const { return cohesionWeight_; }
double FlockingConfig::getDeltaTime() const { return deltaTime_; }

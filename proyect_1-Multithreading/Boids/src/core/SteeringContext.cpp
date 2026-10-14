#include "core/SteeringContext.hpp"
#include "core/FlockingRules.hpp"

SteeringContext::SteeringContext(int boidIndex, const Flock& flock, const FlockingConfig& config)
    : boidIndex_(boidIndex),
      flock_(&flock),
      config_(&config),
      candidateCursor_(0),
      neighborCount_(0),
      quantumsExecuted_(0),
      finished_(flock.getBoidCount() <= 1) {
    skipSelfIndex();
}

bool SteeringContext::stepOnce() {
    if (finished_) {
        return false;
    }

    accumulateCurrentCandidate();
    advanceToNextCandidate();
    ++quantumsExecuted_;
    return !finished_;
}

void SteeringContext::accumulateCurrentCandidate() {
    const Boid& self = flock_->getBoid(boidIndex_);
    const Boid& candidate = flock_->getBoid(candidateCursor_);
    const Vector2D offset = self.getPosition() - candidate.getPosition();
    const double distance = offset.magnitude();

    if (distance > 0.0 && distance < config_->getPerceptionRadius()) {
        velocitySum_ += candidate.getVelocity();
        positionSum_ += candidate.getPosition();
        ++neighborCount_;

        if (distance < config_->getSeparationRadius()) {
            separationSum_ += offset * (1.0 / (distance * distance));
        }
    }
}

void SteeringContext::advanceToNextCandidate() {
    ++candidateCursor_;
    skipSelfIndex();
}

void SteeringContext::skipSelfIndex() {
    if (candidateCursor_ == boidIndex_) {
        ++candidateCursor_;
    }
    if (candidateCursor_ >= flock_->getBoidCount()) {
        finished_ = true;
    }
}

bool SteeringContext::isFinished() const {
    return finished_;
}

bool SteeringContext::hasPendingWork() const {
    return !finished_;
}

Vector2D SteeringContext::computeFinalSteering() const {
    const Boid& self = flock_->getBoid(boidIndex_);
    return FlockingRules::combineForces(self.getPosition(), self.getVelocity(), separationSum_,
                                         velocitySum_, positionSum_, neighborCount_, *config_);
}

int SteeringContext::getBoidIndex() const {
    return boidIndex_;
}

int SteeringContext::getCandidateCursor() const {
    return candidateCursor_;
}

int SteeringContext::getQuantumsExecuted() const {
    return quantumsExecuted_;
}

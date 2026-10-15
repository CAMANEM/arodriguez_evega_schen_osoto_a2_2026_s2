#include "core/FlockingRules.hpp"

Vector2D FlockingRules::computeSteeringForBoid(int boidIndex, const Flock& flock,
                                                const FlockingConfig& config) {
    const Boid& self = flock.getBoid(boidIndex);

    Vector2D separationSum;
    Vector2D velocitySum;
    Vector2D positionSum;
    int neighborCount = 0;

    for (int otherIndex = 0; otherIndex < flock.getBoidCount(); ++otherIndex) {
        if (otherIndex == boidIndex) {
            continue;
        }
        const Boid& other = flock.getBoid(otherIndex);
        const Vector2D offset = self.getPosition() - other.getPosition();
        const double distance = offset.magnitude();

        if (distance > 0.0 && distance < config.getPerceptionRadius()) {
            velocitySum += other.getVelocity();
            positionSum += other.getPosition();
            ++neighborCount;

            if (distance < config.getSeparationRadius()) {
                separationSum += offset * (1.0 / (distance * distance));
            }
        }
    }

    return combineForces(self.getPosition(), self.getVelocity(), separationSum, velocitySum,
                          positionSum, neighborCount, config);
}

Vector2D FlockingRules::combineForces(const Vector2D& selfPosition, const Vector2D& selfVelocity,
                                       const Vector2D& separationSum, const Vector2D& velocitySum,
                                       const Vector2D& positionSum, int neighborCount,
                                       const FlockingConfig& config) {
    if (neighborCount == 0) {
        // Sin vecinos: solo puede actuar la separación (que ya es cero si
        // no hubo vecinos dentro del radio de separación).
        return separationSum.limited(config.getMaxForce());
    }

    const Vector2D averageVelocity = velocitySum * (1.0 / neighborCount);
    const Vector2D averagePosition = positionSum * (1.0 / neighborCount);

    const Vector2D alignmentForce = averageVelocity - selfVelocity;
    const Vector2D cohesionForce = averagePosition - selfPosition;

    const Vector2D combined = separationSum * config.getSeparationWeight() +
                               alignmentForce * config.getAlignmentWeight() +
                               cohesionForce * config.getCohesionWeight();

    return combined.limited(config.getMaxForce());
}

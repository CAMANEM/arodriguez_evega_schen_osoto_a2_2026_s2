#ifndef SPH_PARTICLE_HPP
#define SPH_PARTICLE_HPP

#include "object_interface.hpp"

class SphParticle : public object_interface {
public:
    SphParticle(int id, double mass, double x, double y,
                double velocityX = 0.0, double velocityY = 0.0);

    void update(double dt) override;
    void reset() override;

    double getDensity() const;
    double getPressure() const;
    void setDensity(double density);
    void setPressure(double pressure);

private:
    double initialPosX_;
    double initialPosY_;
    double initialSpeedX_;
    double initialSpeedY_;
    double density_;
    double pressure_;
};

#endif

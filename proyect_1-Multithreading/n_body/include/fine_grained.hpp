#pragma once

#include "body.hpp"
#include <vector>

// Virtual execution context: tracks the force computation state for one body.
struct BodyContext {
    int idx;
    int j;
    int n;
    double fx;
    double fy;
    bool finished;

    BodyContext(int index, int total_bodies);

    // Computes the gravitational contribution of body j onto this body and advances the cursor.
    void calculate(const std::vector<Body>& bodies);
};

// Executes one fine-grained step and returns elapsed scheduler time in milliseconds.
double step_fine(std::vector<Body>& bodies, double dt, bool headless);

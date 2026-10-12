#pragma once
#include "body.hpp"
#include <vector>

// Executes one coarse-grained step and returns elapsed time.
double step_coarse(std::vector<Body>& bodies, double dt, bool headless);

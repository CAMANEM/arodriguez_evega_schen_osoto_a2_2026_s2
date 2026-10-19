#pragma once
#include "body.hpp"
#include <vector>

// Executes one CMP step and returns elapsed time in milliseconds.
double step_cmp(std::vector<Body>& bodies, double dt, bool headless, int thread_count);

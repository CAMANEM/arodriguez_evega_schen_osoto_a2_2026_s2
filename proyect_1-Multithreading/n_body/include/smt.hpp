#pragma once
#include "body.hpp"
#include <vector>

// Executes one SMT step and returns elapsed time in milliseconds.
double step_smt(std::vector<Body>& bodies, double dt, bool headless, int thread_count);

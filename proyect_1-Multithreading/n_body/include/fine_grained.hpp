#pragma once
#include "body.hpp"
#include <vector>

struct BodyContext {
    int idx;
    int j;
    int n;
    double fx;
    double fy;
    bool finished;

    BodyContext(int index, int total_bodies);
    void calculate(const std::vector<Body>& bodies);
};

double step_fine(std::vector<Body>& bodies, double dt, bool headless, int thread_count);

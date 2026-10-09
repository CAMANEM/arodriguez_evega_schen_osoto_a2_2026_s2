#include "raylib.h"
#include "../include/body.hpp"
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <iostream>

/* ===== SIMULATION CONSTANTS ================================================================== */

// Gravitational constant (scaled for simulation units)
constexpr double G = 1e5;
// Softening factor: avoids division by zero at short distances
constexpr double EPSILON = 50.0;
constexpr int SCREEN_W = 1600;
constexpr int SCREEN_H = 900;
constexpr int PARTICLE_R = 1;
constexpr double FIXED_DT = 1.0 / 60.0;

// Background color (#1c1c38)
static const Color BG_COLOR = {28, 28, 56, 255};
// Particle color (#fafafa)
static const Color PARTICLE_COLOR = {250, 250, 250, 255};

/* ===== EXECUTION MODE ======================================================================== */

enum class execution_mode { seq };

/* ===== SIMULATION CONFIG ===================================================================== */

struct simulation_config {
    int n = 100;
    double t = 0.0;
    bool gui = true;
    execution_mode mode = execution_mode::seq;
};

/* ===== ARGUMENT PARSER ======================================================================= */

// Parses command-line arguments into a simulation config.
static simulation_config parse_arguments(int argc, char** argv) {
    simulation_config configuration_parameters;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            configuration_parameters.n = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            configuration_parameters.t = atof(argv[++i]);
        } else if (strcmp(argv[i], "-gui") == 0) {
            configuration_parameters.gui = true;
        } else if (strcmp(argv[i], "-headless") == 0) {
            configuration_parameters.gui = false;
        } else if (strcmp(argv[i], "-seq") == 0) {
            configuration_parameters.mode = execution_mode::seq;
        }
    }
    return configuration_parameters;
}

/* ===== SEQUENTIAL STEP ======================================================================= */

// Computes pairwise gravitational forces (Newton's 3rd law) and updates all bodies.
static void step_seq(std::vector<Body>& bodies, double dt, bool headless) {
    int n = static_cast<int>(bodies.size());
    std::vector<double> fx(n, 0.0);
    std::vector<double> fy(n, 0.0);

    for (int i = 0; i < n; i++) {
        if (headless) {
            std::cout << "Main process - Body " << bodies[i].get_id()
                      << " forces and position updating.\n";
        }
        for (int j = i + 1; j < n; j++) {
            double dx = bodies[j].get_pos_x() - bodies[i].get_pos_x();
            double dy = bodies[j].get_pos_y() - bodies[i].get_pos_y();
            double dist2 = dx * dx + dy * dy + EPSILON * EPSILON;
            double dist = std::sqrt(dist2);
            double force = G * bodies[i].get_mass() * bodies[j].get_mass() / dist2;
            double fx_ij = force * dx / dist;
            double fy_ij = force * dy / dist;

            fx[i] += fx_ij;
            fy[i] += fy_ij;
            fx[j] -= fx_ij;
            fy[j] -= fy_ij;
        }
    }

    for (int i = 0; i < n; i++) {
        bodies[i].set_force(fx[i], fy[i]);
        bodies[i].update(dt);
    }
}

/* ===== MAIN ================================================================================== */

int main(int argc, char** argv) {
    simulation_config configuration_parameters = parse_arguments(argc, argv);

    std::vector<Body> bodies;
    bodies.reserve(configuration_parameters.n);
    for (int i = 0; i < configuration_parameters.n; i++) {
        bodies.emplace_back(i, 1.0);
    }

    if (configuration_parameters.gui) {
        InitWindow(SCREEN_W, SCREEN_H, "Problem: N-Body");
        SetWindowFocused();
        SetTargetFPS(60);
    }

    double elapsed = 0.0;
    bool running = true;

    while (running) {
        if (configuration_parameters.gui && WindowShouldClose()) break;

        double dt = configuration_parameters.gui
            ? static_cast<double>(GetFrameTime())
            : FIXED_DT;

        switch (configuration_parameters.mode) {
            case execution_mode::seq:
                step_seq(bodies, dt, !configuration_parameters.gui);
                break;
        }

        if (configuration_parameters.gui) {
            BeginDrawing();
            ClearBackground(BG_COLOR);
            for (const auto& b : bodies) {
                DrawCircle(static_cast<int>(b.get_pos_x()),
                           static_cast<int>(b.get_pos_y()),
                           static_cast<float>(PARTICLE_R),
                           PARTICLE_COLOR);
            }
            EndDrawing();
        }

        elapsed += dt;
        if (configuration_parameters.t > 0.0 && elapsed >= configuration_parameters.t)
            running = false;
    }

    if (configuration_parameters.gui) CloseWindow();
    return 0;
}

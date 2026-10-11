#include "raylib.h"
#include "../include/body.hpp"
#include "../include/fine_grained.hpp"
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <chrono>
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

enum class execution_mode { seq, fine };

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
        } else if (strcmp(argv[i], "-fine") == 0) {
            configuration_parameters.mode = execution_mode::fine;
        }
    }
    return configuration_parameters;
}

/* ===== SEQUENTIAL STEP ======================================================================= */

// Computes pairwise gravitational forces (Newton's 3rd law), updates all bodies, and returns elapsed time in milliseconds.
static double step_seq(std::vector<Body>& bodies, double dt, bool headless) {
    int n = static_cast<int>(bodies.size());
    std::vector<double> fx(n, 0.0);
    std::vector<double> fy(n, 0.0);

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n; i++) {
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

    auto end = std::chrono::high_resolution_clock::now();
    double step_time_ms = std::chrono::duration<double, std::milli>(end - start).count();

    if (headless) {
        for (int i = 0; i < n; i++) {
            std::cout << "Main process - Body " << bodies[i].get_id()
                      << " forces and position updating.\n";
        }
        std::cout << "Step time: " << step_time_ms << " ms\n";
    }

    return step_time_ms;
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
    double last_step_time_ms = 0.0;
    bool running = true;

    while (running) {
        if (configuration_parameters.gui && WindowShouldClose()) {
            break;
        }

        double dt = configuration_parameters.gui
            ? static_cast<double>(GetFrameTime())
            : FIXED_DT;

        switch (configuration_parameters.mode) {
            case execution_mode::seq:
                last_step_time_ms = step_seq(bodies, dt, !configuration_parameters.gui);
                break;
            case execution_mode::fine:
                last_step_time_ms = step_fine(bodies, dt, !configuration_parameters.gui);
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
            DrawText(TextFormat("Step: %.3f ms", last_step_time_ms), 10, 10, 20, RAYWHITE);
            EndDrawing();
        }

        elapsed += dt;
        if (configuration_parameters.t > 0.0 && elapsed >= configuration_parameters.t) {
            running = false;
        }
    }

    if (configuration_parameters.gui) {
        CloseWindow();
    }
    return 0;
}

#include "raylib.h"
#include "../include/body.hpp"
#include "../include/fine_grained.hpp"
#include "../include/coarse_grained.hpp"
#include "../include/smt.hpp"
#include "../include/cmp.hpp"
#include "../include/stall_policy.hpp"
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

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

enum class execution_mode { seq, fine, coarse, smt, cmp };

/* ===== SIMULATION CONFIG ===================================================================== */

struct simulation_config {
    int n = 100;
    int s = 0;
    int e = 1;
    int c = 0;
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
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            configuration_parameters.s = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-e") == 0 && i + 1 < argc) {
            configuration_parameters.e = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            configuration_parameters.c = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-gui") == 0) {
            configuration_parameters.gui = true;
        } else if (strcmp(argv[i], "-headless") == 0) {
            configuration_parameters.gui = false;
        } else if (strcmp(argv[i], "-seq") == 0) {
            configuration_parameters.mode = execution_mode::seq;
        } else if (strcmp(argv[i], "-fine") == 0) {
            configuration_parameters.mode = execution_mode::fine;
        } else if (strcmp(argv[i], "-coarse") == 0) {
            configuration_parameters.mode = execution_mode::coarse;
        } else if (strcmp(argv[i], "-smt") == 0) {
            configuration_parameters.mode = execution_mode::smt;
        } else if (strcmp(argv[i], "-cmp") == 0) {
            configuration_parameters.mode = execution_mode::cmp;
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

    StallPolicy::instance().reset_stall_count();

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (StallPolicy::instance().check()) {
                std::this_thread::sleep_for(
                    std::chrono::nanoseconds(StallPolicy::instance().get_stall_penalization()));
            }
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
        std::cout << "Iteracion: " << step_time_ms << " ms\n";
        std::cout << "Stalls: " << StallPolicy::instance().get_stall_count() << "\n";
    }

    return step_time_ms;
}

/* ===== HELPERS =============================================================================== */

static std::string current_timestamp() {
    auto t = std::time(nullptr);
    auto* tm_ptr = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm_ptr, "%Y-%m-%d_%H-%M-%S");
    return oss.str();
}

static std::string mode_name(execution_mode mode) {
    switch (mode) {
        case execution_mode::seq:    return "Sequential";
        case execution_mode::fine:   return "Fine-grained";
        case execution_mode::coarse: return "Coarse-grained";
        case execution_mode::smt:    return "SMT";
        case execution_mode::cmp:    return "CMP";
    }
    return "Unknown";
}

/* ===== MAIN ================================================================================== */

int main(int argc, char** argv) {
    simulation_config configuration_parameters = parse_arguments(argc, argv);

    int L = static_cast<int>(std::thread::hardware_concurrency());
    if (L <= 0) {
        L = 1;
    }
    int core_count = (configuration_parameters.c > 0)
        ? std::min(configuration_parameters.c, L)
        : L;

    // SMT doubles the core count to get logical thread count; others use core_count directly.
    int thread_count = (configuration_parameters.mode == execution_mode::smt)
        ? core_count * 2
        : core_count;

    // Sequential is always single-core, single-thread regardless of -c.
    int csv_cores   = (configuration_parameters.mode == execution_mode::seq) ? 1 : core_count;
    int csv_threads = (configuration_parameters.mode == execution_mode::seq) ? 1 : thread_count;

    std::string csv_path = "data/execution_results_" + current_timestamp() + ".csv";
    std::ofstream csv(csv_path);
    csv << "iteracion,particulas,modo,steps,metodologia,nucleos,hilos,tiempo_total_ms\n";

    if (configuration_parameters.gui) {
        InitWindow(SCREEN_W, SCREEN_H, "Problem: N-Body");
        SetWindowFocused();
        SetTargetFPS(60);
    }

    double last_step_time_ms = 0.0;
    bool window_closed = false;

    for (int exec = 1; exec <= configuration_parameters.e && !window_closed; exec++) {
        std::vector<Body> bodies;
        bodies.reserve(configuration_parameters.n);
        for (int i = 0; i < configuration_parameters.n; i++) {
            bodies.emplace_back(i, 1.0);
        }

        int step_count = 0;
        bool running = true;

        auto exec_start = std::chrono::high_resolution_clock::now();

        while (running && !window_closed) {
            if (configuration_parameters.gui && WindowShouldClose()) {
                window_closed = true;
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
                    last_step_time_ms = step_fine(bodies, dt, !configuration_parameters.gui, thread_count);
                    break;
                case execution_mode::coarse:
                    last_step_time_ms = step_coarse(bodies, dt, !configuration_parameters.gui, thread_count);
                    break;
                case execution_mode::smt:
                    last_step_time_ms = step_smt(bodies, dt, !configuration_parameters.gui, thread_count);
                    break;
                case execution_mode::cmp:
                    last_step_time_ms = step_cmp(bodies, dt, !configuration_parameters.gui, thread_count);
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
                DrawText(TextFormat("Iteracion: %.3f ms", last_step_time_ms), 10, 10, 20, RAYWHITE);
                DrawText(TextFormat("Stalls: %d", StallPolicy::instance().get_stall_count()), SCREEN_W - 150, 10, 20, RAYWHITE);
                EndDrawing();
            }

            step_count++;
            if (configuration_parameters.s > 0 && step_count >= configuration_parameters.s) {
                running = false;
            }
        }

        auto exec_end = std::chrono::high_resolution_clock::now();
        double total_time_ms = std::chrono::duration<double, std::milli>(exec_end - exec_start).count();

        csv << exec << ","
            << configuration_parameters.n << ","
            << (configuration_parameters.gui ? "GUI" : "Headless") << ","
            << configuration_parameters.s << ","
            << mode_name(configuration_parameters.mode) << ","
            << csv_cores << ","
            << csv_threads << ","
            << std::fixed << std::setprecision(3) << total_time_ms << "\n";
        csv.flush();
    }

    if (configuration_parameters.gui) {
        CloseWindow();
    }

    return 0;
}

#include "raylib.h"
#include "../include/body.hpp"
#include <vector>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <thread>
#include <mutex>
#include <iostream>

/* ===== SIMULATION CONSTANTS ================================================================== */
constexpr double G              = 1e5;    // Scaled gravitational constant (ajustable)
constexpr double EPSILON        = 50.0;   // Softening factor in pixels (avoids div by zero)
constexpr int    SCREEN_W       = 1600;
constexpr int    SCREEN_H       = 900;
constexpr int    PARTICLE_R     = 1;
constexpr double FIXED_DT       = 1.0 / 60.0;

static const Color BG_COLOR       = {28,  28,  56,  255}; // #1c1c38
static const Color PARTICLE_COLOR = {250, 250, 250, 255}; // #fafafa

/* ===== EXECUTION MODE ======================================================================== */
enum class exec_mode { seq, coarse_grained };

/* ===== SIMULATION CONFIG ===================================================================== */
struct sim_config {
    int       n    = 100;
    double    t    = 0.0;   // 0 = run indefinitely
    bool      gui  = true;
    exec_mode mode = exec_mode::seq;
};

/* ===== ARGUMENT PARSER ======================================================================= */
static sim_config parse_args(int argc, char** argv) {
    sim_config cfg;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            cfg.n = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            cfg.t = atof(argv[++i]);
        } else if (strcmp(argv[i], "-gui") == 0) {
            cfg.gui = true;
        } else if (strcmp(argv[i], "-headless") == 0) {
            cfg.gui = false;
        } else if (strcmp(argv[i], "-seq") == 0) {
            cfg.mode = exec_mode::seq;
        } else if (strcmp(argv[i], "-coarse") == 0) {
            cfg.mode = exec_mode::coarse_grained;
        }
        // -fine, -smt, -cmp: recognized but not yet implemented
    }
    return cfg;
}

/* ===== POSITION SNAPSHOT (for coarse-grained) ================================================ */
struct BodySnapshot {
    double x, y, mass;
};

/* ===== SEQUENTIAL STEP ======================================================================= */
// Uses Newton's 3rd law (pair-wise). Prints per body when headless.
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
            double dx    = bodies[j].get_pos_x() - bodies[i].get_pos_x();
            double dy    = bodies[j].get_pos_y() - bodies[i].get_pos_y();
            double dist2 = dx * dx + dy * dy + EPSILON * EPSILON;
            double dist  = std::sqrt(dist2);
            double force = G * bodies[i].get_mass() * bodies[j].get_mass() / dist2;
            double fx_ij = force * dx / dist;
            double fy_ij = force * dy / dist;

            fx[i] += fx_ij;  fy[i] += fy_ij;
            fx[j] -= fx_ij;  fy[j] -= fy_ij;
        }
    }

    for (int i = 0; i < n; i++) {
        bodies[i].set_force(fx[i], fy[i]);
        bodies[i].update(dt);
    }
}

/* ===== COARSE-GRAINED WORKER ================================================================= */
// Each thread handles a chunk of bodies: computes forces from the shared snapshot
// and updates position. Reads are from snapshot (no races); writes are to disjoint
// body indices (each body owned by exactly one thread).
static std::mutex print_mutex;

static void coarse_worker(std::vector<Body>& bodies,
                          const std::vector<BodySnapshot>& snap,
                          int start, int end,
                          double dt, int thread_id, bool headless) {
    int n = static_cast<int>(bodies.size());

    for (int i = start; i < end; i++) {
        if (headless) {
            std::lock_guard<std::mutex> lock(print_mutex);
            std::cout << "Thread " << thread_id << " - Body " << bodies[i].get_id()
                      << " forces and position updating.\n";
        }

        double fx = 0.0, fy = 0.0;
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            double dx    = snap[j].x - snap[i].x;
            double dy    = snap[j].y - snap[i].y;
            double dist2 = dx * dx + dy * dy + EPSILON * EPSILON;
            double dist  = std::sqrt(dist2);
            double force = G * snap[i].mass * snap[j].mass / dist2;
            fx += force * dx / dist;
            fy += force * dy / dist;
        }

        bodies[i].set_force(fx, fy);
        bodies[i].update(dt);
    }
}

/* ===== COARSE-GRAINED STEP =================================================================== */
static void step_coarse(std::vector<Body>& bodies, double dt, bool headless) {
    int n = static_cast<int>(bodies.size());

    // Snapshot current positions and masses (read-only baseline for all threads)
    std::vector<BodySnapshot> snap(n);
    for (int i = 0; i < n; i++) {
        snap[i] = { bodies[i].get_pos_x(), bodies[i].get_pos_y(), bodies[i].get_mass() };
    }

    int num_threads = static_cast<int>(std::thread::hardware_concurrency());
    if (num_threads < 1) num_threads = 2;

    std::vector<std::thread> threads;
    threads.reserve(num_threads);

    int chunk = (n + num_threads - 1) / num_threads;
    for (int t = 0; t < num_threads; t++) {
        int start = t * chunk;
        int end   = std::min(start + chunk, n);
        if (start >= n) break;
        threads.emplace_back(coarse_worker,
                             std::ref(bodies), std::cref(snap),
                             start, end, dt, t + 1, headless);
    }

    for (auto& th : threads) th.join();
}

/* ===== MAIN ================================================================================== */
int main(int argc, char** argv) {
    sim_config cfg = parse_args(argc, argv);

    std::vector<Body> bodies;
    bodies.reserve(cfg.n);
    for (int i = 0; i < cfg.n; i++) {
        bodies.emplace_back(i, 1.0);
    }

    if (cfg.gui) {
        InitWindow(SCREEN_W, SCREEN_H, "Problem: N-Body");
        SetWindowFocused();
        SetTargetFPS(60);
    }

    double elapsed = 0.0;
    bool   running = true;

    while (running) {
        if (cfg.gui && WindowShouldClose()) break;

        double dt = cfg.gui ? static_cast<double>(GetFrameTime()) : FIXED_DT;

        switch (cfg.mode) {
            case exec_mode::seq:
                step_seq(bodies, dt, !cfg.gui);
                break;
            case exec_mode::coarse_grained:
                step_coarse(bodies, dt, !cfg.gui);
                break;
        }

        if (cfg.gui) {
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
        if (cfg.t > 0.0 && elapsed >= cfg.t) running = false;
    }

    if (cfg.gui) CloseWindow();
    return 0;
}

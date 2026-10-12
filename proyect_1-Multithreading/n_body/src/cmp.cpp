#include "../include/cmp.hpp"
#include "../include/stall_policy.hpp"
#include <thread>
#include <cmath>
#include <chrono>
#include <iostream>
#include <vector>

constexpr double G = 1e5;
constexpr double EPSILON = 50.0;

// L threads all run in parallel; each computes forces for its assigned bodies.
double step_cmp(std::vector<Body>& bodies, double dt, bool headless) {
    int n = static_cast<int>(bodies.size());

    if (n == 0) {
        return 0.0;
    }

    StallPolicy::instance().reset_stall_count();

    int thread_count = static_cast<int>(std::thread::hardware_concurrency());
    if (thread_count <= 0) {
        thread_count = 1;
    }
    if (thread_count > n) {
        thread_count = n;
    }

    int assigned_bodies_size = n / thread_count;

    std::vector<double> fx(n, 0.0);
    std::vector<double> fy(n, 0.0);

    auto worker_cmp = [&](int id) {
        int start = id * assigned_bodies_size;
        int count = (id == thread_count - 1) ? (n - start) : assigned_bodies_size;

        for (int i = start; i < start + count; i++) {
            for (int j = 0; j < n; j++) {
                if (j == i) {
                    continue;
                }
                if (StallPolicy::instance().check()) {
                    std::this_thread::sleep_for(
                        std::chrono::nanoseconds(StallPolicy::instance().get_stall_penalization()));
                }
                double dx = bodies[j].get_pos_x() - bodies[i].get_pos_x();
                double dy = bodies[j].get_pos_y() - bodies[i].get_pos_y();
                double dist2 = dx * dx + dy * dy + EPSILON * EPSILON;
                double dist = std::sqrt(dist2);
                double force = G * bodies[i].get_mass() * bodies[j].get_mass() / dist2;
                fx[i] += force * dx / dist;
                fy[i] += force * dy / dist;
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(thread_count);

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < thread_count; i++) {
        workers.emplace_back(worker_cmp, i);
    }

    for (auto& w : workers) {
        w.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double step_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    for (int i = 0; i < n; i++) {
        bodies[i].set_force(fx[i], fy[i]);
        bodies[i].update(dt);
    }

    if (headless) {
        for (int i = 0; i < n; i++) {
            std::cout << "CMP - Body " << bodies[i].get_id()
                      << " forces and position updating.\n";
        }
        std::cout << "Iteracion: " << step_time_ms << " ms\n";
        std::cout << "Stalls: " << StallPolicy::instance().get_stall_count() << "\n";
    }

    return step_time_ms;
}

#include "../include/fine_grained.hpp"
#include "../include/stall_policy.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <cmath>
#include <iostream>

constexpr double G = 1e5;
constexpr double EPSILON = 50.0;

// Initializes cursor at j=0, skipping own index if needed.
BodyContext::BodyContext(int index, int total_bodies)
    : idx(index), j(0), n(total_bodies), fx(0.0), fy(0.0), finished(total_bodies <= 1) {
    if (j == idx) {
        j++;
    }
    if (j >= n) {
        finished = true;
    }
}

// Accumulates force from body j into body idx, then advances j to the next valid candidate.
void BodyContext::calculate(const std::vector<Body>& bodies) {
    if (finished) {
        return;
    }

    double dx = bodies[j].get_pos_x() - bodies[idx].get_pos_x();
    double dy = bodies[j].get_pos_y() - bodies[idx].get_pos_y();
    double dist2 = dx * dx + dy * dy + EPSILON * EPSILON;
    double dist = std::sqrt(dist2);
    double force = G * bodies[idx].get_mass() * bodies[j].get_mass() / dist2;
    fx += force * dx / dist;
    fy += force * dy / dist;

    j++;
    // Skips index after advancing
    if (j == idx) {
        j++;
    }
    if (j >= n) {
        finished = true;
    }
}

// L threads pass a token in a ring.
double step_fine(std::vector<Body>& bodies, double dt, bool headless, int thread_count) {
    int n = static_cast<int>(bodies.size());

    if (n == 0) {
        return 0.0;
    }

    StallPolicy::instance().reset_stall_count();

    std::vector<BodyContext> contexts;
    contexts.reserve(n);
    for (int i = 0; i < n; i++) {
        contexts.emplace_back(i, n);
    }

    if (thread_count <= 0) {
        thread_count = 1;
    }
    if (thread_count > n) {
        thread_count = n;
    }

    int assigned_bodies_size = n / thread_count;

    std::vector<std::mutex> mtxs(thread_count);
    std::vector<std::condition_variable> cvs(thread_count);
    // int avoids vector<bool> bit-packing data races across threads
    std::vector<int> ready(thread_count, 0);

    std::atomic<int> finished_count{0};
    std::atomic<bool> all_finished{false};

    // Count contexts already done at construction (e.g., n == 1)
    for (const auto& ctx : contexts) {
        if (ctx.finished) {
            finished_count.fetch_add(1);
        }
    }
    if (finished_count.load() == n) {
        all_finished.store(true);
    }

    auto worker_fine = [&](int id) {
        int start = id * assigned_bodies_size;
        int count = (id == thread_count - 1) ? (n - start) : assigned_bodies_size;
        int cursor = 0;
        int last_checked_cursor = -1;

        while (true) {
            {
                std::unique_lock<std::mutex> lock(mtxs[id]);
                cvs[id].wait(lock, [&] { return ready[id] || all_finished.load(); });
                ready[id] = 0;
            }

            if (all_finished.load()) {
                break;
            }

            // Advance past already finished contexts
            while (cursor < count && contexts[start + cursor].finished) {
                cursor++;
            }

            if (cursor < count) {
                // Stall check once per body: delays the quantum but does not interrupt it.
                if (cursor != last_checked_cursor) {
                    if (StallPolicy::instance().check()) {
                        std::this_thread::sleep_for(
                            std::chrono::nanoseconds(StallPolicy::instance().get_stall_penalization()));
                    }
                    last_checked_cursor = cursor;
                }
                contexts[start + cursor].calculate(bodies);
                if (contexts[start + cursor].finished) {
                    if (finished_count.fetch_add(1) + 1 == n) {
                        all_finished.store(true);
                        // Wake all blocked threads so they can check all_finished and exit
                        for (int k = 0; k < thread_count; k++) {
                            cvs[k].notify_all();
                        }
                    }
                }
            }

            if (!all_finished.load()) {
                int next = (id + 1) % thread_count;
                {
                    std::lock_guard<std::mutex> lock(mtxs[next]);
                    ready[next] = 1;
                }
                cvs[next].notify_one();
            }
        }
    };

    std::vector<std::thread> workers;
    workers.reserve(thread_count);
    for (int i = 0; i < thread_count; i++) {
        workers.emplace_back(worker_fine, i);
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    if (!all_finished.load()) {
        std::lock_guard<std::mutex> lock(mtxs[0]);
        ready[0] = 1;
        cvs[0].notify_one();
    } else {
        for (int k = 0; k < thread_count; k++) {
            cvs[k].notify_all();
        }
    }

    for (auto& w : workers) {
        w.join();
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    double step_time_ms = std::chrono::duration<double, std::milli>(end_time - start_time).count();

    for (int i = 0; i < n; i++) {
        bodies[i].set_force(contexts[i].fx, contexts[i].fy);
        bodies[i].update(dt);
    }

    if (headless) {
        for (int i = 0; i < n; i++) {
            std::cout << "Fine-grained - Body " << bodies[i].get_id()
                      << " forces and position updating.\n";
        }
        std::cout << "Iteracion: " << step_time_ms << " ms\n";
        std::cout << "Stalls: " << StallPolicy::instance().get_stall_count() << "\n";
    }

    return step_time_ms;
}

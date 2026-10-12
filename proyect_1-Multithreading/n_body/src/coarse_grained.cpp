#include "../include/coarse_grained.hpp"
#include "../include/fine_grained.hpp"
#include "../include/stall_policy.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <iostream>

// L threads pass a token.
double step_coarse(std::vector<Body>& bodies, double dt, bool headless) {
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

    int thread_count = static_cast<int>(std::thread::hardware_concurrency());
    if (thread_count <= 0) {
        thread_count = 1;
    }
    if (thread_count > n) {
        thread_count = n;
    }

    int assigned_bodies_size = n / thread_count;

    std::vector<std::mutex> mtxs(thread_count);
    std::vector<std::condition_variable> cvs(thread_count);
    std::vector<int> ready(thread_count, 0);

    std::atomic<int> finished_count{0};
    std::atomic<bool> all_finished{false};

    for (const auto& ctx : contexts) {
        if (ctx.finished) {
            finished_count.fetch_add(1);
        }
    }
    if (finished_count.load() == n) {
        all_finished.store(true);
    }

    auto worker_coarse = [&](int id) {
        int start = id * assigned_bodies_size;
        int count = (id == thread_count - 1) ? (n - start) : assigned_bodies_size;
        int cursor = 0;

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

            // Run multiple quant until a stall occurs or this group's work is done.
            while (cursor < count && !all_finished.load()) {
                if (StallPolicy::instance().check()) {
                    // Stall is the context-switch trigger and changes inmediately.
                    break;
                }

                contexts[start + cursor].calculate(bodies);

                if (contexts[start + cursor].finished) {
                    if (finished_count.fetch_add(1) + 1 == n) {
                        all_finished.store(true);
                        for (int k = 0; k < thread_count; k++) {
                            cvs[k].notify_all();
                        }
                    }
                    cursor++;
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
        workers.emplace_back(worker_coarse, i);
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
            std::cout << "Coarse-grained - Body " << bodies[i].get_id()
                      << " forces and position updating.\n";
        }
        std::cout << "Iteracion: " << step_time_ms << " ms\n";
        std::cout << "Stalls: " << StallPolicy::instance().get_stall_count() << "\n";
    }

    return step_time_ms;
}

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
#include <string>
#include <algorithm>
#include <numeric>

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
    int warmup = 0;
    unsigned int smt_factor = 2;
    unsigned int seed = 42;
    bool gui = true;
    execution_mode mode = execution_mode::seq;
    std::string o = "";
    std::string campaign_output = "";
    std::string campaign_raw_output = "";
};

/**
 * @brief Maps a campaign model token to execution_mode.
 * @param name Model name (sequential|fine|fgmt|...).
 * @param out Mode written on success.
 * @return true if the name was recognized.
 */
static bool parse_mode_name(const char* name, execution_mode& out) {
    if (strcmp(name, "sequential") == 0 || strcmp(name, "seq") == 0) {
        out = execution_mode::seq;
        return true;
    }
    if (strcmp(name, "fine") == 0 || strcmp(name, "fgmt") == 0) {
        out = execution_mode::fine;
        return true;
    }
    if (strcmp(name, "coarse") == 0 || strcmp(name, "cgmt") == 0) {
        out = execution_mode::coarse;
        return true;
    }
    if (strcmp(name, "smt") == 0) {
        out = execution_mode::smt;
        return true;
    }
    if (strcmp(name, "cmp") == 0) {
        out = execution_mode::cmp;
        return true;
    }
    return false;
}

/**
 * @brief Prints long and short CLI flags for n_body.
 */
static void print_help() {
    std::cout
        << "Usage: n_body [options]\n"
        << "Short flags (legacy):\n"
        << "  -n N           Body count (default: 100)\n"
        << "  -s N           Steps per execution (0 = forever in GUI)\n"
        << "  -e N           Measured executions / runs (default: 1)\n"
        << "  -c N           Core/worker override (default: hardware_concurrency)\n"
        << "  -o PATH        Legacy CSV path\n"
        << "  -gui|-headless Display mode (default: -gui)\n"
        << "  -seq|-fine|-coarse|-smt|-cmp\n"
        << "Long flags (campaign alignment):\n"
        << "  --model|--scheme NAME   sequential|fine|coarse|smt|cmp\n"
        << "  --workers|--threads N   Alias of -c\n"
        << "  --steps N               Alias of -s\n"
        << "  --runs N                Alias of -e\n"
        << "  --warmup N              Discarded cold-start executions (default: 0)\n"
        << "  --smt-factor N           SMT T = cores * F (default: 2)\n"
        << "  --seed N                Recorded in campaign CSV (default: 42)\n"
        << "  --output PATH           Campaign summary CSV\n"
        << "  --raw-output PATH       Campaign raw CSV with phase column\n"
        << "  --help                  Show this help\n";
}

/* ===== ARGUMENT PARSER ======================================================================= */

/**
 * @brief Parses short and long campaign flags into simulation_config.
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return Parsed configuration (defaults preserved when flags omitted).
 */
static simulation_config parse_arguments(int argc, char** argv) {
    simulation_config configuration_parameters;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_help();
            std::exit(0);
        } else if ((strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--boids") == 0 ||
                    strcmp(argv[i], "--bodies") == 0 || strcmp(argv[i], "--particles") == 0) &&
                   i + 1 < argc) {
            configuration_parameters.n = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--steps") == 0) &&
                   i + 1 < argc) {
            configuration_parameters.s = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-e") == 0 || strcmp(argv[i], "--runs") == 0) &&
                   i + 1 < argc) {
            configuration_parameters.e = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--workers") == 0 ||
                    strcmp(argv[i], "--threads") == 0) &&
                   i + 1 < argc) {
            configuration_parameters.c = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            configuration_parameters.warmup = atoi(argv[++i]);
        } else if ((strcmp(argv[i], "--smt-factor") == 0 ||
                    strcmp(argv[i], "--oversubscribe") == 0) &&
                   i + 1 < argc) {
            const int factor = atoi(argv[++i]);
            configuration_parameters.smt_factor =
                factor > 0 ? static_cast<unsigned int>(factor) : 1u;
        } else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            configuration_parameters.seed = static_cast<unsigned int>(atoi(argv[++i]));
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            configuration_parameters.o = argv[++i];
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            configuration_parameters.campaign_output = argv[++i];
        } else if (strcmp(argv[i], "--raw-output") == 0 && i + 1 < argc) {
            configuration_parameters.campaign_raw_output = argv[++i];
        } else if ((strcmp(argv[i], "--model") == 0 || strcmp(argv[i], "--scheme") == 0) &&
                   i + 1 < argc) {
            execution_mode mode = execution_mode::seq;
            if (!parse_mode_name(argv[++i], mode)) {
                std::cerr << "Unknown model: " << argv[i] << "\n";
                std::exit(1);
            }
            configuration_parameters.mode = mode;
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
        if (StallPolicy::instance().check()) {
            std::this_thread::sleep_for(
                std::chrono::nanoseconds(StallPolicy::instance().get_stall_penalization()));
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

/**
 * @brief Canonical campaign model label for CSV rows.
 * @param mode Execution mode.
 * @return sequential|fine|coarse|smt|cmp.
 */
static std::string campaign_model_name(execution_mode mode) {
    switch (mode) {
        case execution_mode::seq:    return "sequential";
        case execution_mode::fine:   return "fine";
        case execution_mode::coarse: return "coarse";
        case execution_mode::smt:    return "smt";
        case execution_mode::cmp:    return "cmp";
    }
    return "unknown";
}

/**
 * @brief Runs one full execution (s steps).
 * @param configuration_parameters Simulation CLI config.
 * @param thread_count Worker count passed to parallel schemes.
 * @param window_closed Set when the GUI window closes.
 * @param last_step_time_ms Last step wall time in milliseconds.
 * @param total_stalls Accumulated didactic stalls across steps.
 * @return Wall time of the whole execution, in seconds.
 */
static double run_one_execution(simulation_config& configuration_parameters,
                                int thread_count,
                                bool& window_closed,
                                double& last_step_time_ms,
                                long long& total_stalls) {
    std::vector<Body> bodies;
    bodies.reserve(configuration_parameters.n);
    for (int i = 0; i < configuration_parameters.n; i++) {
        bodies.emplace_back(i, 1.0);
    }

    int step_count = 0;
    total_stalls = 0;
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

        total_stalls += StallPolicy::instance().get_stall_count();

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
            DrawText(TextFormat("Stalls: %d", StallPolicy::instance().get_stall_count()),
                     SCREEN_W - 150, 10, 20, RAYWHITE);
            EndDrawing();
        }

        step_count++;
        if (configuration_parameters.s > 0 && step_count >= configuration_parameters.s) {
            running = false;
        }
    }

    auto exec_end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(exec_end - exec_start).count();
}

/**
 * @brief Writes campaign summary CSV using measured-run wall times only.
 */
static void write_campaign_summary(const simulation_config& cfg,
                                   int effective_workers,
                                   const std::vector<double>& measured_seconds) {
    if (cfg.campaign_output.empty()) {
        return;
    }
    std::ofstream file(cfg.campaign_output);
    if (!file) {
        std::cerr << "Could not open campaign output: " << cfg.campaign_output << "\n";
        return;
    }

    double mean = 0.0;
    double stddev = 0.0;
    double ci_lo = 0.0;
    double ci_hi = 0.0;
    const int runs = static_cast<int>(measured_seconds.size());
    if (runs > 0) {
        mean = std::accumulate(measured_seconds.begin(), measured_seconds.end(), 0.0) /
               static_cast<double>(runs);
        if (runs >= 2) {
            double sq = 0.0;
            for (double sample : measured_seconds) {
                const double d = sample - mean;
                sq += d * d;
            }
            stddev = std::sqrt(sq / static_cast<double>(runs - 1));
            const double half = 1.96 * stddev / std::sqrt(static_cast<double>(runs));
            ci_lo = mean - half;
            ci_hi = mean + half;
        } else {
            ci_lo = mean;
            ci_hi = mean;
        }
    }

    file << "model,workload_n,steps,effective_workers,quantum,smt_factor,seed,runs,warmup,"
            "mean_s,stddev_s,ci95_lo,ci95_hi,speedup,efficiency\n";
    file << campaign_model_name(cfg.mode) << ',' << cfg.n << ',' << cfg.s << ','
         << effective_workers << ",1," << cfg.smt_factor << ',' << cfg.seed << ','
         << runs << ',' << cfg.warmup << ','
         << std::setprecision(12) << mean << ',' << stddev << ',' << ci_lo << ',' << ci_hi
         << ",0,0\n";
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

    // SMT uses cores * smt_factor; others use core_count directly.
    int thread_count = (configuration_parameters.mode == execution_mode::smt)
        ? static_cast<int>(core_count * configuration_parameters.smt_factor)
        : core_count;

    // Sequential is always single-core, single-thread regardless of -c.
    int csv_cores   = (configuration_parameters.mode == execution_mode::seq) ? 1 : core_count;
    int csv_threads = (configuration_parameters.mode == execution_mode::seq) ? 1 : thread_count;
    const int effective_workers = csv_threads;

    // Append to -o path when provided; otherwise create a timestamped file.
    std::string csv_path;
    bool csv_append = false;
    if (!configuration_parameters.o.empty()) {
        csv_path = configuration_parameters.o;
        std::ifstream check(csv_path);
        csv_append = check.good();
    } else {
        csv_path = "data/execution_results_" + current_timestamp() + ".csv";
    }

    std::ofstream csv(csv_path, csv_append ? std::ios::app : std::ios::out);
    if (!csv_append) {
        csv << "iteracion,particulas,modo,steps,metodologia,nucleos,hilos,stalls_totales,tiempo_total_ms\n";
    }

    std::ofstream raw_csv;
    if (!configuration_parameters.campaign_raw_output.empty()) {
        raw_csv.open(configuration_parameters.campaign_raw_output);
        raw_csv << "model,run_index,phase,workload_n,steps,effective_workers,seconds\n";
    }

    if (configuration_parameters.gui) {
        InitWindow(SCREEN_W, SCREEN_H, "Problem: N-Body");
        SetWindowFocused();
        SetTargetFPS(60);
    }

    double last_step_time_ms = 0.0;
    bool window_closed = false;
    std::vector<double> measured_seconds;
    const int total_executions =
        configuration_parameters.warmup + configuration_parameters.e;

    for (int exec = 1; exec <= total_executions && !window_closed; exec++) {
        const bool is_warmup = exec <= configuration_parameters.warmup;
        const std::string phase = is_warmup ? "warmup" : "measured";
        const int phase_index = is_warmup
            ? exec - 1
            : exec - configuration_parameters.warmup - 1;

        long long total_stalls = 0;
        const double total_time_s = run_one_execution(
            configuration_parameters, thread_count, window_closed, last_step_time_ms,
            total_stalls);

        // Legacy CSV keeps the original schema for every execution (warmup + measured).
        csv << exec << ","
            << configuration_parameters.n << ","
            << (configuration_parameters.gui ? "GUI" : "Headless") << ","
            << configuration_parameters.s << ","
            << mode_name(configuration_parameters.mode) << ","
            << csv_cores << ","
            << csv_threads << ","
            << total_stalls << ","
            << std::fixed << std::setprecision(3) << (total_time_s * 1000.0) << "\n";
        csv.flush();

        if (raw_csv) {
            raw_csv << campaign_model_name(configuration_parameters.mode) << ','
                    << phase_index << ',' << phase << ','
                    << configuration_parameters.n << ','
                    << configuration_parameters.s << ','
                    << effective_workers << ','
                    << std::setprecision(12) << total_time_s << '\n';
        }

        if (!is_warmup) {
            measured_seconds.push_back(total_time_s);
        }
    }

    write_campaign_summary(configuration_parameters, effective_workers, measured_seconds);

    if (configuration_parameters.gui) {
        CloseWindow();
    }

    return 0;
}

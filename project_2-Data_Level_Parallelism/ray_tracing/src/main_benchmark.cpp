/**
 * @file main_benchmark.cpp
 * @brief Driver del benchmark DLP — proyecto 2.
 *
 * Uso:
 *   rt_benchmark [--width W] [--height H] [--runs N] [--warmup N]
 *                [--output results/benchmark.csv] [--image results/frame.ppm]
 *
 * Variable paralelizable: N rayos = width × height.
 * Para un sweep de escalabilidad, varía --width y --height manteniendo el aspect
 * ratio o usa --width=N --height=1 para forzar exactamente N rayos en línea.
 */
#include "config/raytracing_config.hpp"
#include "core/strategies/IRenderer.h"
#include "core/strategies/SimdRenderer.h"
#include "core/strategies/SequentialRenderer.h"
#include "utils/image_io.hpp"
#include "utils/Metrics.h"
#include "utils/Timer.hpp"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static int parse_positive_int(const std::string& val, const char* opt) {
    std::size_t pos = 0;
    const int r = std::stoi(val, &pos);
    if (pos != val.size() || r < 1)
        throw std::invalid_argument(std::string(opt) + " must be a positive integer");
    return r;
}

/** @brief Escribe un CSV con una fila por ejecución más la fila de resumen. */
static void write_csv(const std::string& path,
                      const std::string& model,
                      int rays, int warmup,
                      const std::vector<double>& times,
                      const dlp::RunStats& stats) {
    const std::filesystem::path out(path);
    if (out.has_parent_path())
        std::filesystem::create_directories(out.parent_path());

    std::ofstream f(path);
    if (!f) throw std::runtime_error("could not open CSV: " + path);
    f << std::setprecision(12);

    f << "model,rays,warmup,run,wall_time_s\n";
    for (int i = 0; i < static_cast<int>(times.size()); ++i)
        f << model << ',' << rays << ',' << warmup << ',' << (i + 1) << ',' << times[i] << '\n';

    f << "\n# summary\n";
    f << "model,rays,runs,warmup,mean_s,stddev_s,min_s,max_s,speedup,efficiency\n";
    // Para el baseline, speedup y efficiency son siempre 1.0
    f << model << ',' << rays << ',' << stats.runs << ',' << warmup << ','
      << stats.mean_s << ',' << stats.stddev_s << ','
      << stats.min_s  << ',' << stats.max_s    << ','
      << stats.speedup(stats.mean_s) << ','
      << stats.efficiency(stats.mean_s, 1) << '\n';
}

static void write_comparison_summary_csv(
    const std::string& path,
    int rays,
    int warmup,
    const std::vector<std::pair<std::string, dlp::RunStats>>& summaries) {
    const auto baseline = std::find_if(
        summaries.begin(), summaries.end(),
        [](const auto& summary) { return summary.first == "sequential"; });
    if (baseline == summaries.end() || baseline->second.mean_s <= 0.0)
        throw std::runtime_error("cannot calculate comparison without a positive sequential baseline");

    for (const auto& summary : summaries) {
        if (summary.second.mean_s <= 0.0)
            throw std::runtime_error("cannot calculate comparison for model with non-positive mean time: "
                                     + summary.first);
    }

    const std::filesystem::path out(path);
    if (out.has_parent_path())
        std::filesystem::create_directories(out.parent_path());

    std::ofstream f(path);
    if (!f) throw std::runtime_error("could not open summary CSV: " + path);
    f << std::setprecision(12);
    f << "model,role,rays,runs,warmup,mean_s,stddev_s,min_s,max_s,"
         "speedup_vs_sequential,improvement_percent_vs_sequential\n";

    for (const auto& summary : summaries) {
        const bool is_baseline = summary.first == "sequential";
        const dlp::RunStats& stats = summary.second;
        const double speedup = is_baseline ? 1.0 : baseline->second.mean_s / stats.mean_s;
        const double improvement_percent = (speedup - 1.0) * 100.0;
        f << summary.first << ',' << (is_baseline ? "baseline" : "experiment") << ','
          << rays << ',' << stats.runs << ',' << warmup << ','
          << stats.mean_s << ',' << stats.stddev_s << ','
          << stats.min_s << ',' << stats.max_s << ','
          << speedup << ',' << improvement_percent << '\n';
    }
}

static std::unique_ptr<IRenderer> create_renderer(const std::string& model) {
    if (model == "sequential")
        return std::make_unique<SequentialRenderer>();
    if (model == "simd")
        return std::make_unique<SimdRenderer>();
    throw std::invalid_argument("unsupported renderer: " + model);
}

static bool frames_match(const std::vector<Vector3>& expected,
                         const std::vector<Vector3>& actual,
                         std::size_t& mismatch_index) {
    if (expected.size() != actual.size()) {
        mismatch_index = std::min(expected.size(), actual.size());
        return false;
    }

    for (std::size_t i = 0; i < expected.size(); ++i) {
        if (expected[i].x != actual[i].x ||
            expected[i].y != actual[i].y ||
            expected[i].z != actual[i].z) {
            mismatch_index = i;
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    try {
        int         runs        = 10;
        int         warmup      = 1;
        int         img_width   = constants::IMAGE_WIDTH;
        int         img_height  = constants::IMAGE_HEIGHT;
        std::string model       = "sequential";
        std::string output_csv  = constants::CSV_FILE_SEQUENTIAL;
        std::string output_img  = constants::IMAGE_FILE_SEQUENTIAL;
        bool        output_specified = false;
        bool        save_image  = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if ((arg == "--help" || arg == "-h")) {
                std::cout <<
                    "Usage: rt_benchmark [options]\n"
                    "  --width  N    image width  (default 100)\n"
                    "  --height N    image height (default 100)\n"
                    "              N rayos = width * height\n"
                    "  --model NAME  renderer: sequential, simd, or all (default sequential)\n"
                    "  --runs   N    measured repetitions (default 10)\n"
                    "  --warmup N    discarded warmup runs (default 1)\n"
                    "  --output PATH CSV path (or output directory with --model all)\n"
                    "  --image  PATH save last rendered frame as PPM\n";
                return 0;
            } else if (arg == "--width"  && i + 1 < argc) {
                img_width  = parse_positive_int(argv[++i], "--width");
            } else if (arg == "--height" && i + 1 < argc) {
                img_height = parse_positive_int(argv[++i], "--height");
            } else if (arg == "--model" && i + 1 < argc) {
                model = argv[++i];
            } else if (arg == "--runs"   && i + 1 < argc) {
                runs   = parse_positive_int(argv[++i], "--runs");
            } else if (arg == "--warmup" && i + 1 < argc) {
                warmup = std::stoi(argv[++i]);
                if (warmup < 0)
                    throw std::invalid_argument("--warmup must be >= 0");
            } else if (arg == "--output" && i + 1 < argc) {
                output_csv = argv[++i];
                output_specified = true;
            } else if (arg == "--image"  && i + 1 < argc) {
                output_img = argv[++i];
                save_image = true;
            } else {
                throw std::invalid_argument("unknown or incomplete argument: " + arg);
            }
        }

        constants::set_image_dimensions(img_width, img_height);
        const int rays = constants::IMAGE_WIDTH * constants::IMAGE_HEIGHT;

        std::vector<std::string> models;
        if (model == "all")
            models = {"sequential", "simd"};
        else if (model == "sequential" || model == "simd")
            models = {model};
        else
            throw std::invalid_argument("--model must be sequential, simd, or all");

        std::time_t now = std::time(nullptr);
        std::tm* tm_now = std::localtime(&now);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_now);

        const std::filesystem::path default_output_dir =
            std::filesystem::path(constants::CSV_FILE_SEQUENTIAL).parent_path();
        const std::filesystem::path requested_output(output_csv);
        const std::filesystem::path all_output_dir =
            output_specified ? requested_output.parent_path() : default_output_dir;
        std::vector<std::pair<std::string, std::vector<Vector3>>> final_frames;
        std::vector<std::pair<std::string, dlp::RunStats>> model_summaries;

        for (const std::string& current_model : models) {
            std::unique_ptr<IRenderer> renderer = create_renderer(current_model);
            renderer->set_camera_pos(constants::CAMERA_ORIGIN);

            for (int w = 0; w < warmup; ++w)
                renderer->render_frame();

            std::vector<double> times;
            times.reserve(runs);
            std::vector<Vector3> last_frame;
            Timer timer;
            for (int r = 0; r < runs; ++r) {
                timer.start();
                last_frame = renderer->render_frame();
                times.push_back(timer.stopAndGetMilliseconds() / 1000.0);
            }
            final_frames.emplace_back(current_model, std::move(last_frame));

            const dlp::RunStats stats = dlp::RunStats::from_times(times);
            model_summaries.emplace_back(current_model, stats);
            std::cout << "model=" << renderer->get_model_name()
                      << "  rays=" << rays
                      << "  (" << constants::IMAGE_WIDTH << 'x' << constants::IMAGE_HEIGHT << ')'
                      << "  warmup=" << warmup << "  runs=" << runs << '\n';
            for (int i = 0; i < static_cast<int>(times.size()); ++i)
                std::cout << "Execution " << (i + 1) << ": "
                          << std::fixed << std::setprecision(6) << times[i] << " s\n";

            std::cout << '\n'
                      << std::left << std::setw(12) << "mean_s"
                      << std::left << std::setw(12) << "stddev_s"
                      << std::left << std::setw(12) << "min_s"
                      << std::left << std::setw(12) << "max_s" << '\n';
            std::cout << std::fixed << std::setprecision(6)
                      << std::left << std::setw(12) << stats.mean_s
                      << std::left << std::setw(12) << stats.stddev_s
                      << std::left << std::setw(12) << stats.min_s
                      << std::left << std::setw(12) << stats.max_s << '\n';

            std::filesystem::path model_output;
            if (models.size() > 1 || !output_specified) {
                const std::filesystem::path output_dir =
                    models.size() > 1 ? all_output_dir : default_output_dir;
                model_output = output_dir / (current_model + "_benchmark.csv");
            } else {
                model_output = requested_output;
            }

            const std::filesystem::path log_dir = model_output.parent_path() / "log";
            const std::string log_csv =
                (log_dir / ("execution_" + current_model + "_" +
                            std::string(timestamp) + ".csv")).string();
            write_csv(model_output.string(), current_model, rays, warmup, times, stats);
            write_csv(log_csv, current_model, rays, warmup, times, stats);
            std::cout << "CSV: " << model_output.string() << '\n';
            std::cout << "Log: " << log_csv << '\n';

            if (save_image) {
                std::filesystem::path model_image(output_img);
                if (models.size() > 1) {
                    model_image = model_image.parent_path() /
                        (model_image.stem().string() + "_" + current_model +
                         model_image.extension().string());
                }
                write_ppm(model_image.string(), final_frames.back().second);
                std::cout << "Image: " << model_image.string() << '\n';
            }
        }

        if (final_frames.size() > 1) {
            const auto& baseline = final_frames.front();
            for (std::size_t i = 1; i < final_frames.size(); ++i) {
                std::size_t mismatch_index = 0;
                if (!frames_match(baseline.second, final_frames[i].second, mismatch_index)) {
                    throw std::runtime_error(
                        "final frame mismatch between " + baseline.first + " and " +
                        final_frames[i].first + " at pixel " +
                        std::to_string(mismatch_index));
                }
            }
            std::cout << "Frame verification passed: all " << final_frames.size()
                      << " models produced identical colors for " << rays << " pixels.\n";

            const std::string summary_path =
                (all_output_dir / "summary_benchmark.csv").string();
            write_comparison_summary_csv(summary_path, rays, warmup, model_summaries);
            std::cout << "Summary CSV: " << summary_path << '\n';
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}

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
#include "core/strategies/SequentialRenderer.h"
#include "utils/image_io.hpp"
#include "utils/Metrics.h"
#include "utils/Timer.hpp"

#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
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

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    try {
        int         runs        = 10;
        int         warmup      = 1;
        int         img_width   = constants::IMAGE_WIDTH;
        int         img_height  = constants::IMAGE_HEIGHT;
        std::string output_csv  = constants::CSV_FILE_SEQUENTIAL;
        std::string output_img  = constants::IMAGE_FILE_SEQUENTIAL;
        bool        save_image  = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if ((arg == "--help" || arg == "-h")) {
                std::cout <<
                    "Usage: rt_benchmark [options]\n"
                    "  --width  N    image width  (default 240)\n"
                    "  --height N    image height (default 160)\n"
                    "              N rayos = width * height\n"
                    "  --runs   N    measured repetitions (default 10)\n"
                    "  --warmup N    discarded warmup runs (default 1)\n"
                    "  --output PATH CSV output path\n"
                    "  --image  PATH save last rendered frame as PPM\n";
                return 0;
            } else if (arg == "--width"  && i + 1 < argc) {
                img_width  = parse_positive_int(argv[++i], "--width");
            } else if (arg == "--height" && i + 1 < argc) {
                img_height = parse_positive_int(argv[++i], "--height");
            } else if (arg == "--runs"   && i + 1 < argc) {
                runs   = parse_positive_int(argv[++i], "--runs");
            } else if (arg == "--warmup" && i + 1 < argc) {
                warmup = std::stoi(argv[++i]);
                if (warmup < 0)
                    throw std::invalid_argument("--warmup must be >= 0");
            } else if (arg == "--output" && i + 1 < argc) {
                output_csv = argv[++i];
            } else if (arg == "--image"  && i + 1 < argc) {
                output_img = argv[++i];
                save_image = true;
            } else {
                throw std::invalid_argument("unknown or incomplete argument: " + arg);
            }
        }

        constants::set_image_dimensions(img_width, img_height);
        const int rays = constants::IMAGE_WIDTH * constants::IMAGE_HEIGHT;

        std::unique_ptr<IRenderer> renderer = std::make_unique<SequentialRenderer>();
        renderer->set_camera_pos(constants::CAMERA_ORIGIN);

        // Warmup (descartado)
        for (int w = 0; w < warmup; ++w)
            renderer->render_frame();

        // Ejecuciones medidas
        std::vector<double> times;
        times.reserve(runs);
        std::vector<Vector3> last_frame;

        Timer timer;
        for (int r = 0; r < runs; ++r) {
            timer.start();
            last_frame = renderer->render_frame();
            times.push_back(timer.stopAndGetMilliseconds() / 1000.0);
        }

        const dlp::RunStats stats = dlp::RunStats::from_times(times);

        // Consola
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

        // Timestamp para el log
        std::time_t now = std::time(nullptr);
        std::tm* tm_now = std::localtime(&now);
        char timestamp[32];
        std::strftime(timestamp, sizeof(timestamp), "%Y%m%d_%H%M%S", tm_now);

        const std::filesystem::path log_dir =
            std::filesystem::path(output_csv).parent_path() / "log";
        const std::string log_csv =
            (log_dir / ("execution_" + std::string(timestamp) + ".csv")).string();

        write_csv(output_csv, renderer->get_model_name(), rays, warmup, times, stats);
        write_csv(log_csv,    renderer->get_model_name(), rays, warmup, times, stats);
        std::cout << "CSV: " << output_csv << '\n';
        std::cout << "Log: " << log_csv    << '\n';

        if (save_image) {
            write_ppm(output_img, last_frame);
            std::cout << "Image: " << output_img << '\n';
        }

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << '\n';
        return 1;
    }
}

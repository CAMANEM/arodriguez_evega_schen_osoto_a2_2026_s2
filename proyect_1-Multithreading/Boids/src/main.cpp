/**
 * @file main.cpp
 * @brief Ejecutable unificado de Boids: modelo, parametros y GUI por CLI.
 */

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include "cli/CliOptions.hpp"
#include "cli/SchemeFactory.hpp"
#include "core/CmpScheme.hpp"
#include "core/CoarseGrainedScheme.hpp"
#include "core/FineGrainedScheme.hpp"
#include "core/Flock.hpp"
#include "core/FlockingScheme.hpp"
#include "core/FrameWriter.hpp"
#include "core/SequentialScheme.hpp"
#include "core/SmtScheme.hpp"

#ifdef BOIDS_HAS_GUI
#include "visual/Renderer.hpp"
#endif

namespace {

void printMetricsRow(const BoidsMetrics& metrics) {
    const std::string workerCount =
        std::to_string(metrics.get_n_workers()) +
        (metrics.uses_virtual_workers() ? " virtuales" : "");
    std::cout << std::left << std::setw(50) << metrics.get_scheme_name()
              << std::setw(16) << workerCount
              << std::setw(15) << std::fixed << std::setprecision(3)
              << metrics.elapsed_milliseconds()
              << std::setw(12) << metrics.get_boids_processed() << "\n";
}

bool flocksMatchApprox(const Flock& a, const Flock& b, int count = -1,
                       double tolerance = 1e-6) {
    if (a.getBoidCount() != b.getBoidCount()) {
        return false;
    }
    const int comparedBoids =
        count < 0 ? a.getBoidCount() : std::min(count, a.getBoidCount());
    for (int i = 0; i < comparedBoids; ++i) {
        const Vector2D positionDifference =
            a.getBoid(i).getPosition() - b.getBoid(i).getPosition();
        const Vector2D velocityDifference =
            a.getBoid(i).getVelocity() - b.getBoid(i).getVelocity();
        if (positionDifference.magnitude() > tolerance ||
            velocityDifference.magnitude() > tolerance) {
            return false;
        }
    }
    return true;
}

void printConfigBanner(const CliOptions& options, const FlockingConfig& config) {
    std::cout << "scheme=" << schemeLabel(options.scheme)
              << " gui=" << (options.gui ? "yes" : "no")
              << " boids=" << config.getBoidCount()
              << " seed=" << options.seed;
    if (options.gui) {
        std::cout << " (loop hasta cerrar ventana)";
    } else if (options.forever) {
        std::cout << " steps=forever";
    } else {
        std::cout << " steps=" << options.steps;
    }
    std::cout << "\n";
}

int runCompare(const CliOptions& options) {
    const FlockingConfig config = options.toConfig();
    const Flock initialFlock(config, options.seed);

    std::cout << std::left << std::setw(50) << "Esquema"
              << std::setw(16) << "Trabajadores"
              << std::setw(15) << "Tiempo (ms)"
              << std::setw(12) << "Boids" << "\n";
    std::cout << std::string(93, '-') << "\n";

    Flock referenceFlock = initialFlock;
    {
        SequentialScheme scheme;
        printMetricsRow(scheme.simulateStep(referenceFlock, config));
    }

    {
        Flock flock = initialFlock;
        FineGrainedScheme scheme(options.finePartialBoids);
        printMetricsRow(scheme.simulateStep(flock, config));
        std::cout << "  -> Validacion parcial (" << options.finePartialBoids
                  << " boids contra baseline): "
                  << (flocksMatchApprox(flock, referenceFlock, options.finePartialBoids)
                          ? "SI"
                          : "NO")
                  << "\n";
    }

    {
        Flock flock = initialFlock;
        CoarseGrainedScheme scheme(static_cast<unsigned int>(std::max(1, options.workers)));
        printMetricsRow(scheme.simulateStep(flock, config));
        std::cout << "  -> Validacion (coincide con baseline secuencial): "
                  << (flocksMatchApprox(flock, referenceFlock) ? "SI" : "NO") << "\n";
    }

    {
        Flock flock = initialFlock;
        SmtScheme scheme(options.smtOversubscribe);
        printMetricsRow(scheme.simulateStep(flock, config));
        std::cout << "  -> Validacion (coincide con baseline secuencial): "
                  << (flocksMatchApprox(flock, referenceFlock) ? "SI" : "NO") << "\n";
    }

    {
        Flock flock = initialFlock;
        CmpScheme scheme;
        printMetricsRow(scheme.simulateStep(flock, config));
        std::cout << "  -> Validacion (coincide con baseline secuencial): "
                  << (flocksMatchApprox(flock, referenceFlock) ? "SI" : "NO") << "\n";
    }

    // Animacion / evidencia PPM con CMP (Demo 2: 350 pasos en frames/).
    const std::string dir =
        options.exportFramesDir.empty() ? "frames" : options.exportFramesDir;
    const int animationSteps = options.steps > 1 ? options.steps : 350;

    if (dir != "none" && animationSteps > 0) {
        std::filesystem::create_directories(dir);
        std::cout << "\nGenerando frames de animacion en " << dir << " ("
                  << animationSteps << " pasos CMP)...\n";

        Flock animatedFlock = initialFlock;
        CmpScheme animationScheme;
        int frameNumber = 0;

        for (int step = 0; step < animationSteps; ++step) {
            animationScheme.simulateStep(animatedFlock, config);
            if (step % options.frameInterval == 0) {
                std::ostringstream filename;
                filename << dir << "/frame_" << std::setw(3) << std::setfill('0')
                         << frameNumber << ".ppm";
                FrameWriter::writeFrame(animatedFlock,
                                        static_cast<int>(config.getWorldWidth()),
                                        static_cast<int>(config.getWorldHeight()),
                                        filename.str());
                ++frameNumber;
            }
        }
        std::cout << frameNumber << " frames exportados en " << dir << "/\n";
    }

    return 0;
}

int runHeadless(const CliOptions& options) {
    if (options.scheme == RunScheme::Compare) {
        return runCompare(options);
    }

    const FlockingConfig config = options.toConfig();
    printConfigBanner(options, config);

    Flock flock(config, options.seed);
    std::unique_ptr<FlockingScheme> scheme = createScheme(options);

    Flock referenceFlock = flock;
    if (options.validate) {
        SequentialScheme sequential;
        sequential.simulateStep(referenceFlock, config);
    }

    if (!options.exportFramesDir.empty()) {
        std::filesystem::create_directories(options.exportFramesDir);
    }

    long long step = 0;
    const bool infinite = options.forever;
    const int limit = options.steps;

    while (infinite || step < limit) {
        const BoidsMetrics metrics = scheme->simulateStep(flock, config);

        if (step == 0 || (!infinite && step + 1 == limit) || (step % 100 == 0 && step > 0)) {
            std::cout << "step=" << step << "  ";
            printMetricsRow(metrics);
        }

        if (options.validate && step == 0) {
            const int compareCount =
                options.scheme == RunScheme::Fine ? options.finePartialBoids : -1;
            std::cout << "  -> Validacion vs secuencial: "
                      << (flocksMatchApprox(flock, referenceFlock, compareCount) ? "SI"
                                                                                 : "NO")
                      << "\n";
        }

        if (!options.exportFramesDir.empty() &&
            (step % options.frameInterval == 0)) {
            std::ostringstream filename;
            filename << options.exportFramesDir << "/frame_" << std::setw(6)
                     << std::setfill('0') << step << ".ppm";
            FrameWriter::writeFrame(flock, static_cast<int>(config.getWorldWidth()),
                                    static_cast<int>(config.getWorldHeight()),
                                    filename.str());
        }

        ++step;
    }

    return 0;
}

int runGui(const CliOptions& options) {
#ifdef BOIDS_HAS_GUI
    if (options.scheme == RunScheme::Compare) {
        std::cerr << "compare no admite GUI\n";
        return 1;
    }

    const FlockingConfig config = options.toConfig();
    printConfigBanner(options, config);

    std::unique_ptr<Renderer> renderer = createRaylibRenderer();
    std::ostringstream title;
    title << "Flocking - " << schemeLabel(options.scheme);
    if (!renderer->init(static_cast<int>(config.getWorldWidth()),
                        static_cast<int>(config.getWorldHeight()), title.str().c_str())) {
        std::cerr << "Error al inicializar la ventana\n";
        return 1;
    }

    Flock flock(config, options.seed);
    std::unique_ptr<FlockingScheme> scheme = createScheme(options);
    std::cout << "Modalidad grafica: " << scheme->getSchemeName() << "\n";

    while (!renderer->shouldClose()) {
        scheme->simulateStep(flock, config);
        renderer->beginDrawing();
        renderer->drawFlock(flock);
        renderer->endDrawing();
    }

    renderer->shutdown();
    return 0;
#else
    (void)options;
    std::cerr << "Este binario se compile sin Raylib (--gui no disponible).\n"
              << "Reconfigure con -DBOIDS_BUILD_VISUAL=ON y red para FetchContent.\n";
    return 1;
#endif
}

} // namespace

int main(int argc, char** argv) {
    try {
        const CliOptions options = parseCli(argc, argv);
        if (options.showHelp) {
            printCliHelp(argv[0]);
            return 0;
        }
        if (options.gui) {
            return runGui(options);
        }
        return runHeadless(options);
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        printCliHelp(argv[0]);
        return 1;
    }
}

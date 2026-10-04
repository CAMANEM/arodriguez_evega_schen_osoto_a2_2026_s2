#include "cli/CliOptions.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

bool startsWith(const std::string& value, const char* prefix) {
    return value.rfind(prefix, 0) == 0;
}

std::string requireValue(int argc, char** argv, int& i, const char* flag) {
    if (i + 1 >= argc) {
        throw std::runtime_error(std::string("Falta valor para ") + flag);
    }
    return argv[++i];
}

int parseInt(const std::string& text, const char* flag) {
    try {
        size_t idx = 0;
        const int value = std::stoi(text, &idx);
        if (idx != text.size()) {
            throw std::runtime_error("basura");
        }
        return value;
    } catch (...) {
        throw std::runtime_error(std::string("Entero invalido en ") + flag + ": " + text);
    }
}

unsigned int parseUInt(const std::string& text, const char* flag) {
    const int value = parseInt(text, flag);
    if (value < 0) {
        throw std::runtime_error(std::string("Entero no negativo requerido en ") + flag);
    }
    return static_cast<unsigned int>(value);
}

double parseDouble(const std::string& text, const char* flag) {
    try {
        size_t idx = 0;
        const double value = std::stod(text, &idx);
        if (idx != text.size()) {
            throw std::runtime_error("basura");
        }
        return value;
    } catch (...) {
        throw std::runtime_error(std::string("Numero invalido en ") + flag + ": " + text);
    }
}

RunScheme parseScheme(const std::string& text) {
    if (text == "sequential" || text == "seq") {
        return RunScheme::Sequential;
    }
    if (text == "fine" || text == "fine-grained" || text == "fgmt") {
        return RunScheme::Fine;
    }
    if (text == "coarse" || text == "coarse-grained" || text == "cgmt") {
        return RunScheme::Coarse;
    }
    if (text == "smt") {
        return RunScheme::Smt;
    }
    if (text == "cmp") {
        return RunScheme::Cmp;
    }
    if (text == "compare" || text == "all" || text == "demo2") {
        return RunScheme::Compare;
    }
    throw std::runtime_error("Esquema desconocido: " + text +
                             " (sequential|fine|coarse|smt|cmp|compare)");
}

} // namespace

FlockingConfig CliOptions::toConfig() const {
    return FlockingConfig(boidCount, worldWidth, worldHeight, perceptionRadius,
                          separationRadius, maxSpeed, maxForce, separationWeight,
                          alignmentWeight, cohesionWeight, deltaTime);
}

const char* schemeLabel(RunScheme scheme) {
    switch (scheme) {
    case RunScheme::Sequential:
        return "sequential";
    case RunScheme::Fine:
        return "fine";
    case RunScheme::Coarse:
        return "coarse";
    case RunScheme::Smt:
        return "smt";
    case RunScheme::Cmp:
        return "cmp";
    case RunScheme::Compare:
        return "compare";
    }
    return "unknown";
}

void printCliHelp(const char* argv0) {
    std::cout
        << "Uso: " << argv0 << " [opciones]\n"
        << "\n"
        << "Un solo ejecutable: el modelo, los parametros y la GUI se eligen por CLI.\n"
        << "\n"
        << "Modelo de ejecucion:\n"
        << "  --scheme sequential|fine|coarse|smt|cmp|compare\n"
        << "      sequential  Sistema base sin hilos\n"
        << "      fine        Grano fino simulado (round-robin por vecino)\n"
        << "      coarse      Dummy grano grueso\n"
        << "      smt         Dummy SMT (sobresuscripcion)\n"
        << "      cmp         Dummy CMP (hilos ~ nucleos logicos)\n"
        << "      compare     Demo 2: un paso de cada esquema + validacion\n"
        << "\n"
        << "Interfaz:\n"
        << "  --gui / --no-gui     Con o sin ventana Raylib (default: --no-gui)\n"
        << "\n"
        << "Duracion (solo --no-gui; con --gui corre hasta cerrar la ventana):\n"
        << "  --steps N            Cantidad de pasos (default: 1; compare usa 1 por esquema)\n"
        << "  --forever            Bucle infinito (Ctrl+C para salir)\n"
        << "\n"
        << "Parametros del problema:\n"
        << "  --boids N            Cantidad de boids (default: 70; GUI default visual: 250)\n"
        << "  --width W --height H Mundo / ventana\n"
        << "  --perception R --separation R\n"
        << "  --max-speed S --max-force F\n"
        << "  --sep-weight W --align-weight W --cohesion-weight W\n"
        << "  --dt T               Paso de integracion\n"
        << "  --seed N             Semilla del enjambre inicial\n"
        << "\n"
        << "Trabajadores por modelo:\n"
        << "  --workers N          Hilos coarse (default: 4)\n"
        << "  --partial N          Limite de contextos en fine (0 = flock completo)\n"
        << "  --oversubscribe N    Factor SMT (default: 2)\n"
        << "\n"
        << "Salida / evidencia:\n"
        << "  --export-frames DIR  Exporta PPM cada --frame-interval pasos\n"
        << "  --frame-interval N   (default: 5)\n"
        << "  --validate           Tras 1 paso, comparar contra secuencial\n"
        << "  -h, --help           Esta ayuda\n"
        << "\n"
        << "Ejemplos:\n"
        << "  " << argv0 << " --scheme sequential --forever\n"
        << "  " << argv0 << " --scheme cmp --gui\n"
        << "  " << argv0 << " --scheme sequential --gui --boids 250\n"
        << "  " << argv0 << " --scheme compare --export-frames frames --steps 350\n"
        << "  " << argv0 << " --scheme fine --validate --boids 40\n"
        << "  " << argv0 << " --scheme fine --partial 20 --steps 1\n"
        << "  " << argv0 << " --scheme coarse --workers 8 --steps 1000 --boids 200\n";
}

CliOptions parseCli(int argc, char** argv) {
    CliOptions options;
    bool boidsSet = false;
    bool widthSet = false;
    bool heightSet = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            options.showHelp = true;
            return options;
        }
        if (arg == "--gui") {
            options.gui = true;
            continue;
        }
        if (arg == "--no-gui") {
            options.gui = false;
            continue;
        }
        if (arg == "--forever") {
            options.forever = true;
            continue;
        }
        if (arg == "--validate") {
            options.validate = true;
            continue;
        }
        if (arg == "--scheme") {
            options.scheme = parseScheme(requireValue(argc, argv, i, "--scheme"));
            continue;
        }
        if (startsWith(arg, "--scheme=")) {
            options.scheme = parseScheme(arg.substr(9));
            continue;
        }
        if (arg == "--steps") {
            options.steps = parseInt(requireValue(argc, argv, i, "--steps"), "--steps");
            continue;
        }
        if (arg == "--seed") {
            options.seed = parseInt(requireValue(argc, argv, i, "--seed"), "--seed");
            continue;
        }
        if (arg == "--workers" || arg == "--threads") {
            options.workers = parseInt(requireValue(argc, argv, i, arg.c_str()), arg.c_str());
            continue;
        }
        if (arg == "--partial") {
            options.finePartialBoids =
                parseInt(requireValue(argc, argv, i, "--partial"), "--partial");
            continue;
        }
        if (arg == "--oversubscribe") {
            options.smtOversubscribe =
                parseUInt(requireValue(argc, argv, i, "--oversubscribe"), "--oversubscribe");
            continue;
        }
        if (arg == "--boids") {
            options.boidCount = parseInt(requireValue(argc, argv, i, "--boids"), "--boids");
            boidsSet = true;
            continue;
        }
        if (arg == "--width") {
            options.worldWidth =
                parseDouble(requireValue(argc, argv, i, "--width"), "--width");
            widthSet = true;
            continue;
        }
        if (arg == "--height") {
            options.worldHeight =
                parseDouble(requireValue(argc, argv, i, "--height"), "--height");
            heightSet = true;
            continue;
        }
        if (arg == "--perception") {
            options.perceptionRadius =
                parseDouble(requireValue(argc, argv, i, "--perception"), "--perception");
            continue;
        }
        if (arg == "--separation") {
            options.separationRadius =
                parseDouble(requireValue(argc, argv, i, "--separation"), "--separation");
            continue;
        }
        if (arg == "--max-speed") {
            options.maxSpeed =
                parseDouble(requireValue(argc, argv, i, "--max-speed"), "--max-speed");
            continue;
        }
        if (arg == "--max-force") {
            options.maxForce =
                parseDouble(requireValue(argc, argv, i, "--max-force"), "--max-force");
            continue;
        }
        if (arg == "--sep-weight") {
            options.separationWeight =
                parseDouble(requireValue(argc, argv, i, "--sep-weight"), "--sep-weight");
            continue;
        }
        if (arg == "--align-weight") {
            options.alignmentWeight =
                parseDouble(requireValue(argc, argv, i, "--align-weight"), "--align-weight");
            continue;
        }
        if (arg == "--cohesion-weight") {
            options.cohesionWeight = parseDouble(
                requireValue(argc, argv, i, "--cohesion-weight"), "--cohesion-weight");
            continue;
        }
        if (arg == "--dt") {
            options.deltaTime = parseDouble(requireValue(argc, argv, i, "--dt"), "--dt");
            continue;
        }
        if (arg == "--export-frames") {
            options.exportFramesDir = requireValue(argc, argv, i, "--export-frames");
            continue;
        }
        if (arg == "--frame-interval") {
            options.frameInterval =
                parseInt(requireValue(argc, argv, i, "--frame-interval"), "--frame-interval");
            continue;
        }

        throw std::runtime_error("Argumento desconocido: " + arg + " (use --help)");
    }

    // Defaults visuales mas comodos cuando se pide GUI sin override.
    if (options.gui) {
        if (!boidsSet) {
            options.boidCount = 250;
        }
        if (!widthSet) {
            options.worldWidth = 1200.0;
        }
        if (!heightSet) {
            options.worldHeight = 800.0;
        }
        if (options.separationRadius == 20.0 && options.separationWeight == 1.0) {
            options.separationRadius = 45.0;
            options.separationWeight = 6.0;
            options.alignmentWeight = 0.7;
            options.cohesionWeight = 0.1;
            options.maxForce = 0.15;
        }
    }

    if (options.forever && options.gui) {
        // Con GUI el bucle ya es hasta cerrar ventana; forever es implicito.
        options.forever = true;
    }
    if (options.steps < 0) {
        throw std::runtime_error("--steps debe ser >= 0");
    }
    if (options.boidCount <= 0) {
        throw std::runtime_error("--boids debe ser > 0");
    }
    if (options.gui && options.scheme == RunScheme::Compare) {
        throw std::runtime_error("--scheme compare no admite --gui; use un esquema concreto");
    }
    if (options.frameInterval <= 0) {
        throw std::runtime_error("--frame-interval debe ser > 0");
    }

    return options;
}

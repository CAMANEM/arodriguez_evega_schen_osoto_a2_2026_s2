#include "cli/CliOptions.hpp"

#include <cstdlib>
#include <initializer_list>
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

/**
 * @brief Extrae el valor de `--flag valor` o `--flag=valor` si arg coincide.
 * @return true si alguno de los nombres coincidió.
 */
bool takeFlagValue(int argc, char** argv, int& i, const std::string& arg,
                   std::initializer_list<const char*> names, std::string& value) {
    for (const char* name : names) {
        if (arg == name) {
            value = requireValue(argc, argv, i, name);
            return true;
        }
        const std::string equalsPrefix = std::string(name) + "=";
        if (startsWith(arg, equalsPrefix.c_str())) {
            value = arg.substr(equalsPrefix.size());
            return true;
        }
    }
    return false;
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
        << "      coarse      Grano grueso (hilos reales + stalls opcionales)\n"
        << "      smt         SMT por sobre-suscripcion (T = L x F)\n"
        << "      cmp         CMP directo (T = L = hardware_concurrency)\n"
        << "      compare     Demo 2: un paso de cada esquema + validacion\n"
        << "\n"
        << "Interfaz:\n"
        << "  --gui / --no-gui     Con o sin ventana Raylib (default: --no-gui)\n"
        << "\n"
        << "Duracion (solo --no-gui; con --gui corre hasta cerrar la ventana):\n"
        << "  --steps N            Cantidad de pasos (default: 1; compare usa 1 por esquema)\n"
        << "  --forever            Bucle infinito (Ctrl+C para salir)\n"
        << "\n"
        << "Parametros del problema (validos con --gui y --no-gui):\n"
        << "  --boids N, --bodies N, -n N\n"
        << "                       Cantidad de boids (default: 70; GUI sin flag: 250)\n"
        << "  --width W --height H Mundo / ventana\n"
        << "  --perception R --separation R\n"
        << "  --max-speed S --max-force F\n"
        << "  --sep-weight W --align-weight W --cohesion-weight W\n"
        << "  --dt T               Paso de integracion\n"
        << "  --seed N             Semilla del enjambre / stalls aleatorios\n"
        << "  Tambien se acepta --flag=valor (ej. --boids=80).\n"
        << "\n"
        << "Trabajadores por modelo:\n"
        << "  --workers N          Hilos coarse (default: 4); no define CMP\n"
        << "  --partial N          Limite de contextos en fine (0 = flock completo)\n"
        << "  --oversubscribe F    Factor SMT: T = L x F (default: 2, minimo 1)\n"
        << "  cmp                  T automatico = L (sin --workers ni --oversubscribe)\n"
        << "\n"
        << "Stalls didacticos (solo --scheme coarse, tras boid completo):\n"
        << "  --stall-every K      Stall cada K boids completados (0 = off)\n"
        << "  --stall-probability P  Probabilidad [0,1] tras cada boid (0 = off)\n"
        << "  --stall-ms X         Duracion fija del stall en ms (default: 1)\n"
        << "  --stall-ms-min A --stall-ms-max B\n"
        << "                       Duracion uniforme en [A,B] ms (ambos o ninguno)\n"
        << "  --log-checkpoints    Imprime el checkpoint manual en cada stall\n"
        << "  Precedencia de disparo: (cada K) OR (sorteo P). Si K=0 y P=0, sin stalls.\n"
        << "  Precedencia de duracion: rango min/max si ambos >= 0; si no, --stall-ms.\n"
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
        << "  " << argv0 << " --scheme fine --gui -n 80 --perception 60 --separation 25\n"
        << "  " << argv0 << " --scheme fine --no-gui --bodies 120 --seed 7 --steps 50\n"
        << "  " << argv0 << " --scheme compare --export-frames frames --steps 350\n"
        << "  " << argv0 << " --scheme fine --validate --boids=40\n"
        << "  " << argv0 << " --scheme fine --partial 20 --steps 1\n"
        << "  " << argv0 << " --scheme coarse --workers 8 --steps 1000 --boids 200\n"
        << "  " << argv0 << " --scheme coarse --workers 4 --stall-every 10 --stall-ms 2 --validate\n"
        << "  " << argv0 << " --scheme coarse --stall-probability 0.1 --stall-ms-min 1 --stall-ms-max 5\n"
        << "  " << argv0 << " --scheme smt --oversubscribe 2 --validate --boids 120\n"
        << "  " << argv0 << " --scheme smt --oversubscribe 4 --no-gui --steps 50 --boids 200\n"
        << "  " << argv0 << " --scheme smt --gui --oversubscribe 2\n"
        << "  " << argv0 << " --scheme cmp --validate --boids 120 --steps 1\n"
        << "  " << argv0 << " --scheme cmp --no-gui --boids 200 --steps 50\n"
        << "  " << argv0 << " --scheme cmp --gui --boids 300\n";
}

CliOptions parseCli(int argc, char** argv) {
    CliOptions options;
    bool boidsSet = false;
    bool widthSet = false;
    bool heightSet = false;
    bool physicsSet = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        std::string value;

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
        if (takeFlagValue(argc, argv, i, arg, {"--scheme"}, value)) {
            options.scheme = parseScheme(value);
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--steps"}, value)) {
            options.steps = parseInt(value, "--steps");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--seed"}, value)) {
            options.seed = parseInt(value, "--seed");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--workers", "--threads"}, value)) {
            options.workers = parseInt(value, "--workers");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--partial"}, value)) {
            options.finePartialBoids = parseInt(value, "--partial");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--oversubscribe"}, value)) {
            options.smtOversubscribe = parseUInt(value, "--oversubscribe");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--stall-every"}, value)) {
            options.stallEvery = parseInt(value, "--stall-every");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--stall-probability"}, value)) {
            options.stallProbability = parseDouble(value, "--stall-probability");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--stall-ms"}, value)) {
            options.stallMs = parseDouble(value, "--stall-ms");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--stall-ms-min"}, value)) {
            options.stallMsMin = parseDouble(value, "--stall-ms-min");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--stall-ms-max"}, value)) {
            options.stallMsMax = parseDouble(value, "--stall-ms-max");
            continue;
        }
        if (arg == "--log-checkpoints") {
            options.logCoarseCheckpoints = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--boids", "--bodies", "--n", "-n"}, value)) {
            options.boidCount = parseInt(value, "--boids");
            boidsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--width"}, value)) {
            options.worldWidth = parseDouble(value, "--width");
            widthSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--height"}, value)) {
            options.worldHeight = parseDouble(value, "--height");
            heightSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--perception"}, value)) {
            options.perceptionRadius = parseDouble(value, "--perception");
            physicsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--separation"}, value)) {
            options.separationRadius = parseDouble(value, "--separation");
            physicsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--max-speed"}, value)) {
            options.maxSpeed = parseDouble(value, "--max-speed");
            physicsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--max-force"}, value)) {
            options.maxForce = parseDouble(value, "--max-force");
            physicsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--sep-weight"}, value)) {
            options.separationWeight = parseDouble(value, "--sep-weight");
            physicsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--align-weight"}, value)) {
            options.alignmentWeight = parseDouble(value, "--align-weight");
            physicsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--cohesion-weight"}, value)) {
            options.cohesionWeight = parseDouble(value, "--cohesion-weight");
            physicsSet = true;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--dt"}, value)) {
            options.deltaTime = parseDouble(value, "--dt");
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--export-frames"}, value)) {
            options.exportFramesDir = value;
            continue;
        }
        if (takeFlagValue(argc, argv, i, arg, {"--frame-interval"}, value)) {
            options.frameInterval = parseInt(value, "--frame-interval");
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
        if (!physicsSet) {
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
    if (options.workers < 1) {
        throw std::runtime_error("--workers debe ser >= 1");
    }
    // SMT: F >= 1; 0 (u omitido inválido) se clampa a 1 para barridos seguros.
    if (options.smtOversubscribe < 1u) {
        options.smtOversubscribe = 1u;
    }

    // Validación/clamp de stalls (también usada por CoarseGrainedScheme).
    {
        int stallEvery = options.stallEvery;
        double stallProbability = options.stallProbability;
        double stallMs = options.stallMs;
        double stallMsMin = options.stallMsMin;
        double stallMsMax = options.stallMsMax;
        // Incluye StallPolicy.hpp solo vía validación local para no acoplar el
        // parseo a la lógica de hilos: replicamos las reglas documentadas.
        if (stallEvery < 0) {
            stallEvery = 0;
        }
        if (stallProbability < 0.0 || stallProbability > 1.0) {
            throw std::runtime_error("--stall-probability debe estar en [0,1]");
        }
        if (stallMs < 0.0) {
            throw std::runtime_error("--stall-ms debe ser >= 0");
        }
        const bool minSet = stallMsMin >= 0.0;
        const bool maxSet = stallMsMax >= 0.0;
        if (minSet != maxSet) {
            throw std::runtime_error(
                "Debe indicar ambos --stall-ms-min y --stall-ms-max, o ninguno");
        }
        if (minSet && maxSet && stallMsMin > stallMsMax) {
            throw std::runtime_error(
                "--stall-ms-min no puede ser mayor que --stall-ms-max");
        }
        options.stallEvery = stallEvery;
        options.stallProbability = stallProbability;
        options.stallMs = stallMs;
    }

    return options;
}

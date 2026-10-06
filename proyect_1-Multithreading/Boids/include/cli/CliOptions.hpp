#ifndef CLI_OPTIONS_HPP
#define CLI_OPTIONS_HPP

#include <string>

#include "core/FlockingConfig.hpp"

/**
 * @brief Modo de ejecución seleccionado por línea de comandos.
 *
 * `compare` reproduce la evidencia de Demo 2: un paso de cada esquema
 * contra el baseline secuencial (y opcionalmente frames).
 */
enum class RunScheme {
    Sequential,
    Fine,
    Coarse,
    Smt,
    Cmp,
    Compare
};

/**
 * @brief Opciones parseadas del ejecutable unificado `boids`.
 *
 * El binario es siempre el mismo; el modelo, los parámetros del enjambre
 * y la presencia de GUI se eligen en tiempo de ejecución.
 */
struct CliOptions {
    RunScheme scheme = RunScheme::Sequential;
    bool gui = false;
    bool forever = false;
    bool validate = false;
    bool showHelp = false;

    int steps = 1;
    int seed = 42;
    int workers = 4;
    int finePartialBoids = 0;
    unsigned int smtOversubscribe = 2;
    int frameInterval = 5;
    std::string exportFramesDir;

    // Parámetros de stall didáctico para Coarse-Grained.
    int stallEvery = 0;                 // Stall cada K boids (0 = off). 
    double stallProbability = 0.0;      // Probabilidad [0,1] tras cada boid. 
    double stallMs = 1.0;               // Duración fija (ms). 
    double stallMsMin = -1.0;           // Mínimo aleatorio (-1 = usar fija). 
    double stallMsMax = -1.0;           // Máximo aleatorio (-1 = usar fija). 
    bool logCoarseCheckpoints = false;  // Traza de checkpoints en demo. 

    int boidCount = 70;
    double worldWidth = 450.0;
    double worldHeight = 450.0;
    double perceptionRadius = 50.0;
    double separationRadius = 20.0;
    double maxSpeed = 2.6;
    double maxForce = 0.18;
    double separationWeight = 1.0;
    double alignmentWeight = 1.4;
    double cohesionWeight = 0.8;
    double deltaTime = 1.0;

    FlockingConfig toConfig() const;
};

/**
 * @brief Interpreta argc/argv. Lanza std::runtime_error si hay error.
 *
 * Cantidad de boids: `--boids`, `--bodies` o `-n` (también `--flag=valor`).
 * El resto de variables del problema (`--perception`, `--separation`, pesos,
 * mundo, `--seed`, `--partial`, etc.) aplican igual con `--gui` y `--no-gui`.
 */
CliOptions parseCli(int argc, char** argv);

/** @brief Texto de ayuda del ejecutable. */
void printCliHelp(const char* argv0);

/** @return Nombre corto del esquema para logs / título de ventana. */
const char* schemeLabel(RunScheme scheme);

#endif // CLI_OPTIONS_HPP

/**
 * @file main_benchmark.cpp
 * @brief Punto de entrada del framework experimental de Flocking/Boids
 *        para la Demostración 2 del Proyecto Grupal 1 (CE4302).
 *
 * Este programa:
 *   1. Define la configuración del enjambre (variables críticas: cantidad
 *      de boids, radio de percepción, pesos de las reglas de Reynolds).
 *   2. Ejecuta un único paso de simulación con cada esquema (Secuencial,
 *      Grano Fino parcial, Grano Grueso, SMT, CMP) partiendo del MISMO
 *      estado inicial, para medir tiempos y validar que el resultado
 *      paralelo coincida con el secuencial.
 *   3. Ejecuta una simulación larga (350 pasos) usando CmpScheme y
 *      exporta un frame cada pocos pasos, como evidencia visual funcional
 *      del enjambre formándose y moviéndose.
 *
 * 
 */

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "core/CmpScheme.hpp"
#include "core/CoarseGrainedScheme.hpp"
#include "core/FineGrainedScheme.hpp"
#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/FlockingScheme.hpp"
#include "core/FrameWriter.hpp"
#include "core/SequentialScheme.hpp"
#include "core/SmtScheme.hpp"

namespace {

/**
 * @brief Imprime una fila de métricas preliminares del benchmark.
 * @param metrics Resultado del paso que se desea mostrar.
 */
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

/**
 * @brief Compara posición y velocidad de un rango de dos enjambres.
 * @param a Primer enjambre.
 * @param b Segundo enjambre.
 * @param count Cantidad inicial de boids a comparar; un valor negativo
 *        compara el enjambre completo.
 * @param tolerance Tolerancia para posición y velocidad.
 * @return true cuando ambos estados coinciden dentro de la tolerancia.
 */
bool flocksMatchApprox(const Flock& a, const Flock& b, int count = -1,
                       double tolerance = 1e-6) {
    if (a.getBoidCount() != b.getBoidCount()) {
        return false;
    }
    const int comparedBoids = count < 0 ? a.getBoidCount()
                                        : std::min(count, a.getBoidCount());
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

} // namespace

/**
 * @brief Ejecuta los cinco esquemas y genera evidencia no gráfica.
 */
int main() {
    // --- Variables críticas identificadas para la Demostración 2 ---
    // boidCount: volumen total de trabajo (cada paso evalúa hasta
    //   boidCount*(boidCount-1) pares en el peor caso, brute-force).
    // perceptionRadius / separationRadius: determinan cuántos vecinos
    //   REALES evalúa cada boid; en un enjambre ya agrupado, los boids del
    //   centro del grupo tienen muchos más vecinos que los dispersos en
    //   los bordes, lo que introduce una densidad de trabajo no uniforme
    //   (similar en espíritu a la irregularidad de Mandelbrot, pero
    //   originada por la dinámica espacial en vez del tiempo de escape).
    const int boidCount = 70;
    const FlockingConfig config(boidCount, /*worldWidth=*/450.0, /*worldHeight=*/450.0,
                                 /*perceptionRadius=*/50.0, /*separationRadius=*/20.0,
                                 /*maxSpeed=*/2.6, /*maxForce=*/0.18,
                                 /*separationWeight=*/1.0, /*alignmentWeight=*/1.4,
                                 /*cohesionWeight=*/0.8, /*deltaTime=*/1.0);

    std::cout << std::left << std::setw(50) << "Esquema"
              << std::setw(16) << "Trabajadores"
              << std::setw(15) << "Tiempo (ms)"
              << std::setw(12) << "Boids" << "\n";
    std::cout << std::string(93, '-') << "\n";

    // Mismo enjambre inicial para todos los esquemas, para poder validar
    // correctitud comparando resultados tras UN solo paso.
    const Flock initialFlock(config, /*seed=*/7);

    // 1. Esquema secuencial (baseline obligatorio de la Demo 2, y referencia de correctitud).
    Flock referenceFlock = initialFlock;
    {
        SequentialScheme scheme;
        const BoidsMetrics metrics = scheme.simulateStep(referenceFlock, config);
        printMetricsRow(metrics);
    }

    // 2. Esquema de grano fino: demostración dummy/parcial (round-robin por vecino).
    {
        Flock flock = initialFlock;
        FineGrainedScheme scheme(/*partialBoidCount=*/20);
        const BoidsMetrics metrics = scheme.simulateStep(flock, config);
        printMetricsRow(metrics);
        std::cout << "  -> Validacion parcial (20 boids contra baseline): "
                  << (flocksMatchApprox(flock, referenceFlock, 20) ? "SI" : "NO")
                  << "\n";
    }

    // 3. Esquema de grano grueso: hilos tradicionales sobre el enjambre completo.
    {
        Flock flock = initialFlock;
        CoarseGrainedScheme scheme(/*threadCount=*/4);
        const BoidsMetrics metrics = scheme.simulateStep(flock, config);
        printMetricsRow(metrics);
        std::cout << "  -> Validacion (coincide con baseline secuencial): "
                  << (flocksMatchApprox(flock, referenceFlock) ? "SI" : "NO") << "\n";
    }

    // 4. Esquema SMT: sobre-suscripción de hilos lógicos.
    {
        Flock flock = initialFlock;
        SmtScheme scheme(/*oversubscriptionFactor=*/2);
        const BoidsMetrics metrics = scheme.simulateStep(flock, config);
        printMetricsRow(metrics);
        std::cout << "  -> Validacion (coincide con baseline secuencial): "
                  << (flocksMatchApprox(flock, referenceFlock) ? "SI" : "NO") << "\n";
    }

    // 5. Esquema CMP: un hilo por núcleo lógico reportado por el SO.
    {
        Flock flock = initialFlock;
        CmpScheme scheme;
        const BoidsMetrics metrics = scheme.simulateStep(flock, config);
        printMetricsRow(metrics);
        std::cout << "  -> Validacion (coincide con baseline secuencial): "
                  << (flocksMatchApprox(flock, referenceFlock) ? "SI" : "NO") << "\n";
    }

    // --- Evidencia visual: simulación larga con exportación de frames ---
    std::cout << "\nGenerando frames de animacion...\n";
    std::filesystem::create_directory("frames");

    Flock animatedFlock = initialFlock;
    CmpScheme animationScheme;
    const int totalSteps = 350;
    const int frameInterval = 5;
    int frameNumber = 0;

    for (int step = 0; step < totalSteps; ++step) {
        animationScheme.simulateStep(animatedFlock, config);
        if (step % frameInterval == 0) {
            std::ostringstream filename;
            filename << "frames/frame_" << std::setw(3) << std::setfill('0') << frameNumber << ".ppm";
            FrameWriter::writeFrame(animatedFlock, static_cast<int>(config.getWorldWidth()),
                                     static_cast<int>(config.getWorldHeight()), filename.str());
            ++frameNumber;
        }
    }

    std::cout << frameNumber << " frames exportados en frames/ (evidencia visual funcional).\n";
    std::cout << "Para verlos como animacion: convert -delay 6 -loop 0 frames/frame_*.ppm flock.gif\n";
    return 0;
}

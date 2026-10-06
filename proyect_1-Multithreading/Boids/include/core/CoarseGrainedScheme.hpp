#ifndef COARSE_GRAINED_SCHEME_HPP
#define COARSE_GRAINED_SCHEME_HPP

#include <cstdint>
#include <vector>

#include "core/CoarseWorkerCheckpoint.hpp"
#include "core/StallPolicy.hpp"
#include "core/ThreadedScheme.hpp"
#include "core/Vector2D.hpp"

/**
 * @brief Multihilo de grano grueso con hilos reales del SO (std::thread).
 *
 * Parte el flock en bloques grandes, calcula boids completos de corrido y solo
 * cede ante bloqueos costosos: stalls didácticos inyectados tras
 * terminar un boid y la barrera final join(). No usa round-robin por quantum
 * ni corta a mitad del cálculo de vecinos.
 *
 * El checkpoint manual en cada stall es un espejo académico del save implícito
 * que el sistema operativo ya realiza al bloquear el hilo (stack/PCB).
 */
class CoarseGrainedScheme : public ThreadedScheme {
public:
    /**
     * @brief Parámetros de construcción del esquema Coarse-Grained.
     */
    struct Options {
        unsigned int threadCount = 4;       // Hilos reales (--workers). 
        int stallEveryBoids = 0;            // Stall cada K boids (0 = off). 
        double stallProbability = 0.0;      // Probabilidad de stall [0,1]. 
        double stallMilliseconds = 1.0;     // Duración fija de stall (ms). 
        double stallMillisecondsMin = -1.0; // Mínimo aleatorio (-1 = off). 
        double stallMillisecondsMax = -1.0; // Máximo aleatorio (-1 = off). 
        std::uint32_t seed = 42;            // Semilla para stalls aleatorios. 
        bool logCheckpoints = false;        // Traza de checkpoints (demo).
    };

    /**
     * @brief Construye el esquema con cantidad fija de workers (sin stalls).
     * @param threadCount Cantidad de hilos tradicionales (>= 1).
     */
    explicit CoarseGrainedScheme(unsigned int threadCount = 4);

    /**
     * @brief Construye el esquema con política de stall parametrizable.
     * @param options Workers, stalls, semilla y flags de demo.
     */
    explicit CoarseGrainedScheme(const Options& options);

    /**
     * @brief Un paso: partición → workers reales + stalls → join → integrar.
     * @param flock Enjambre a actualizar.
     * @param config Configuración de flocking.
     * @return Métricas con workers, tiempo y contadores de stall.
     */
    BoidsMetrics simulateStep(Flock& flock, const FlockingConfig& config) override;

    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

    /** @return Opciones efectivas del esquema (para métricas/CLI). */
    const Options& getOptions() const { return options_; }

protected:
    unsigned int computeThreadCount(const Flock& flock) const override;

private:
    Options options_;

    /**
     * @brief Acumuladores por worker (escritura disjunta; se suman tras join).
     */
    struct WorkerStallStats {
        int stallCount = 0;
        double stallTimeMs = 0.0;
        int checkpointsRecorded = 0;
    };

    /**
     * @brief Loop de un hilo real: boids completos + stall/checkpoint.
     * @param workerId Identificador del worker.
     * @param startIndex Inicio inclusivo del bloque.
     * @param endIndex Fin exclusivo del bloque.
     * @param flock Enjambre de solo lectura en esta fase.
     * @param config Configuración de flocking.
     * @param steeringForces Vector compartido de fuerzas (rangos disjuntos).
     * @param stats Salida de métricas de stall de este worker.
     * @param lastCheckpoint Si logCheckpoints, guarda el último snapshot.
     */
    void runWorker(int workerId, int startIndex, int endIndex, const Flock& flock,
                   const FlockingConfig& config, std::vector<Vector2D>& steeringForces,
                   WorkerStallStats& stats,
                   CoarseWorkerCheckpoint* lastCheckpoint) const;
};

#endif // COARSE_GRAINED_SCHEME_HPP

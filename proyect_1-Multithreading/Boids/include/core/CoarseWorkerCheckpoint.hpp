#ifndef COARSE_WORKER_CHECKPOINT_HPP
#define COARSE_WORKER_CHECKPOINT_HPP

#include <cstdint>
#include <string>

/**
 * @brief Checkpoint manual didáctico del progreso de un worker Coarse-Grained.
 *
 * En un sistema real, al bloquearse el hilo el sistema operativo conserva
 * automáticamente su contexto (stack, registros, PCB). Este checkpoint manual
 * no sustituye ese mecanismo: lo hace visible para demostración académica del
 * save/resume ante un stall costoso.
 *
 * Solo se guarda progreso a frontera de boid completo. No incluye
 * acumuladores parciales de vecinos ni fuerzas ya escritas en steeringForces[].
 */
struct CoarseWorkerCheckpoint {
    int workerId = 0;          /**< Identificación del worker para logs/demo. */
    int startIndex = 0;        /**< Inicio inclusivo del bloque asignado. */
    int endIndex = 0;          /**< Fin exclusivo del bloque asignado. */
    int nextBoidIndex = 0;     /**< Siguiente boid a calcular (imprescindible). */
    int boidsCompleted = 0;    /**< Boids ya terminados en este bloque. */
    int stallCount = 0;        /**< Stalls didácticos ya ejecutados. */
    std::uint64_t rngState = 0; /**< Estado del RNG de la política de stall. */

    /**
     * @brief Crea una copia inmutable del estado actual (save didáctico).
     * @return Snapshot listo para restaurar tras el bloqueo costoso.
     */
    CoarseWorkerCheckpoint snapshot() const;

    /**
     * @brief Restaura este checkpoint sobre el estado vivo del worker.
     * @param target Referencia al contexto del worker a reanudar.
     * @note Tras restore, el worker continúa en nextBoidIndex sin recalcular
     *       índices ya escritos en steeringForces[].
     */
    void restoreInto(CoarseWorkerCheckpoint& target) const;

    /**
     * @brief Representación textual corta para traza/demo.
     * @return Cadena con workerId, rango y nextBoidIndex.
     */
    std::string toString() const;
};

#endif // COARSE_WORKER_CHECKPOINT_HPP

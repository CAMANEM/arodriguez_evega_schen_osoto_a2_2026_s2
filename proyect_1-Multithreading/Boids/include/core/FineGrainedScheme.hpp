#ifndef FINE_GRAINED_SCHEME_HPP
#define FINE_GRAINED_SCHEME_HPP

#include <vector>

#include "core/FlockingScheme.hpp"
#include "core/SteeringContext.hpp"

/**
 * @brief Simulación cooperativa de Fine-Grained Multithreading.
 *
 * El enunciado exige planificación por cuantums en round-robin, no hilos
 * del sistema operativo: un único hilo del SO ejecuta N contextos virtuales
 * (uno por boid procesado). El quantum mínimo es examinar un vecino
 * candidato; el scheduler cede el turno aunque el contexto actual aún
 * tenga trabajo pendiente.
 *
 * Esto mide overhead y comportamiento del modelo, no speedup esperado.
 * --partial N limita los contextos para demos; 0 (default) usa el flock
 * completo, que es el modo de campaña experimental.
 */
class FineGrainedScheme : public FlockingScheme {
public:
    /**
     * @brief Construye el esquema de grano fino.
     * @param partialBoidCount Máximo de contextos virtuales. Si es <= 0,
     *        se procesa el enjambre completo.
     */
    explicit FineGrainedScheme(int partialBoidCount = 0);

    /**
     * @brief Un paso: round-robin de cuantums, luego integración conjunta.
     * @param flock Enjambre a actualizar (solo se escribe al final).
     * @param config Configuración de flocking.
     * @return Métricas con contextos virtuales, tiempo y flag de parcialidad.
     */
    BoidsMetrics simulateStep(Flock& flock, const FlockingConfig& config) override;

    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

    /**
     * @brief Resuelve cuántos contextos virtuales crear.
     * @param flockSize Cantidad de boids del enjambre.
     * @param partialBoidCount Límite opcional; <= 0 significa "todos".
     * @return min(partial, flockSize), o flockSize si partial <= 0.
     */
    static int resolveContextCount(int flockSize, int partialBoidCount);

    /**
     * @brief Una ronda round-robin: un quantum a cada contexto activo.
     * @param contexts Contextos virtuales a planificar.
     * @return true si algún contexto sigue con trabajo pendiente.
     */
    static bool runOneRound(std::vector<SteeringContext>& contexts);

    /**
     * @brief Scheduler cooperativo hasta que todos los contextos terminen.
     * @param contexts Contextos virtuales a planificar.
     */
    static void runRoundRobinUntilDone(std::vector<SteeringContext>& contexts);

private:
    int partialBoidCount_;

    /**
     * @brief Crea un contexto virtual por cada boid a procesar (índices 0..n-1).
     * @param flock Enjambre de solo lectura.
     * @param config Configuración de flocking.
     * @param contextCount Cantidad de contextos a crear.
     * @return Vector de contextos listos para el scheduler.
     */
    static std::vector<SteeringContext> createContexts(const Flock& flock,
                                                       const FlockingConfig& config,
                                                       int contextCount);

    /**
     * @brief Aplica las fuerzas finales de cada contexto al enjambre.
     * @param flock Enjambre a mutar.
     * @param config Configuración de integración.
     * @param contexts Contextos ya terminados.
     */
    static void applyIntegrations(Flock& flock, const FlockingConfig& config,
                                  const std::vector<SteeringContext>& contexts);
};

#endif // FINE_GRAINED_SCHEME_HPP

#ifndef FINE_GRAINED_SCHEME_HPP
#define FINE_GRAINED_SCHEME_HPP

#include "core/FlockingScheme.hpp"

/**
 * @brief Simulación por software del multihilo de grano fino
 *        (Fine-Grained Multithreading).
 *
 * Según el enunciado del proyecto, este modelo "debe implementarse mediante
 * un modelo de planificación cooperativa o simulación de ejecución por
 * ciclos o cuantums, donde el cambio de hilo ocurre en cada ciclo (quantum
 * mínimo) en forma round-robin, independientemente de si el hilo actual ha
 * sufrido un stall".
 *
 * En este problema, la unidad de trabajo natural para un "ciclo" es
 * examinar un único vecino candidato: cada contexto virtual
 * (SteeringContext) recibe un turno de un candidato a la vez, en
 * round-robin, sin importar si a otros contextos les queden muchos más
 * candidatos por examinar (su "stall").
 *
 * Nota (Demostración 2): esta es una ejecución "dummy"/trivial sobre una
 * porción parcial del problema (un subconjunto de boids), tal como lo
 * permite el enunciado para esta etapa. No se usa para la animación
 * visual completa (ver CoarseGrainedScheme y CmpScheme), solo
 * para demostrar y medir el modelo de planificación en sí.
 *
 * 
 */
class FineGrainedScheme : public FlockingScheme {
public:
    /**
     * @brief Construye el esquema de grano fino.
     * @param partialBoidCount Cantidad de boids ("hilos virtuales") que se
     *        actualizan de forma concurrente en round-robin durante esta
     *        demostración parcial.
     */
    explicit FineGrainedScheme(int partialBoidCount = 20);

    BoidsMetrics simulateStep(Flock& flock, const FlockingConfig& config) override;
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;

private:
    int partialBoidCount_;
};

#endif // FINE_GRAINED_SCHEME_HPP

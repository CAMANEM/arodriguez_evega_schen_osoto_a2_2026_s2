#ifndef FLOCKING_SCHEME_HPP
#define FLOCKING_SCHEME_HPP

#include <string>

#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/BoidsMetrics.hpp"

/**
 * @brief Interfaz común para todos los modelos de ejecución evaluados en el
 *        framework experimental (secuencial, grano fino, grano grueso,
 *        SMT y CMP).
 *
 * Aplica el patrón de diseño Strategy: el código cliente solo
 * conoce esta interfaz y puede intercambiar el esquema de ejecución sin
 * modificar el resto del programa, cumpliendo con el principio de
 * abierto/cerrado (Open/Closed Principle).
 *
 * 
 */
class FlockingScheme {
public:
    virtual ~FlockingScheme() = default;

    /**
     * @brief Ejecuta un único paso de la simulación: calcula la fuerza de
     *        dirección de cada boid y actualiza su posición/velocidad.
     * @param flock Enjambre a actualizar.
     * @param config Configuración de la simulación.
     * @return Métricas preliminares de la ejecución (tiempo, hilos usados,
     *         boids procesados).
     */
    virtual BoidsMetrics simulateStep(Flock& flock, const FlockingConfig& config) = 0;

    /** @return Nombre descriptivo del esquema, usado para reportes. */
    virtual std::string getSchemeName() const = 0;

    /** @return Modelo común asociado con este esquema. */
    virtual execution_model getExecutionModel() const = 0;
};

#endif // FLOCKING_SCHEME_HPP

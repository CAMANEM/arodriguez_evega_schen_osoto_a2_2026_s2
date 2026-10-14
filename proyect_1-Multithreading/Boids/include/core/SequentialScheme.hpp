#ifndef SEQUENTIAL_SCHEME_HPP
#define SEQUENTIAL_SCHEME_HPP

#include "core/FlockingScheme.hpp"

/**
 * @brief Implementación secuencial (sin hilos) de un paso de simulación
 *        del enjambre.
 *
 * Corresponde al "sistema base" exigido explícitamente en la Demostración 2:
 * debe ejecutarse primero y sin paralelismo, ya que es el baseline contra
 * el cual se calculará el speedup de los demás esquemas, y también sirve
 * como referencia de correctitud para validar los resultados paralelos.
 *
 * Nota de diseño: el cálculo se hace en dos fases (primero se calculan
 * TODAS las fuerzas de dirección leyendo el estado actual del enjambre, y
 * luego se aplican TODAS las integraciones) para que el orden en que se
 * procesan los boids no afecte el resultado — un boid no debe "ver" la
 * posición ya actualizada de otro boid dentro del mismo paso. Esta misma
 * estructura de dos fases es la que permite paralelizar el problema sin
 * condiciones de carrera en ThreadedScheme.
 *
 * 
 */
class SequentialScheme : public FlockingScheme {
public:
    BoidsMetrics simulateStep(Flock& flock, const FlockingConfig& config) override;
    std::string getSchemeName() const override;
    execution_model getExecutionModel() const override;
};

#endif // SEQUENTIAL_SCHEME_HPP

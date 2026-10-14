#ifndef FLOCKING_RULES_HPP
#define FLOCKING_RULES_HPP

#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/Vector2D.hpp"

/**
 * @brief Contiene la lógica matemática pura de las tres reglas clásicas de
 *        Craig Reynolds (separación, alineación y cohesión), reutilizable
 *        por cualquier esquema de ejecución (secuencial o multihilo).
 *
 * Separar las reglas de comportamiento de la política de paralelización
 * garantiza que las diferencias de tiempo medidas entre esquemas provengan
 * únicamente del modelo de concurrencia y no de variaciones en la física
 * del enjambre.
 *
 * 
 */
class FlockingRules {
public:
    /**
     * @brief Calcula la fuerza de dirección completa de un boid,
     *        recorriendo de una sola vez a todos los demás boids del
     *        enjambre (barrido "de corrido", usado por los esquemas
     *        secuencial y de hilos reales).
     * @param boidIndex Índice del boid para el que se calcula la fuerza.
     * @param flock Enjambre completo (de solo lectura durante el cálculo).
     * @param config Configuración de la simulación.
     * @return Fuerza de dirección combinada, ya limitada a maxForce.
     */
    static Vector2D computeSteeringForBoid(int boidIndex, const Flock& flock,
                                            const FlockingConfig& config);

    /**
     * @brief Combina las sumas parciales acumuladas de vecinos (separación,
     *        suma de velocidades, suma de posiciones y cantidad de
     *        vecinos) en la fuerza de dirección final, aplicando los pesos
     *        configurados y el límite de fuerza máxima.
     *
     * Se extrajo esta combinación final como un método independiente para
     * que tanto el barrido directo (computeSteeringForBoid) como el
     * cálculo incremental por ciclos (ver SteeringContext, usado en el
     * esquema de grano fino) compartan exactamente la misma fórmula de
     * combinación, evitando duplicar lógica.
     * @param selfPosition Posición del boid propio.
     * @param selfVelocity Velocidad del boid propio.
     * @param separationSum Suma acumulada de vectores de repulsión.
     * @param velocitySum Suma acumulada de velocidades de los vecinos.
     * @param positionSum Suma acumulada de posiciones de los vecinos.
     * @param neighborCount Cantidad de vecinos dentro del radio de
     *        percepción.
     * @param config Configuración de la simulación.
     * @return Fuerza de dirección combinada, ya limitada a maxForce.
     */
    static Vector2D combineForces(const Vector2D& selfPosition, const Vector2D& selfVelocity,
                                   const Vector2D& separationSum, const Vector2D& velocitySum,
                                   const Vector2D& positionSum, int neighborCount,
                                   const FlockingConfig& config);
};

#endif // FLOCKING_RULES_HPP

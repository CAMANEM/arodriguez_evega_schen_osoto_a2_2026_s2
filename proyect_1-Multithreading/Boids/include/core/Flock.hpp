#ifndef FLOCK_HPP
#define FLOCK_HPP

#include <vector>

#include "core/Boid.hpp"
#include "core/FlockingConfig.hpp"
#include "core/Vector2D.hpp"

/**
 * @brief Contenedor del conjunto completo de boids que forman el enjambre.
 *
 * Se diseñó como una clase independiente (y no como un simple
 * std::vector<Boid> en el código cliente) para que la generación aleatoria
 * inicial, el acceso indexado y la aplicación de la integración física
 * tengan un único punto de responsabilidad, reutilizable por todos los
 * esquemas de ejecución.
 *
 * 
 */
class Flock {
public:
    /**
     * @brief Genera un enjambre con boids distribuidos aleatoriamente en
     *        el mundo, con velocidades iniciales aleatorias.
     * @param config Configuración de la simulación (cantidad de boids,
     *        límites del mundo, rapidez máxima).
     * @param seed Semilla del generador aleatorio, para reproducibilidad.
     */
    Flock(const FlockingConfig& config, unsigned int seed = 42);

    /** @return Cantidad de boids en el enjambre. */
    int getBoidCount() const;

    /**
     * @brief Obtiene un boid por índice (lectura).
     * @param index Índice del boid (0 <= index < getBoidCount()).
     * @return Referencia constante al boid solicitado.
     */
    const Boid& getBoid(int index) const;

    /**
     * @brief Aplica la integración física de un boid dado, usando una
     *        fuerza de dirección ya calculada por alguno de los esquemas
     *        de ejecución.
     * @param index Índice del boid a actualizar.
     * @param steeringForce Fuerza de dirección a aplicar.
     * @param config Configuración de la simulación.
     */
    void applyIntegration(int index, const Vector2D& steeringForce, const FlockingConfig& config);

private:
    std::vector<Boid> boids_;
};

#endif // FLOCK_HPP

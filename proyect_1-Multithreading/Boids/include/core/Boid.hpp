#ifndef BOID_HPP
#define BOID_HPP

#include "core/FlockingConfig.hpp"
#include "core/Vector2D.hpp"
#include "object_interface.hpp"

/**
 * @brief Representa un único agente (boid) del enjambre: su posición y
 *        velocidad actuales, y la lógica para integrar una fuerza de
 *        dirección en un nuevo estado físico.
 *
 * El boid no sabe nada sobre sus vecinos ni sobre cómo se calculan las
 * reglas de comportamiento (eso vive en FlockingRules); solo sabe
 * "moverse" dada una fuerza ya calculada. Esta separación de
 * responsabilidades permite que el mismo Boid sea reutilizado sin cambios
 * por todos los esquemas de ejecución.
 *
 * 
 */
class Boid : public object_interface {
public:
    /**
     * @brief Crea un boid con una posición y velocidad iniciales.
     * @param id Identificador estable dentro del enjambre.
     * @param position Posición inicial en el mundo.
     * @param velocity Velocidad inicial.
     * @param mass Masa utilizada por el contrato físico compartido.
     */
    Boid(int id, const Vector2D& position, const Vector2D& velocity, double mass = 1.0);

    /** @return Posición actual del boid. */
    Vector2D getPosition() const;

    /** @return Velocidad actual del boid. */
    Vector2D getVelocity() const;

    /**
     * @brief Integra fuerza, velocidad y posición con Euler semiimplícito.
     * @param dt Duración positiva del paso; los límites del mundo no se
     *        aplican en este método genérico.
     */
    void update(double dt) override;

    /**
     * @brief Restablece posición, velocidad, aceleración y fuerza a cero.
     */
    void reset() override;

    /**
     * @brief Aplica una fuerza de dirección ya calculada: actualiza la
     *        velocidad (limitada a maxSpeed), integra la posición, y
     *        envuelve (wrap-around) la posición dentro de los límites del
     *        mundo para que el enjambre nunca "desaparezca" por un borde.
     * @param steeringForce Fuerza de dirección resultante de combinar
     *        separación, alineación y cohesión (ya limitada a maxForce).
     * @param config Configuración de la simulación (límites del mundo,
     *        rapidez máxima, paso de tiempo).
     */
    void integrate(const Vector2D& steeringForce, const FlockingConfig& config);
};

#endif // BOID_HPP

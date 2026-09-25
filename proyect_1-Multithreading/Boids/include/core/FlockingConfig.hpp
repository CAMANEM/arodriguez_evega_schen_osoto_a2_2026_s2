#ifndef FLOCKING_CONFIG_HPP
#define FLOCKING_CONFIG_HPP

/**
 * @brief Encapsula los parámetros que gobiernan el comportamiento del
 *        enjambre (parvada) y el mundo donde se simula.
 *
 * Estas son las variables críticas identificadas para la paralelización:
 * el radio de percepción determina cuántos vecinos evalúa cada boid (y por
 * lo tanto cuánto trabajo real hace), mientras que la cantidad de boids
 * determina el volumen total de pares a evaluar en el peor caso.
 *
 * 
 */
class FlockingConfig {
public:
    /**
     * @brief Construye la configuración de la simulación.
     * @param boidCount Cantidad de agentes (boids) en el enjambre.
     * @param worldWidth Ancho del mundo (también usado como ancho del
     *        lienzo de exportación, para simplificar el mapeo).
     * @param worldHeight Alto del mundo.
     * @param perceptionRadius Radio dentro del cual un boid considera a
     *        otro como vecino para alineación y cohesión.
     * @param separationRadius Radio (más pequeño que perceptionRadius)
     *        dentro del cual se aplica la fuerza de separación.
     * @param maxSpeed Rapidez máxima permitida para un boid.
     * @param maxForce Magnitud máxima de la fuerza de dirección aplicada
     *        por paso de simulación.
     * @param separationWeight Peso de la regla de separación.
     * @param alignmentWeight Peso de la regla de alineación.
     * @param cohesionWeight Peso de la regla de cohesión.
     * @param deltaTime Paso de tiempo de integración por actualización.
     */
    FlockingConfig(int boidCount, double worldWidth, double worldHeight,
                   double perceptionRadius, double separationRadius,
                   double maxSpeed, double maxForce,
                   double separationWeight, double alignmentWeight, double cohesionWeight,
                   double deltaTime);

    /** @return Cantidad de boids configurada. */
    int getBoidCount() const;
    /** @return Ancho del mundo simulado. */
    double getWorldWidth() const;
    /** @return Alto del mundo simulado. */
    double getWorldHeight() const;
    /** @return Radio usado para alineación y cohesión. */
    double getPerceptionRadius() const;
    /** @return Radio usado para separación. */
    double getSeparationRadius() const;
    /** @return Rapidez máxima de un boid. */
    double getMaxSpeed() const;
    /** @return Magnitud máxima de la dirección aplicada. */
    double getMaxForce() const;
    /** @return Peso de separación. */
    double getSeparationWeight() const;
    /** @return Peso de alineación. */
    double getAlignmentWeight() const;
    /** @return Peso de cohesión. */
    double getCohesionWeight() const;
    /** @return Duración de un paso de integración. */
    double getDeltaTime() const;

private:
    int boidCount_;
    double worldWidth_;
    double worldHeight_;
    double perceptionRadius_;
    double separationRadius_;
    double maxSpeed_;
    double maxForce_;
    double separationWeight_;
    double alignmentWeight_;
    double cohesionWeight_;
    double deltaTime_;
};

#endif // FLOCKING_CONFIG_HPP

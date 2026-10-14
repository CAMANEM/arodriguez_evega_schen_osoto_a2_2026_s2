#ifndef STEERING_CONTEXT_HPP
#define STEERING_CONTEXT_HPP

#include "core/Flock.hpp"
#include "core/FlockingConfig.hpp"
#include "core/Vector2D.hpp"

/**
 * @brief Contexto virtual (hilo lógico) que calcula el steering de un boid
 *        evaluando exactamente un vecino candidato por quantum.
 *
 * Existe para simular Fine-Grained Multithreading: el sistema operativo no
 * cambia de contexto por ciclo, así que el scheduler cooperativo de
 * FineGrainedScheme llama a stepOnce() y cede el turno aunque quede
 * trabajo pendiente (el "stall" lógico de este modelo).
 */
class SteeringContext {
public:
    /**
     * @brief Crea un contexto de ejecución para un boid específico.
     * @param boidIndex Índice del boid cuya fuerza de dirección calculará
     *        este contexto.
     * @param flock Enjambre completo (solo lectura durante el cálculo).
     * @param config Configuración de la simulación.
     */
    SteeringContext(int boidIndex, const Flock& flock, const FlockingConfig& config);

    /**
     * @brief Ejecuta un único quantum: examina el siguiente candidato y
     *        acumula su contribución si está dentro del radio de
     *        percepción (y de separación, si aplica).
     * @return true si el contexto sigue activo (le quedan candidatos),
     *         false si ya examinó a todos los demás boids.
     */
    bool stepOnce();

    /** @return true si el contexto ya terminó de examinar candidatos. */
    bool isFinished() const;

    /**
     * @brief Indica si queda trabajo pendiente (stall lógico del modelo).
     * @return true cuando isFinished() es false.
     */
    bool hasPendingWork() const;

    /**
     * @brief Combina las sumas parciales acumuladas durante el round-robin
     *        en la fuerza de dirección final del boid.
     * @return Fuerza de dirección combinada, ya limitada a maxForce.
     */
    Vector2D computeFinalSteering() const;

    /** @return Índice del boid asociado a este contexto. */
    int getBoidIndex() const;

    /**
     * @brief Cursor del próximo candidato a examinar (índice en el flock).
     * @return Índice del siguiente boid candidato, o getBoidCount() si ya
     *         no quedan candidatos.
     */
    int getCandidateCursor() const;

    /**
     * @brief Cuantums ejecutados por este contexto (cada uno = 1 candidato).
     * @return Cantidad de llamadas a stepOnce() que examinaron un candidato.
     */
    int getQuantumsExecuted() const;

private:
    int boidIndex_;
    const Flock* flock_;
    const FlockingConfig* config_;
    int candidateCursor_;
    int neighborCount_;
    int quantumsExecuted_;
    Vector2D separationSum_;
    Vector2D velocitySum_;
    Vector2D positionSum_;
    bool finished_;

    /**
     * @brief Acumula alineación, cohesión y separación del candidato actual
     *        si está dentro de los radios configurados.
     */
    void accumulateCurrentCandidate();

    /**
     * @brief Avanza el cursor un índice y salta el boid propio.
     */
    void advanceToNextCandidate();

    /**
     * @brief Avanza candidateCursor_ si apunta al propio boid y marca el
     *        contexto como terminado cuando no quedan candidatos.
     */
    void skipSelfIndex();
};

#endif // STEERING_CONTEXT_HPP

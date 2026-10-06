#ifndef STALL_POLICY_HPP
#define STALL_POLICY_HPP

#include <chrono>
#include <cstdint>
#include <random>

/**
 * @brief Política configurable de stalls didácticos para Coarse-Grained.
 *
 * Decide cuándo inyectar un bloqueo costoso tras terminar un boid completo
 * y cuánto debe durar la espera. No altera el cálculo de fuerzas:
 * solo el timing/scheduling, para evidenciar cambios de contexto ante stalls.
 *
 * Precedencia de disparo tras cada boid completado:
 * - Si @c stallEveryBoids > 0 y @c boidsCompleted es múltiplo de K → stall.
 * - O si @c stallProbability > 0 y un sorteo uniforme cae bajo P → stall.
 * Ambos modos pueden estar activos a la vez (OR). Si K=0 y P=0, no hay stalls.
 *
 * Precedencia de duración:
 * - Si el rango [minMs, maxMs] es válido (ambos >= 0 y min <= max) → uniforme.
 * - En caso contrario → duración fija @c stallMilliseconds.
 */
class StallPolicy {
public:
    using ClockDuration = std::chrono::duration<double, std::milli>;

    /**
     * @brief Construye la política de stall.
     * @param stallEveryBoids Stall cada K boids completados (0 = desactivado).
     * @param stallProbability Probabilidad [0,1] de stall tras cada boid.
     * @param stallMilliseconds Duración fija en ms cuando no hay rango.
     * @param stallMillisecondsMin Extremo inferior del rango aleatorio (-1 = off).
     * @param stallMillisecondsMax Extremo superior del rango aleatorio (-1 = off).
     * @param seed Semilla del generador (reproducibilidad con --seed).
     */
    StallPolicy(int stallEveryBoids = 0,
                double stallProbability = 0.0,
                double stallMilliseconds = 1.0,
                double stallMillisecondsMin = -1.0,
                double stallMillisecondsMax = -1.0,
                std::uint32_t seed = 42);

    /**
     * @brief Indica si corresponde un stall tras completar un boid.
     * @param boidsCompleted Cantidad de boids ya terminados por este worker.
     * @return true si la política dispara un bloqueo costoso.
     */
    bool shouldStallAfterBoid(int boidsCompleted);

    /**
     * @brief Calcula la duración del próximo stall.
     * @return Duración en milisegundos (fija o uniforme en el rango).
     */
    ClockDuration nextStallDuration();

    /** @return true si hay alguna forma de stall activa. */
    bool isEnabled() const;

    /** @return Stall cada K boids (0 = desactivado). */
    int getStallEveryBoids() const { return stallEveryBoids_; }

    /** @return Probabilidad de stall tras cada boid. */
    double getStallProbability() const { return stallProbability_; }

    /** @return Duración fija configurada (ms). */
    double getStallMilliseconds() const { return stallMilliseconds_; }

    /** @return Mínimo del rango aleatorio, o < 0 si no aplica. */
    double getStallMillisecondsMin() const { return stallMillisecondsMin_; }

    /** @return Máximo del rango aleatorio, o < 0 si no aplica. */
    double getStallMillisecondsMax() const { return stallMillisecondsMax_; }

    /**
     * @brief Serializa el estado del RNG para el checkpoint didáctico.
     * @return Entero que representa el estado interno del generador.
     * @note Permite restaurar la secuencia aleatoria tras un stall manual.
     */
    std::uint64_t captureRngState() const;

    /**
     * @brief Restaura el estado del RNG desde un checkpoint.
     * @param state Valor previamente obtenido con captureRngState().
     */
    void restoreRngState(std::uint64_t state);

    /**
     * @brief Valida y normaliza parámetros de stall desde CLI/config.
     * @param stallEveryBoids Referencia a K (se clampea a >= 0).
     * @param stallProbability Referencia a P (se clampea a [0,1]).
     * @param stallMilliseconds Referencia a duración fija (se clampea a >= 0).
     * @param stallMillisecondsMin Referencia a mínimo del rango.
     * @param stallMillisecondsMax Referencia a máximo del rango.
     * @throws std::runtime_error si el rango min/max es inconsistente.
     */
    static void validateAndClamp(int& stallEveryBoids,
                                 double& stallProbability,
                                 double& stallMilliseconds,
                                 double& stallMillisecondsMin,
                                 double& stallMillisecondsMax);

private:
    int stallEveryBoids_;
    double stallProbability_;
    double stallMilliseconds_;
    double stallMillisecondsMin_;
    double stallMillisecondsMax_;
    std::mt19937 rng_;
    bool useRandomDuration_;
    std::uint32_t seed_;
    std::uint64_t advanceCount_;

    bool hasRandomDuration() const;
    double drawUnitInterval();
};

#endif // STALL_POLICY_HPP

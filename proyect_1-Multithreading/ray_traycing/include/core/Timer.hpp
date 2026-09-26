/**
 * @file Timer.hpp
 * @brief Cronómetro de pared para medir una operación en milisegundos.
 */
#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

/** @brief Mide el intervalo transcurrido entre start y stopAndGetMilliseconds. */
class Timer {
public:
    /** @brief Guarda el instante inicial de la medición. */
    void start();
    /**
     * @brief Detiene conceptualmente la medición y devuelve el tiempo transcurrido.
     * @return Duración desde la última llamada a start(), en milisegundos.
    * @pre Debe llamarse start() antes de consultar el intervalo.
     */
    double stopAndGetMilliseconds();

private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

#endif // TIMER_HPP
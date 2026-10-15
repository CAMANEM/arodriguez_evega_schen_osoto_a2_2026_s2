#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

/**
 * @brief Utilidad sencilla para medir tiempos de ejecución en milisegundos.
 *
 * Se implementó como una clase separada para que todos los esquemas de
 * ejecución midan el tiempo de la misma forma, evitando inconsistencias al
 * comparar los resultados preliminares pedidos en la Demostración 2.
 *
 * 
 */
class Timer {
public:
    /** @brief Marca el instante de inicio de la medición. */
    void start();

    /**
     * @brief Detiene la medición y calcula el tiempo transcurrido.
     * @return Milisegundos transcurridos desde la última llamada a start().
     */
    double stopAndGetMilliseconds();

private:
    std::chrono::high_resolution_clock::time_point startTime_;
};

#endif // TIMER_HPP

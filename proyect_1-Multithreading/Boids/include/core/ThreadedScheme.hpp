#ifndef THREADED_SCHEME_HPP
#define THREADED_SCHEME_HPP

#include "core/FlockingScheme.hpp"

#include <vector>

#include "core/Vector2D.hpp"

/**
 * @brief Clase base para los esquemas que se implementan mediante hilos
 *        reales del sistema operativo (std::thread).
 *
 * Aplica el patrón de diseño Template Method: el método simulateStep()
 * define el algoritmo común (particionar los boids en bloques, lanzar un
 * hilo por bloque que calcula las fuerzas de dirección de su rango —
 * leyendo el enjambre completo pero escribiendo solo en su porción
 * disjunta del vector de fuerzas—, esperar su finalización mediante join
 * —el "evento de bloqueo costoso" del grano grueso— y luego aplicar todas
 * las integraciones). Las subclases solo definen la política de cuántos
 * hilos usar mediante computeThreadCount(), evitando duplicar la lógica de
 * partición y medición entre Coarse-Grained, SMT y CMP.
 *
 * 
 */
class ThreadedScheme : public FlockingScheme {
public:
    BoidsMetrics simulateStep(Flock& flock, const FlockingConfig& config) final;

protected:
    /**
     * @brief Determina cuántos hilos de sistema operativo se deben lanzar.
     *
     * Cada subclase concreta define aquí la política de hardware que
     * representa (grano grueso genérico, sobre-suscripción tipo SMT, o
     * mapeo a procesadores lógicos disponibles para aproximar CMP).
     * @param flock Enjambre a procesar, por si la política depende del
     *        tamaño del problema.
     * @return Cantidad de hilos a utilizar (siempre >= 1).
     */
    virtual unsigned int computeThreadCount(const Flock& flock) const = 0;

private:
    /**
     * @brief Calcula las fuerzas de dirección del rango de boids
     *        [startIndex, endIndex) y las escribe en su porción disjunta
     *        del vector de fuerzas compartido.
     * @param flock Enjambre completo (de solo lectura durante el cálculo).
     * @param config Configuración de la simulación.
     * @param startIndex Primer índice (inclusivo) asignado a este hilo.
     * @param endIndex Último índice (exclusivo) asignado a este hilo.
     * @param steeringForces Vector de salida compartido (cada hilo escribe
     *        en índices disjuntos, por lo que no requiere sincronización).
     */
    void computeForceBlock(const Flock& flock, const FlockingConfig& config,
                            int startIndex, int endIndex,
                            std::vector<Vector2D>& steeringForces) const;
};

#endif // THREADED_SCHEME_HPP

#ifndef BOIDS_METRICS_HPP
#define BOIDS_METRICS_HPP

#include <string>

#include "metrics_interface.hpp"

/**
 * @brief Métricas de un paso de Boids alineadas con la interfaz compartida.
 *
 * El tiempo se almacena en segundos en metrics_interface. Los métodos de
 * conveniencia de esta clase exponen milisegundos para la tabla preliminar
 * de la Demostración 2.
 */
class BoidsMetrics : public metrics_interface {
public:
    /**
     * @brief Construye el resultado de una ejecución de un esquema.
     * @param model Modelo de ejecución medido.
     * @param schemeName Nombre legible del esquema.
     * @param workers Hilos del SO o contextos virtuales utilizados.
     * @param elapsedMilliseconds Tiempo de pared del paso.
     * @param boidsProcessed Cantidad de boids actualizados.
     * @param virtualWorkers Indica si workers representa contextos simulados.
     */
    BoidsMetrics(execution_model model, const std::string& schemeName, int workers,
                 double elapsedMilliseconds, int boidsProcessed,
                 bool virtualWorkers = false)
        : metrics_interface(model, workers),
          schemeName_(schemeName),
          boidsProcessed_(boidsProcessed),
          virtualWorkers_(virtualWorkers) {
        record_time(elapsedMilliseconds / 1000.0);
    }

    /** @return Nombre legible del esquema medido. */
    const std::string& get_scheme_name() const { return schemeName_; }

    /** @return Tiempo medio registrado, expresado en milisegundos. */
    double elapsed_milliseconds() const { return mean_time() * 1000.0; }

    /** @return Cantidad de boids actualizados en esta ejecución. */
    int get_boids_processed() const { return boidsProcessed_; }

    /** @return true cuando el conteo representa contextos virtuales. */
    bool uses_virtual_workers() const { return virtualWorkers_; }

private:
    std::string schemeName_;
    int boidsProcessed_;
    bool virtualWorkers_;
};

#endif // BOIDS_METRICS_HPP

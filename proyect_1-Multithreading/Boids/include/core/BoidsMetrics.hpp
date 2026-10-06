#ifndef BOIDS_METRICS_HPP
#define BOIDS_METRICS_HPP

#include <algorithm>
#include <string>

#include "metrics_interface.hpp"

/**
 * @brief Métricas de un paso de Boids alineadas con la interfaz compartida.
 *
 * El tiempo se almacena en segundos en metrics_interface. Los métodos de
 * conveniencia de esta clase exponen milisegundos para la tabla preliminar
 * de la Demostración 2. Los campos de stall son opcionales (Coarse-Grained);
 * el resto de esquemas los dejan en cero.
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
     * @param isPartial true si no se actualizó el enjambre completo.
     * @param stallCount Cantidad de stalls didácticos inyectados (coarse).
     * @param stallTimeMs Tiempo acumulado en stalls (ms).
     * @param computeTimeMs Tiempo estimado de cómputo (elapsed - stall), ms.
     */
    BoidsMetrics(execution_model model, const std::string& schemeName, int workers,
                 double elapsedMilliseconds, int boidsProcessed,
                 bool virtualWorkers = false, bool isPartial = false,
                 int stallCount = 0, double stallTimeMs = 0.0,
                 double computeTimeMs = -1.0)
        : metrics_interface(model, workers),
          schemeName_(schemeName),
          boidsProcessed_(boidsProcessed),
          virtualWorkers_(virtualWorkers),
          isPartial_(isPartial),
          stallCount_(stallCount),
          stallTimeMs_(stallTimeMs),
          computeTimeMs_(computeTimeMs < 0.0
                             ? std::max(0.0, elapsedMilliseconds - stallTimeMs)
                             : computeTimeMs) {
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

    /** @return true si esta ejecución no actualizó todos los boids. */
    bool is_partial() const { return isPartial_; }

    /** @return Stalls didácticos inyectados en este paso (0 si no aplica). */
    int get_stall_count() const { return stallCount_; }

    /** @return Tiempo acumulado en stalls, en milisegundos. */
    double get_stall_time_ms() const { return stallTimeMs_; }

    /** @return Tiempo estimado de cómputo (elapsed - stall), en milisegundos. */
    double get_compute_time_ms() const { return computeTimeMs_; }

private:
    std::string schemeName_;
    int boidsProcessed_;
    bool virtualWorkers_;
    bool isPartial_;
    int stallCount_;
    double stallTimeMs_;
    double computeTimeMs_;
};

#endif // BOIDS_METRICS_HPP

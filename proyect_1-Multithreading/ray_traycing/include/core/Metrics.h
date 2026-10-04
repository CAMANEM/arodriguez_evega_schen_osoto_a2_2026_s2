/**
 * @file Metrics.h
 * @brief Estructuras con métricas virtuales y estadísticas por worker.
 */
#ifndef METRICS_H
#define METRICS_H

#include <vector>

namespace trace {

/** @brief Contadores de un hilo o contexto durante el último frame. */
struct ThreadMetrics {
    int thread_id = 0;                  /**< Índice del worker. */
    long long nops_count = 0;           /**< NOPs simulados ejecutados. */
    double nop_time_ns = 0.0;           /**< Tiempo acumulado en NOPs, en ns. */
    int cache_misses = 0;               /**< Misses de caché observados. */
    long long virtual_time_ns = 0LL;    /**< Tiempo virtual acumulado, en ns. */
};

/** @brief Resumen global de una campaña de renderizado. */
struct Metrics {
    int runs = 0;                   /**< Cantidad de ejecuciones. */
    double total = 0.0;             /**< Suma de tiempos de pared. */
    double avg = 0.0;               /**< Tiempo de pared promedio. */
    double min = 0.0;               /**< Menor tiempo de pared. */
    double max = 0.0;               /**< Mayor tiempo de pared. */
    double stddev = 0.0;             /**< Desviación estándar de la muestra. */
    std::vector<double> times;       /**< Tiempos de pared por ejecución, en s. */

    long long virtual_time_ns = 0LL;       /**< Tiempo virtual promedio, en ns. */
    std::vector<long long> virtual_times;  /**< Tiempo virtual por ejecución, en ns. */

    std::vector<ThreadMetrics> thread_metrics; /**< Métricas por worker. */
    std::vector<int> stall_counts;              /**< Misses por ejecución. */
};

} // namespace trace

#endif // METRICS_H
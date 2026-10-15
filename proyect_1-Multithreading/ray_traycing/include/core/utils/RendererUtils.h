/**
 * @file RendererUtils.h
 * @brief Operaciones compartidas para reiniciar y sumar métricas de workers.
 */
#ifndef RENDERER_UTILS_H
#define RENDERER_UTILS_H

#include "core/utils/CacheModel.h"
#include "core/utils/Metrics.h"
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

namespace trace {

/**
 * @brief Valida una cantidad de workers que debe recibir trabajo no vacío.
 * @param workers Cantidad solicitada.
 * @param max_workers Límite impuesto por el número de unidades de trabajo.
 * @return La cantidad validada.
 * @throws std::invalid_argument Si la cantidad no está en el rango permitido.
 */
inline int validated_worker_count(int workers, int max_workers) {
    if (workers < 1 || workers > max_workers)
        throw std::invalid_argument(
            "workers must be between 1 and the total number of pixels");
    return workers;
}

/**
 * @brief Consume tiempo de pared mediante espera activa para un cache miss.
 * @param duration_ns Duración de la espera simulada en nanosegundos.
 */
inline void wait_for_stall_wall_time(long long duration_ns) {
    if (duration_ns <= 0) return;

    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::nanoseconds(duration_ns);
    while (std::chrono::steady_clock::now() < deadline) {
    }
}

// reset_thread_stats: Reinicia el estado de contadores y caches de todos
// los threads antes de iniciar un nuevo render_frame().
//
// Extrae la lógica de reset duplicada en FinegrainedRenderer, CoarseRenderer
// y CMPRenderer. Itera sobre el tamaño real del vector para ser agnóstico a
// la constante (NUM_THREADS vs CMP_NUM_CORES — ambas son 4, pero conceptualmente
// distintas).
// CoarseRenderer adicionalmente reinicia thread_done[] (scheduler-specific, fuera de aquí).
/**
 * @brief Reinicia contadores y cachés al comenzar un nuevo frame.
 * @param stats Métricas por worker que se ponen a cero.
 * @param caches Modelos de caché cuyo estado espacial se reinicia.
 * @note Ambos vectores deben tener el mismo tamaño.
 */
inline void reset_thread_stats(
    std::vector<ThreadMetrics>& stats,
    std::vector<CacheModel>&    caches)
{
    if (stats.size() != caches.size())
        throw std::invalid_argument("worker metrics and caches must have equal sizes");

    for (std::size_t i = 0; i < stats.size(); ++i) {
        caches[i].reset();
        stats[i].nops_count      = 0;
        stats[i].nop_time_ns     = 0.0;
        stats[i].cache_misses    = 0;
        stats[i].stall_time_ns   = 0LL;
        stats[i].context_switch_time_ns = 0LL;
        stats[i].context_switches = 0;
        stats[i].virtual_time_ns = 0LL;
    }
}

// sum_virtual_times: Suma el tiempo virtual de todos los threads.
//
// Extrae el bucle de acumulación duplicado en FinegrainedRenderer y CoarseRenderer.
// En ambos modelos, el VT total es la suma (no el máximo): los threads comparten
// el mismo pipeline y sus quanta se suman, no se solapan.
/**
 * @brief Suma tiempos virtuales de workers que comparten un pipeline.
 * @param stats Métricas por worker.
 * @return Tiempo virtual agregado, en nanosegundos.
 */
inline long long sum_virtual_times(const std::vector<ThreadMetrics>& stats) {
    long long total = 0LL;
    for (const auto& s : stats) total += s.virtual_time_ns;
    return total;
}

} // namespace trace

#endif // RENDERER_UTILS_H

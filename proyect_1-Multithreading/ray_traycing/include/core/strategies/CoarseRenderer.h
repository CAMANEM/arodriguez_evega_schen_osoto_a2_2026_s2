/**
 * @file CoarseRenderer.h
 * @brief Declaración del scheduler CGMT y el estado compartido de sus workers.
 */
#ifndef COARSE_RENDERER_H
#define COARSE_RENDERER_H

#include "core/strategies/IRenderer.h"
#include "core/utils/CacheModel.h"
#include "core/utils/Metrics.h"
#include "core/utils/SchedulerLogger.h"
#include <vector>
#include <mutex>
#include <condition_variable>

// CoarseRenderer: Renderizador CGMT (Coarse-Grained Multithreading).
//
// Scheduler: solo un thread ejecuta a la vez. La rotación ocurre
// ÚNICAMENTE cuando el thread activo detecta un stall (cache miss).
// Esto difiere de FGMT, donde la rotación es obligatoria cada ciclo.
//
// Mecanismo (idéntico al código de referencia en C):
//   - current_thread_ indica qué thread tiene el pipeline.
//   - En STALL: paga 400 ns de cambio de contexto y cede el slot.
//        → la latencia del miss (3200 ns) queda pendiente para ese worker.
//        → otro worker ejecuta después del costo de cambio.
//        → el pixel NO avanza y se reintenta cuando venza la latencia.
//   - En COMPUTE: se renderiza el pixel, se avanza, sin cambio de contexto.
//        → stats: +PIXEL_QUANTUM_NS
//   - Al terminar el tile: switch_to_next_thread() sin coste extra.
//
// switch_to_next_thread() está extraída como método privado (DRY + SRP)
// para evitar duplicar la búsqueda del siguiente thread activo.
/** @brief Simula CGMT con un slot y cambio de contexto al detectar stalls. */
class CoarseRenderer : public IRenderer {
private:
    std::vector<Vector3> frame;
    std::vector<CacheModel> cache_models;
    std::vector<trace::ThreadMetrics> thread_stats;
    
    struct Task {
        int start, end;
        int thread_id;
    };
    std::vector<Task> tasks;
    
    // Variables de scheduler CGMT
    const int worker_count;
    std::mutex sched_mutex;
    std::condition_variable sched_cv;
    int current_thread;                // Thread que tiene asignado el pipeline
    std::vector<bool> thread_done;     // true si el thread completó su tile
    int threads_finished;              // Conteo de threads terminados (bajo mutex)
    int global_clock_;                 // Ciclos totales de hardware simulados
    SchedulerLogger logger_;           // Traza ciclo-a-ciclo (activar con set_verbose)

    long long virtual_time_ns_ = 0LL;

    // switch_to_next_thread(): Scheduler hardware CGMT.
    // Busca el siguiente thread activo; si es el único, conserva el slot.
    // Debe llamarse mientras se sostiene sched_mutex.
    /** @brief Rota el slot al siguiente tile pendiente bajo sched_mutex. */
    void switch_to_next_thread();
    /** @param thread_id Identificador del worker CGMT que espera o ejecuta. */
    void render_worker(int thread_id);

public:
    /** @param workers Cantidad de workers CGMT, entre 1 y la cantidad de píxeles. */
    explicit CoarseRenderer(int workers = constants::NUM_THREADS);

    // Habilita la traza del scheduler para los primeros `cycles` ciclos de pipeline.
    /** @param cycles Cantidad de ciclos iniciales que se registran. */
    void set_verbose(int cycles) override { logger_.set_max_cycles(cycles); }

    /** @return Frame completo después de que todos los workers terminen. */
    std::vector<Vector3> render_frame() override;
    /** @return Identificador `cgmt`. */
    std::string get_model_name() const override { return "cgmt"; }
    /** @return Contadores por worker del último frame. */
    const std::vector<trace::ThreadMetrics>& get_thread_metrics() const override {
        return thread_stats;
    }
    /** @return Suma del tiempo virtual de los workers, en nanosegundos. */
    long long get_virtual_time_ns() const override { return virtual_time_ns_; }
    /** @return Total de misses registrados por los workers. */
    int get_total_stalls() const override {
        int total = 0;
        for (const auto& ts : thread_stats) total += ts.cache_misses;
        return total;
    }
    /** @return Penalización virtual de cambio asociada a misses, en ns. */
    long long get_stall_time_ns() const override {
        long long total = 0LL;
        for (const auto& ts : thread_stats) total += ts.stall_time_ns;
        return total;
    }
    /** @return Suma de costos de cambio de contexto inducidos por misses. */
    long long get_context_switch_time_ns() const override {
        long long total = 0LL;
        for (const auto& ts : thread_stats) total += ts.context_switch_time_ns;
        return total;
    }
    /** @return Transferencias reales entre contextos simulados. */
    int get_context_switches() const override {
        int total = 0;
        for (const auto& ts : thread_stats) total += ts.context_switches;
        return total;
    }
};

#endif // COARSE_RENDERER_H
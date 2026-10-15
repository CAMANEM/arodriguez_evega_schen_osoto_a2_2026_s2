/**
 * @file CoarseRenderer.cpp
 * @brief Scheduler CGMT que conserva el slot hasta un stall o fin de tile.
 */
#include "core/strategies/CoarseRenderer.h"
#include "core/config/raytracing_config.hpp"
#include "core/utils/RendererUtils.h"

using namespace constants;
using namespace trace;

/** @brief Divide el frame y prepara caches y estado del scheduler CGMT. */
CoarseRenderer::CoarseRenderer(int workers)
    : frame(IMAGE_WIDTH * IMAGE_HEIGHT),
      worker_count(validated_worker_count(workers, IMAGE_WIDTH * IMAGE_HEIGHT)),
      current_thread(0), threads_finished(0), global_clock_(0) {

    tasks.resize(worker_count);
    int total_pixels    = IMAGE_WIDTH * IMAGE_HEIGHT;
    int pixels_per_thread = total_pixels / worker_count;

    for (int i = 0; i < worker_count; ++i) {
        tasks[i].start     = i * pixels_per_thread;
        tasks[i].end       = (i == worker_count - 1) ? total_pixels
                                                     : (i + 1) * pixels_per_thread;
        tasks[i].thread_id = i;
    }

    cache_models.resize(worker_count);
    // Semilla determinista por thread (ver CacheModel.h)
    for (int i = 0; i < worker_count; ++i)
        cache_models[i] = CacheModel(CACHE_SIZE, 42u + static_cast<uint32_t>(i));

    thread_done.resize(worker_count, false);

    thread_stats.resize(worker_count);
    for (int i = 0; i < worker_count; ++i)
        thread_stats[i].thread_id = i;
}

// switch_to_next_thread: scheduler hardware CGMT.
//
// Busca el siguiente thread activo en round-robin saltando los que ya
// terminaron su tile. Si no hay otro activo, conserva el slot actual para
// evitar bloquear el único worker que aún puede avanzar. Requiere sched_mutex.
// Espejo del switch_to_next_thread() del código de referencia en C.
/** @brief Asigna el slot al siguiente tile activo; requiere sched_mutex. */
void CoarseRenderer::switch_to_next_thread() {
    for (int offset = 1; offset < worker_count; ++offset) {
        const int next = (current_thread + offset) % worker_count;
        if (!thread_done[next]) {
            current_thread = next;
            return;
        }
    }
}

// render_worker: ciclo principal del scheduler CGMT.
//
// Replica el patrón del código de referencia en C:
//   1. ESPERA  — bloquea hasta que el scheduler asigne este thread.
//   2a. STALL  — cache miss: no avanza el pixel, registra 3200 ns de latencia
//                y paga 400 ns de cambio antes de ceder el slot.
//   2b. COMPUTE — sin stall: renderiza el pixel y avanza al siguiente.
//                 Si terminó el tile: marca done y cede sin costo extra.
//   3. BROADCAST — notifica a todos para que el siguiente despierte.
//
// Diferencia clave vs FGMT: la rotación solo ocurre en stall, no en
// cada ciclo. El thread retiene el pipeline mientras no tenga stalls.
/**
 * @brief Ejecuta el tile del worker siguiendo la política CGMT.
 * @param thread_id Índice del worker que participa en el scheduler.
 */
void CoarseRenderer::render_worker(int thread_id) {
    const Task&    task  = tasks[thread_id];
    CacheModel&    cache = cache_models[thread_id];
    ThreadMetrics& stats = thread_stats[thread_id];

    int pixel_idx = task.start;
    bool pending_stall = false;  // true = stall pagado, no re-consultar cache

    while (true) {
        std::unique_lock<std::mutex> lock(sched_mutex);

        // Salida anticipada: todos los tiles completados
        if (threads_finished == worker_count) break;

        // CGMT: esperar a que el hardware asigne este contexto
        sched_cv.wait(lock, [&] {
            return current_thread == thread_id
                || threads_finished == worker_count;
        });

        if (threads_finished == worker_count) break;

        int x = pixel_idx % IMAGE_WIDTH;
        int y = pixel_idx / IMAGE_WIDTH;

        // Determinar miss solo una vez por píxel.
        // Si pending_stall, el dato ya volvió de memoria → proceder como HIT.
        if (!pending_stall && cache.is_cache_miss(x, y)) {
            // ── STALL DETECTADO → CAMBIO DE CONTEXTO CON COSTO ────────────
            // Registra por separado la latencia del miss y el cambio de contexto.
            // Este worker espera los 3200 ns completos, pero el scheduler entrega
            // el slot a otro worker tras contabilizar los 400 ns de cambio.
            stats.cache_misses++;
            stats.virtual_time_ns += CONTEXT_SWITCH_COST_NS;
            stats.stall_time_ns += CACHE_MISS_PENALTY_NS;
            stats.context_switch_time_ns += CONTEXT_SWITCH_COST_NS;
            pending_stall = true;
            const int previous_thread = current_thread;
            wait_for_stall_wall_time(CONTEXT_SWITCH_COST_NS);
            switch_to_next_thread();
            if (current_thread != previous_thread)
                stats.context_switches++;
            const int cycle = global_clock_++;
            const std::string note = "ctx switch→T" + std::to_string(current_thread);
            logger_.log_stall(cycle, thread_id, x, y, CONTEXT_SWITCH_COST_NS, note.c_str());
            sched_cv.notify_all();
            lock.unlock();
            wait_for_stall_wall_time(CACHE_MISS_PENALTY_NS - CONTEXT_SWITCH_COST_NS);
            continue;
        } else {
            // ── COMPUTE ────────────────────────────────────────────────────
            // Renderizar pixel y avanzar al siguiente de este tile
            frame[pixel_idx] = pixel_kernel_.compute(x, y);
            stats.virtual_time_ns += PIXEL_QUANTUM_NS;
            pending_stall = false;
            const int cycle = global_clock_++;
            logger_.log_compute(cycle, thread_id, x, y, PIXEL_QUANTUM_NS);
            pixel_idx++;

            if (pixel_idx >= task.end) {
                // Tile terminado: ceder al siguiente sin costo extra
                thread_done[thread_id] = true;
                threads_finished++;
                const int previous_thread = current_thread;
                switch_to_next_thread();
                if (threads_finished < worker_count && current_thread != previous_thread)
                    stats.context_switches++;
                logger_.log_done(cycle, thread_id);
            }
        }

        sched_cv.notify_all();
    }
}

/**
 * @brief Reinicia estado, ejecuta los workers y agrega el reloj virtual.
 * @return Buffer row-major con el frame completo.
 */
std::vector<Vector3> CoarseRenderer::render_frame() {
    // Reiniciar contadores, caches y estado del scheduler
    reset_thread_stats(thread_stats, cache_models);
    for (int i = 0; i < worker_count; ++i)
        thread_done[i] = false;
    current_thread   = 0;
    threads_finished = 0;
    global_clock_    = 0;
    logger_.log_header("cgmt", worker_count, 1, PIXEL_QUANTUM_NS, CACHE_MISS_PENALTY_NS);

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(worker_count));
    for (int i = 0; i < worker_count; ++i)
        threads.emplace_back(&CoarseRenderer::render_worker, this, i);
    for (auto& t : threads) t.join();

    // VT total = suma de threads (ejecución serial; stalls ocultos por switch)
    virtual_time_ns_ = sum_virtual_times(thread_stats);

    return frame;
}
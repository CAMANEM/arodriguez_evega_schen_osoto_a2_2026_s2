/**
 * @file FinegrainedRenderer.cpp
 * @brief Scheduler FGMT con señalización por semáforos y tiles de píxeles.
 */
#include "core/strategies/FinegrainedRenderer.h"
#include "core/config/raytracing_config.hpp"
#include "core/utils/RendererUtils.h"
#include <cstdint>
#include <thread>

using namespace constants;
using namespace trace;

/** @brief Divide el frame en tiles y crea cachés deterministas por worker. */
FinegrainedRenderer::FinegrainedRenderer(int workers)
    : frame(IMAGE_HEIGHT * IMAGE_WIDTH),
      worker_count_(validated_worker_count(workers, IMAGE_WIDTH * IMAGE_HEIGHT)),
      slots_(static_cast<std::size_t>(worker_count_)),
      tile_done_(new std::atomic<bool>[worker_count_]) {

    tiles.resize(worker_count_);

    //  cada thread recorre una banda contigua del frame en orden raster. Garantiza el mismo patrón de acceso
    const int total_pixels    = IMAGE_WIDTH * IMAGE_HEIGHT;
    const int pixels_per_thread = total_pixels / worker_count_;

    for (int i = 0; i < worker_count_; ++i) {
        tiles[i].start     = i * pixels_per_thread;
        tiles[i].end       = (i == worker_count_ - 1) ? total_pixels
                                                     : (i + 1) * pixels_per_thread;
        tiles[i].thread_id = i;
    }

    cache_models.resize(worker_count_);
    // Semilla determinista por thread: misma escena → mismo patrón de misses
    // en cada ejecución (reproducibilidad). Se diferencia por thread_id para
    // evitar que todos los tiles compartan el mismo estado inicial del RNG.
    for (int i = 0; i < worker_count_; ++i)
        cache_models[i] = CacheModel(CACHE_SIZE, 42u + static_cast<std::uint32_t>(i));

    thread_stats.resize(worker_count_);
    for (int i = 0; i < worker_count_; ++i)
        thread_stats[i].thread_id = i;
    for (int i = 0; i < worker_count_; ++i)
        tile_done_[i].store(false, std::memory_order_relaxed);
}

// Worker: scheduler FGMT con semáforos por thread (señalización punto a punto).
//
// Cada thread bloquea en sem_wait(&slots_[thread_id]) hasta recibir su turno.
// Al terminar el ciclo, señala directamente al siguiente thread con píxeles
// pendientes (skip-idle), eliminando los spurious wakeups de notify_all().
//
// Terminación: el último thread en completar su tile hace un broadcast
// (sem_post a todos los demás) para que salgan del sem_wait y terminen.
//
// VT: los threads IDLE no consumen slots de pipeline (duermen hasta el broadcast).
/**
 * @brief Procesa el tile de un worker bajo turnos de pipeline señalizados.
 * @param thread_id Índice del worker y de su semáforo.
 */
void FinegrainedRenderer::render_tile_worker(int thread_id) {
    ThreadMetrics&    stats = thread_stats[thread_id];
    CacheModel&       cache = cache_models[thread_id];
    const ThreadTile& tile  = tiles[thread_id];

    int pixel_idx = tile.start;
    bool pending_stall = false;  // true = ya se pagó el stall, no re-consultar cache

    while (true) {
        slots_[thread_id].acquire();

        // Salida: todos los tiles terminaron (broadcast recibido)
        if (threads_completed_.load(std::memory_order_acquire) == worker_count_) break;

        // Capturar ciclo global antes de procesar: serializado por el semáforo.
        const int cycle = global_cycle_.fetch_add(1, std::memory_order_relaxed);

        const int x = pixel_idx % IMAGE_WIDTH;
        const int y = pixel_idx / IMAGE_WIDTH;
        bool incurred_stall = false;

        // Ciclo de pipeline: determinar miss solo una vez por píxel.
        // Si pending_stall es true, el stall ya se pagó en el turno anterior
        // y este turno el dato ya está en cache → proceder como HIT.
        if (!pending_stall && cache.is_cache_miss(x, y)) {
            // STALL: el slot se ocupa con un NOP de duración PIXEL_QUANTUM_NS.
            // El slot fue gastado pero no produjo un píxel → el quantum completo
            // se desperdicia. El píxel se renderizará en el próximo turno.
            stats.virtual_time_ns += PIXEL_QUANTUM_NS;
            stats.cache_misses++;
            stats.stall_time_ns += CACHE_MISS_PENALTY_NS;
            incurred_stall = true;
            pending_stall = true;
            logger_.log_stall(cycle, thread_id, x, y, PIXEL_QUANTUM_NS, "slot wasted");
        } else {
            // COMPUTE: renderizar pixel y avanzar al siguiente
            frame[y * IMAGE_WIDTH + x] = pixel_kernel_.compute(x, y);
            stats.virtual_time_ns += PIXEL_QUANTUM_NS;
            logger_.log_compute(cycle, thread_id, x, y, PIXEL_QUANTUM_NS);
            pending_stall = false;

            pixel_idx++;
            if (pixel_idx >= tile.end) {
                logger_.log_done(cycle, thread_id);
                tile_done_[thread_id].store(true, std::memory_order_release);
                int done = threads_completed_.fetch_add(1, std::memory_order_acq_rel) + 1;
                if (done == worker_count_) {
                    // Último en terminar: despertar todos los demás para que salgan
                    for (int i = 0; i < worker_count_; ++i)
                        if (i != thread_id) slots_[i].release();
                    break;
                }
                // Este tile terminó — caer al bloque de señalización
                // para pasar el control al siguiente thread activo
            }
        }

        // Señalar al siguiente thread con píxeles pendientes (skip IDLE).
        // Si solo quedo este mismo thread activo, se señala a sí mismo.
        int next    = (thread_id + 1) % worker_count_;
        int checked = 0;
        while (tile_done_[next].load(std::memory_order_acquire) && checked < worker_count_) {
            next = (next + 1) % worker_count_;
            ++checked;
        }
        // checked == worker_count_ solo si todos están done — ya salimos arriba
        if (next != thread_id)
            stats.context_switches++;
        slots_[next].release();
        if (incurred_stall)
            wait_for_stall_wall_time(CACHE_MISS_PENALTY_NS);
    }
}

/**
 * @brief Reinicia el scheduler, ejecuta sus workers y reúne el frame.
 * @return Buffer row-major con todos los píxeles renderizados.
 */
std::vector<Vector3> FinegrainedRenderer::render_frame() {
    reset_thread_stats(thread_stats, cache_models);
    virtual_time_ns_   = 0LL;
    threads_completed_.store(0, std::memory_order_relaxed);
    global_cycle_.store(0, std::memory_order_relaxed);
    logger_.log_header("fgmt", worker_count_, 1, PIXEL_QUANTUM_NS, CACHE_MISS_PENALTY_NS);

    // Inicializar semáforos: thread 0 parte listo, el resto bloqueado.
    for (int i = 0; i < worker_count_; ++i) {
        slots_[i].reset(i == 0);
        tile_done_[i].store(false, std::memory_order_relaxed);
    }

    std::vector<std::thread> threads;
    threads.reserve(static_cast<std::size_t>(worker_count_));
    for (int i = 0; i < worker_count_; ++i)
        threads.emplace_back(&FinegrainedRenderer::render_tile_worker, this, i);
    for (auto& t : threads) t.join();

    // VT total = SUMA de los threads activos (pipeline compartido, no paralelo).
    virtual_time_ns_ = sum_virtual_times(thread_stats);

    return frame;
}

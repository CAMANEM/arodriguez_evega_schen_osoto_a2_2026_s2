/**
 * @file FinegrainedRenderer.h
 * @brief Declaración del scheduler FGMT con workers por tile.
 */
#ifndef FINEGRAINED_RENDERER_H
#define FINEGRAINED_RENDERER_H

#include "IRenderer.h"
#include "Scene.h"
#include "CacheModel.h"
#include "Metrics.h"
#include "Ray.h"
#include "raytracing_config.hpp"
#include "SchedulerLogger.h"
#include <vector>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <mutex>

/** @brief Semáforo binario con mutex y condition_variable para turnos FGMT. */
class CountingSemaphore {
public:
    /** @brief Establece si el semáforo comienza disponible. */
    void reset(bool available) {
        std::lock_guard<std::mutex> lock(mutex_);
        available_ = available;
    }

    /** @brief Espera hasta adquirir el único permiso disponible. */
    void acquire() {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return available_; });
        available_ = false;
    }

    /** @brief Libera el permiso y despierta a un waiter. */
    void release() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            available_ = true;
        }
        condition_.notify_one();
    }

private:
    std::mutex mutex_;
    std::condition_variable condition_;
    bool available_ = false;
};

// FinegrainedRenderer: modelo FGMT (Fine-Grained Multithreading).
//
// Simula 1 pipeline con NUM_THREADS contextos de hardware.
// Scheduler: semáforo por thread — cada thread espera en su propio
// sem_wait() y, al terminar su ciclo, señala directamente al siguiente
// thread con píxeles pendientes (skip de IDLE).
//
// Esto elimina los spurious wakeups que generaba notify_all():
//   Antes : notify_all() → wakeup × NUM_THREADS × 192 000 ciclos
//   Ahora : sem_post(next) → 1 wakeup × ciclos_activos
//
// Comportamiento por ciclo:
//   Sin stall : COMPUTE — renderiza el pixel y avanza (+PIXEL_QUANTUM_NS)
//   Con stall : NOP     — stall oculto; otro contexto toma el pipeline
//                          (+NOP_PENALTY_NS)
//   IDLE      : el thread duerme hasta broadcast final (no consume VT)
//
// Tiempo virtual = SUMA de los cuatro threads (pipeline compartido).
/** @brief Simula FGMT con un slot compartido y rotación en cada turno activo. */
class FinegrainedRenderer : public IRenderer {
private:
    Scene scene;
    std::vector<Vector3>              frame;
    std::vector<CacheModel>           cache_models;
    std::vector<trace::ThreadMetrics> thread_stats;

    struct ThreadTile {
        int start, end;    // Rango lineal de píxeles [start, end)
        int thread_id;
    };
    std::vector<ThreadTile> tiles;

    long long virtual_time_ns_ = 0LL;

    // Posición de cámara para el frame actual (actualizada por GenericRunner).
    // Permite que el scheduler reciba la órbita elíptica sin cambiar su lógica.
    Vector3 camera_pos_;

    // Scheduler FGMT: semáforo por thread para señalización punto a punto.
    // slots_[i]: thread i espera aquí su turno de pipeline.
    // tile_done_[i]: true cuando thread i terminó todos sus píxeles.
    // threads_completed_: contador atómico; al llegar a NUM_THREADS el
    //   último thread hace broadcast para desbloquear los demás.
    CountingSemaphore        slots_[constants::NUM_THREADS];
    std::atomic<int>         threads_completed_{0};
    std::atomic<bool>        tile_done_[constants::NUM_THREADS];
    // global_cycle_: cuenta cuántos slots de pipeline se han despachado.
    // Incrementado atómicamente por el thread activo al tomar el semáforo.
    // Serializado de facto por el protocolo de semáforos (un thread activo).
    std::atomic<int>         global_cycle_{0};
    SchedulerLogger          logger_;   // Traza ciclo-a-ciclo (activar con set_verbose)

    /** @param thread_id Identificador del worker y del tile que procesa. */
    void render_tile_worker(int thread_id);

public:
    // Constructor: divide el frame en 4 tiles 2×2 (uno por thread) e inicializa
    // los CacheModel con semillas deterministas (base 42 + thread_id).
    /** @brief Divide el frame entre NUM_THREADS y prepara cachés deterministas. */
    FinegrainedRenderer();

    // Actualiza la posición de cámara antes de render_frame().
    // GenericRunner la llama una vez por frame; el scheduler FGMT no cambia.
    /** @param pos Posición de cámara para el siguiente frame. */
    void set_camera_pos(const Vector3& pos) override { camera_pos_ = pos; }

    // Habilita la traza del scheduler para los primeros `cycles` ciclos de pipeline.
    /** @param cycles Cantidad de ciclos iniciales que se registran. */
    void set_verbose(int cycles) override { logger_.set_max_cycles(cycles); }

    // render_frame(): resetea estado, lanza NUM_THREADS threads (uno por tile) y
    // espera a que todos terminen. VT = suma de VTs por thread (pipeline compartido).
    /** @return Frame completo después de que todos los workers terminen. */
    std::vector<Vector3> render_frame() override;

    // get_model_name(): identifica el modelo para logging/CSV
    /** @return Identificador `fgmt`. */
    std::string get_model_name() const override { return "fgmt"; }

    // get_thread_metrics(): estadísticas (misses, VT) de los 4 threads del último frame.
    /** @return Contadores por worker del último frame. */
    const std::vector<trace::ThreadMetrics>& get_thread_metrics() const override { return thread_stats; }

    // get_frame(): frame procesado (para debugging o exportación manual)
    /** @return Referencia de solo lectura al último buffer renderizado. */
    const std::vector<Vector3>& get_frame() const { return frame; }
    /** @return Suma del tiempo virtual de los contextos, en nanosegundos. */
    long long get_virtual_time_ns() const override { return virtual_time_ns_; }
    /** @return Total de misses registrados por los workers. */
    int get_total_stalls() const override {
        int total = 0;
        for (const auto& ts : thread_stats) total += ts.cache_misses;
        return total;
    }
};

#endif // FINEGRAINED_RENDERER_H

/**
 * @file SMTRenderer.h
 * @brief Declaración del simulador SMT por ciclos y contextos virtuales.
 */
#ifndef SMT_RENDERER_H
#define SMT_RENDERER_H

#include "IRenderer.h"
#include "Scene.h"
#include "CacheModel.h"
#include "Metrics.h"
#include "Ray.h"
#include "raytracing_config.hpp"
#include "SchedulerLogger.h"
#include <vector>

// SMTRenderer: Renderizador SMT (Simultaneous Multithreading) — modelo por píxel.
//
// Modela W=SMT_ISSUE_WIDTH=2 slots de issue simultáneos con
// hardware_concurrency() × SMT_OVERSUBSCRIPTION_FACTOR contextos virtuales.
// Cada contexto procesa píxeles completos y permanece en su slot hasta un stall.
//
// Modelo de ejecución:
//   - Thread en slot → procesa píxeles uno a uno.
//   - Cache hit  → píxel renderizado; thread acumula PIXEL_QUANTUM_NS de VT.
//   - Cache miss → thread CEDE inmediatamente el slot (stall oculto);
//                  el siguiente thread listo entra y procesa trabajo real.
//   - Thread stallado → espera CACHE_MISS_PENALTY_NS/PIXEL_QUANTUM_NS ciclos
//                       antes de re-entrar a la cola de listos.
//
// Diferencia clave vs FGMT/CGMT:
//   - FGMT: rota SIEMPRE cada ciclo (1 slot), paga PIXEL_QUANTUM_NS en stall.
//   - CGMT: rota solo en stall (1 slot), paga 0 VT (stall oculto, cambio inmediato).
//   - SMT:  W=2 slots simultáneos; stall → swap inmediato, 0 VT desperdiciado.
//
// Implementación: simulación pura sin OS threads ni semáforos; la sobre-suscripción
// aumenta los contextos virtuales, no los threads reales del proceso.
// La simultaneidad de W=2 se modela procesando W slots por iteración del loop.
// Rendimiento: microsegundos/frame (sin syscalls de futex).
//
// Tiempo Virtual (VT):
//   VT = global_clock_ × PIXEL_QUANTUM_NS  (reloj de pared de la simulación).
//   Con W=2 y stalls ocultos: VT ≈ (total_píxeles / W) × PIXEL_QUANTUM_NS.
//   Speedup vs Sequential (Amdahl, W=2): ~2×.
//   NO es la suma de VT por thread — esa suma siempre = total_px × Q.
/**
 * @brief Simula SMT con emisión simultánea y contextos virtuales sobre-suscritos.
 * @note No crea hilos del sistema operativo; el paralelismo se simula por ciclo.
 */
class SMTRenderer : public IRenderer {
public:
    /** @brief Crea contextos según smt_context_count() y distribuye los píxeles. */
    SMTRenderer();

    // Habilitar logging ciclo a ciclo. cycles = número de ciclos a imprimir (0 = off).
    /** @param cycles Cantidad de ciclos iniciales que se registran. */
    void set_verbose(int cycles) override { logger_.set_max_cycles(cycles); }

    // Actualiza la posición de cámara antes de render_frame().
    // GenericRunner la llama una vez por frame; el scheduler SMT no cambia.
    /** @param pos Posición de cámara para el siguiente frame. */
    void set_camera_pos(const Vector3& pos) override { camera_pos_ = pos; }

    /** @return Frame completo producido por la simulación SMT. */
    std::vector<Vector3> render_frame() override;

    /** @return Identificador `smt`. */
    std::string get_model_name() const override { return "smt"; }

    /** @return Estadísticas asociadas a cada contexto virtual. */
    const std::vector<trace::ThreadMetrics>& get_thread_metrics() const override {
        return thread_stats_;
    }

    /** @return Duración del reloj simulado SMT, en nanosegundos. */
    long long get_virtual_time_ns() const override { return virtual_time_ns_; }

    /** @return Suma de misses de caché de todos los contextos. */
    int get_total_stalls() const override {
        int total = 0;
        for (const auto& ts : thread_stats_) total += ts.cache_misses;
        return total;
    }

private:
    // Rango de píxeles asignado a cada contexto virtual.
    struct Task { int start, end; };

    Scene   scene_;
    Vector3 camera_pos_;   // Posición de cámara para el frame actual (órbita elíptica)
    std::vector<Vector3>              frame_;
    std::vector<CacheModel>           cache_models_;
    std::vector<trace::ThreadMetrics> thread_stats_;
    std::vector<Task>                 tasks_;

    long long virtual_time_ns_ = 0LL;
    int       global_clock_    = 0;

    // ── Estado por contexto hardware ─────────────────────────────────────
    std::vector<int>  pixel_idx_;       // píxel actual de cada thread
    std::vector<int>  stall_countdown_; // ciclos restantes de stall (0 = listo)
    std::vector<bool> thread_finished_; // tile completado
    std::vector<bool> pending_stall_;   // true = stall pagado, no re-consultar cache

    SchedulerLogger logger_; // Traza ciclo-a-ciclo (activar con set_verbose)
};

#endif // SMT_RENDERER_H

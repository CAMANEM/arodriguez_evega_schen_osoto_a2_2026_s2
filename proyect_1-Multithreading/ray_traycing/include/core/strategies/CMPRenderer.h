/**
 * @file CMPRenderer.h
 * @brief Declaración del renderer CMP con ejecución en hilos de SO.
 */
#ifndef CMP_RENDERER_H
#define CMP_RENDERER_H

#include "core/strategies/IRenderer.h"
#include "core/utils/CacheModel.h"
#include "core/utils/Metrics.h"
#include "core/utils/SchedulerLogger.h"
#include <vector>

// CMPRenderer: modelo CMP (Chip Multiprocessing) — paralelismo real multinúcleo.
//
// Cada "núcleo" es un hilo del SO corriendo simultáneamente sobre distintos
// núcleos físicos del CPU. NO hay pipeline compartido: cada núcleo tiene su
// propio pipeline, su propia cache L1 y su propio contador de VT.
//
// Diferencias clave con los otros modelos:
//   - Sequential : 1 hilo, stall completo, sin solapamiento.
//   - FGMT/CGMT  : 1 pipeline compartido entre N contextos (time-sliced).
//   - SMT        : 1 core con issue-width=2 (despacha 2 slots/ciclo).
//   - CMP        : N cores independientes en paralelo real (OS threads).
//
// VT(CMP) = max(VT por núcleo) — ya que los núcleos avanzan en paralelo
// el reloj de pared del sistema avanza al ritmo del núcleo más lento.
// Cada núcleo paga su propio CACHE_MISS_PENALTY_NS (sin otro thread que lo oculte).
//
// Cada core procesa un rango contiguo del frame en paralelo.
/**
 * @brief Renderiza tiles en paralelo real con los workers CMP configurados.
 * @note El reloj virtual agregado es el máximo de los tiempos por core.
 */
class CMPRenderer : public IRenderer {
private:
    const int worker_count_;
    std::vector<Vector3>              frame_;
    std::vector<CacheModel>           cache_models_;
    std::vector<trace::ThreadMetrics> core_stats_;

    // Rango de píxeles asignado a cada núcleo (índice lineal, row-major).
    struct CoreTile {
        int start;   // índice de primer píxel (inclusivo)
        int end;     // índice de último píxel + 1 (exclusivo)
        int core_id;
    };
    std::vector<CoreTile> tiles_;

    long long virtual_time_ns_ = 0LL;
    SchedulerLogger logger_; // Traza ciclo-a-ciclo (activar con set_verbose)

    // Worker ejecutado por cada núcleo en un OS thread independiente.
    // Procesa su tile de forma autónoma (Sequential-like) sin coordinación
    // con los otros núcleos: no hay mutex, no hay semáforos, no hay señales.
    /** @param core_id Índice del core y del rango de píxeles asignado. */
    void render_core_worker(int core_id);

public:
    // Divide el frame en rangos contiguos; el último recibe los píxeles residuales.
    // CacheModel semilla determinista: base 42 + core_id → reproducibilidad.
    /** @param workers Cantidad de workers CMP, entre 1 y la cantidad de píxeles. */
    explicit CMPRenderer(int workers = constants::CMP_NUM_CORES);

    // Habilita la traza del scheduler para los primeros `cycles` ciclos de pipeline.
    // En CMP los núcleos corren en paralelo real: las líneas de log pueden intercalarse.
    /** @param cycles Cantidad de ciclos locales iniciales que se registran. */
    void set_verbose(int cycles) override { logger_.set_max_cycles(cycles); }

    // render_frame(): lanza un OS thread por worker y espera a que terminen
    // y retorna el frame completo.
    // VT = max(per-core VT): los cores corren en paralelo real.
    /** @return Frame completo cuando todos los workers terminan. */
    std::vector<Vector3> render_frame() override;

    /** @return Identificador `cmp`. */
    std::string get_model_name() const override { return "cmp"; }
    /** @return Máximo tiempo virtual entre cores, en nanosegundos. */
    long long   get_virtual_time_ns() const override { return virtual_time_ns_; }
    /** @return Total de misses de caché de los cores. */
    int get_total_stalls() const override {
        int total = 0;
        for (const auto& c : core_stats_) total += c.cache_misses;
        return total;
    }
    /** @return Suma de latencias virtuales de misses de todos los cores, en ns. */
    long long get_stall_time_ns() const override {
        long long total = 0LL;
        for (const auto& c : core_stats_) total += c.stall_time_ns;
        return total;
    }
    /** @return Estadísticas del último frame por core. */
    const std::vector<trace::ThreadMetrics>& get_thread_metrics() const override {
        return core_stats_;
    }
};

#endif // CMP_RENDERER_H

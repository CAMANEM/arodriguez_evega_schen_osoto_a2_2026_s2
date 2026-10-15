/**
 * @file SequentialRenderer.h
 * @brief Baseline secuencial del contrato IRenderer.
 */
#ifndef SEQUENTIAL_RENDERER_H
#define SEQUENTIAL_RENDERER_H

#include "core/strategies/IRenderer.h"
#include "core/utils/CacheModel.h"
#include "core/utils/SchedulerLogger.h"
#include <vector>

/** @brief Renderer de referencia de un worker, usado como baseline secuencial. */
// SequentialRenderer: modelo de referencia sin multithreading (baseline).
//
// Renderiza todos los pixels en un solo hilo, fila por fila (row-major).
// El pipeline secuencial paga el costo COMPLETO de cada stall:
//   CACHE_MISS_PENALTY_NS = NOPS_PER_STALL x NOP_PENALTY_NS = 3200 ns
// porque no hay otro thread que pueda ejecutar mientras espera la memoria.
//
// Se usa como linea base para calcular el speedup de FGMT y CGMT.
class SequentialRenderer : public IRenderer {
private:
    CacheModel cache;                  // Una unica instancia de cache (hilo unico)
    long long virtual_time_ns_ = 0LL; // Tiempo de reloj virtual del ultimo render_frame()
    int stall_count_ = 0;             // Cache misses en el ultimo render_frame()
    long long stall_time_ns_ = 0LL;
    SchedulerLogger logger_;          // Traza ciclo-a-ciclo (activar con set_verbose)

public:
    /** @brief Crea la escena y el modelo de caché secuencial. */
    SequentialRenderer();

    // Habilita la traza del scheduler para los primeros `cycles` ciclos de pipeline.
    /** @param cycles Ciclos iniciales que se imprimirán; cero desactiva la traza. */
    void set_verbose(int cycles) override { logger_.set_max_cycles(cycles); }

    /** @return Frame renderizado row-major y estado virtual actualizado. */
    std::vector<Vector3> render_frame() override;
    /** @return Identificador `sequential`. */
    std::string get_model_name() const override { return "sequential"; }
    /** @return Tiempo virtual del frame, en nanosegundos. */
    long long get_virtual_time_ns() const override { return virtual_time_ns_; }
    /** @return Misses de caché del último frame. */
    int get_total_stalls() const override { return stall_count_; }
    /** @return Latencia virtual de misses en nanosegundos. */
    long long get_stall_time_ns() const override { return stall_time_ns_; }
};

#endif // SEQUENTIAL_RENDERER_H

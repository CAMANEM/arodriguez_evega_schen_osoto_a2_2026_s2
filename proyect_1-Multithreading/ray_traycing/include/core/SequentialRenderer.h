/**
 * @file SequentialRenderer.h
 * @brief Baseline secuencial del contrato IRenderer.
 */
#ifndef SEQUENTIAL_RENDERER_H
#define SEQUENTIAL_RENDERER_H

#include "IRenderer.h"
#include "Scene.h"
#include "Ray.h"
#include "CacheModel.h"
#include "raytracing_config.hpp"
#include "SchedulerLogger.h"
#include <vector>

/** @brief Renderer de referencia de un worker, usado como baseline secuencial. */
// SequentialRenderer: modelo de referencia sin multithreading (baseline).
//
// Renderiza todos los pixels en un solo hilo, fila por fila (row-major).
// Es el unico modelo que paga el costo COMPLETO de cada stall:
//   CACHE_MISS_PENALTY_NS = NOPS_PER_STALL x NOP_PENALTY_NS = 3200 ns
// porque no hay otro thread que pueda ejecutar mientras espera la memoria.
//
// Se usa como linea base para calcular el speedup de FGMT y CGMT.
class SequentialRenderer : public IRenderer {
private:
    Scene scene;
    CacheModel cache;                  // Una unica instancia de cache (hilo unico)
    long long virtual_time_ns_ = 0LL; // Tiempo de reloj virtual del ultimo render_frame()
    int stall_count_ = 0;             // Cache misses en el ultimo render_frame()
    Vector3 camera_pos_;              // Posición de cámara para el frame actual
    SchedulerLogger logger_;          // Traza ciclo-a-ciclo (activar con set_verbose)

public:
    /** @brief Crea la escena y el modelo de caché secuencial. */
    SequentialRenderer();

    // Actualiza la posición de la cámara antes de render_frame().
    // GenericRunner la llama una vez por frame con la posición de la órbita.
    /** @param pos Nueva posición de cámara para los píxeles del siguiente frame. */
    void set_camera_pos(const Vector3& pos) override { camera_pos_ = pos; }

    // Habilita la traza del scheduler para los primeros `cycles` ciclos de pipeline.
    /** @param cycles Ciclos iniciales que se imprimirán; cero desactiva la traza. */
    void set_verbose(int cycles) override { logger_.set_max_cycles(cycles); }

    // Proyecta el pixel (x, y) usando make_ray con look-at desde camera_pos_.
    /**
     * @brief Calcula un píxel con la cámara y carga actuales.
     * @param x Coordenada horizontal.
     * @param y Coordenada vertical.
     * @return Color RGB del píxel.
     */
    Vector3 render_pixel(int x, int y) const {
        return compute_pixel(scene, x, y, camera_pos_, workload_);
    }

    /** @return Frame renderizado row-major y estado virtual actualizado. */
    std::vector<Vector3> render_frame() override;
    /** @return Identificador `sequential`. */
    std::string get_model_name() const override { return "sequential"; }
    /** @return Tiempo virtual del frame, en nanosegundos. */
    long long get_virtual_time_ns() const override { return virtual_time_ns_; }
    /** @return Misses de caché del último frame. */
    int get_total_stalls() const override { return stall_count_; }
};

#endif // SEQUENTIAL_RENDERER_H

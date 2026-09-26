/**
 * @file IRenderer.h
 * @brief Contrato común para renderers con distintas políticas de ejecución.
 */
#ifndef IRENDERER_H
#define IRENDERER_H

#include <vector>
#include <string>
#include "Vector3.h"
#include "Metrics.h"
#include "Workload.h"

/**
 * @brief Interfaz polimórfica para renderizar frames con cualquier esquema.
 *
 * Mantiene los clientes independientes de las implementaciones secuencial,
 * FGMT, CGMT, SMT y CMP.
 */
class IRenderer {
public:
    /** @brief Destructor virtual para destruir implementaciones por interfaz. */
    virtual ~IRenderer() = default;

    /** @brief Selecciona la carga que usará cada píxel. */
    void set_workload(Workload workload) { workload_ = workload; }
    /** @return Carga configurada actualmente. */
    Workload get_workload() const { return workload_; }

    /** @brief Renderiza un frame y devuelve IMAGE_WIDTH × IMAGE_HEIGHT colores row-major. */
    virtual std::vector<Vector3> render_frame() = 0;

    /** @return Métricas por worker; vacío si el modelo no las recopila. */
    virtual const std::vector<trace::ThreadMetrics>& get_thread_metrics() const {
        static const std::vector<trace::ThreadMetrics> empty;
        return empty;
    }

    /** @return Tiempo virtual del último frame en nanosegundos; no es tiempo de pared. */
    virtual long long get_virtual_time_ns() const { return 0LL; }

    /** @return Misses de caché del último frame. */
    virtual int get_total_stalls() const { return 0; }

    /** @return Identificador estable del modelo. */
    virtual std::string get_model_name() const = 0;

    /**
     * @brief Establece la posición de cámara para el siguiente frame.
     * @param pos Posición de cámara en coordenadas del mundo.
     * @note Implementación opcional; por defecto no modifica el renderer.
     */
    virtual void set_camera_pos(const Vector3& /*pos*/) {}

    /**
     * @brief Habilita logging del scheduler para los primeros ciclos.
     * @param cycles Cantidad de ciclos a registrar; cero desactiva el logging.
     * @note Implementación opcional; por defecto no hace nada.
     */
    virtual void set_verbose(int /*cycles*/) {}

protected:
    Workload workload_ = Workload::raytracing;
};

#endif // IRENDERER_H

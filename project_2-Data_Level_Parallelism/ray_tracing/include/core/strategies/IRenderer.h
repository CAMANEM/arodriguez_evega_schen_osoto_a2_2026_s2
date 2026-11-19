#pragma once
#include "geometry/PixelKernel.h"
#include "geometry/Vector3.h"
#include <string>
#include <vector>

/**
 * @brief Interfaz común para todos los renderers DLP del proyecto 2.
 *
 * Desacopla al main_benchmark de las implementaciones concretas:
 * Sequential, SIMD, CUDA y FPGA implementan esta misma interfaz.
 *
 * A diferencia del proyecto 1, no hay métricas de tiempo virtual ni
 * simulación de caché: solo tiempo de pared real.
 */
class IRenderer {
public:
    virtual ~IRenderer() = default;

    /** @brief Renderiza un frame completo y devuelve el buffer de colores row-major. */
    virtual std::vector<Vector3> render_frame() = 0;

    /** @return Nombre del modelo, e.g. "sequential", "simd", "cuda", "fpga". */
    virtual std::string get_model_name() const = 0;

    /** @brief Establece la posición de cámara para el siguiente render_frame(). */
    virtual void set_camera_pos(const Vector3& pos) { pixel_kernel_.set_camera_pos(pos); }

protected:
    PixelKernel pixel_kernel_;
};

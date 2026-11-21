#pragma once

#include "config/raytracing_config.hpp"
#include "core/strategies/IRenderer.h"

#include <string>
#include <vector>

/**
 * @brief Renderer CUDA que asigna un hilo a cada píxel de la imagen.
 *
 * @details La escena se copia al dispositivo como datos simples y el kernel
 * calcula intersecciones rayo-círculo en paralelo. El resultado se devuelve
 * como un buffer row-major compatible con las demás estrategias.
 */
class CudaRenderer final : public IRenderer {
public:
    /**
     * @brief Renderiza un frame completo con CUDA.
     * @return Colores RGB en orden row-major.
     * @throws std::runtime_error Si una operación CUDA falla.
     */
    std::vector<Vector3> render_frame() override;

    /**
     * @brief Obtiene el identificador de esta estrategia.
     * @return El nombre "cuda".
     */
    std::string get_model_name() const override { return "cuda"; }

    /**
     * @brief Establece la posición de cámara usada por el siguiente frame.
     * @param pos Posición 2D de la cámara.
     */
    void set_camera_pos(const Vector2& pos) override;

private:
    Vector2 camera_pos_ = constants::CAMERA_ORIGIN;
};

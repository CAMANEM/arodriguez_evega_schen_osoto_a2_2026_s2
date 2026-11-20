#pragma once
#include "core/strategies/IRenderer.h"
#include <vector>

/**
 * @brief Renderer baseline: un solo hilo, sin ninguna forma de paralelismo.
 *
 * Recorre los píxeles en orden row-major. Es la referencia para calcular el
 * speedup de SIMD, CUDA y FPGA. No simula caché ni tiempo virtual (eso era
 * exclusivo del proyecto 1).
 */
class SequentialRenderer : public IRenderer {
public:
    std::vector<Vector3> render_frame() override;
    std::string          get_model_name() const override { return "sequential"; }
};

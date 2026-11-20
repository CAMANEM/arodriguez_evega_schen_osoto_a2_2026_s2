#pragma once
#include "core/strategies/IRenderer.h"
#include "geometry/Scene.h"

class SimdRenderer final : public IRenderer {
public:
    std::vector<Vector3> render_frame() override;
    std::string get_model_name() const override { return "simd"; }
    void set_camera_pos(const Vector3& pos) override;

private:
    Scene scene_;
    Vector3 camera_pos_;
};

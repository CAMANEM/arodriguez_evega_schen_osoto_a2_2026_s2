#pragma once

#include "../../shared/include/object_interface.hpp"

/* ===== BODY ================================================================================== */
class Body : public object_interface {

public:
    /* ===== CONSTANTS ========================================================================= */
    static constexpr int SCREEN_W = 1600;
    static constexpr int SCREEN_H = 900;

    /* ===== CONSTRUCTOR AND DESTRUCTOR ======================================================== */
    explicit Body(int id, double mass = 1.0);
    ~Body() override = default;

    /* ===== OTHER METHODS ===================================================================== */
    void update(double dt) override;
    void reset() override;
};

# Project 1 — Multithreading Simulations

Particle simulations implemented in C++ exploring multithreading strategies.
Each problem is self-contained and shares a common object interface.

## Problems

| Folder | Simulation |
|---|---|
| `boids/` | Flocking behavior (separation, alignment, cohesion) |
| `n_body/` | Gravitational attraction — universal law |
| `liquids/` | Particle-based fluid simulation |
| `ray_traycing/` | Ray tracing |

## Structure

```
proyect_1-Multithreading/
├── shared/
│   ├── include/
│   │   └── object_interface.hpp
│   └── src/
├── boids/
│   ├── include/
│   ├── src/
│   └── data/
├── n_body/
│   ├── include/
│   ├── src/
│   └── data/
├── liquids/
│   ├── include/
│   ├── src/
│   └── data/
├── ray_traycing/
│   ├── include/
│   ├── src/
│   └── data/
├── docs/
│   └── Proyectos_Arqui2_Proyecto_Individual_v3.pdf
└── scripts/
    └── plot_metrics.py
```

## Shared Interface

`shared/include/object_interface.hpp` defines the abstract base class that all
simulation objects must inherit from.

```cpp
#include "../../shared/include/object_interface.hpp"

class MyObject : public object_interface {
public:
    MyObject(int id, double mass, double x, double y)
        : object_interface(id, mass) {set_position(x, y);}

    void compute_forces() override { /* problem-specific logic */ }
    void update(double dt) override { /* problem-specific logic */ }
    void reset() override { /* problem-specific logic */ }
};
```

**Attributes inherited from `object_interface`:**

| Attribute | Type | Description |
|---|---|---|
| `id` | `int` | Unique object identifier |
| `mass` | `double` | Object mass |
| `pos_x`, `pos_y` | `double` | Position |
| `force_x`, `force_y` | `double` | Accumulated force |
| `speed_x`, `speed_y` | `double` | Velocity |

## Data

Each problem stores its VTune Profiler measurements and screenshots under its own `data/` folder.

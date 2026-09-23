# Project 1 — Multithreading Simulations

Particle simulations implemented in C++ evaluated under four execution models:
Fine-Grained, Coarse-Grained, SMT, and CMP multithreading.

## Problems

| Folder | Simulation |
|---|---|
| `boids/` | Flocking behavior (separation, alignment, cohesion) |
| `n_body/` | Gravitational attraction (universal law) |
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

`shared/include/object_interface.hpp` defines the abstract base class all simulation
objects must inherit from. It holds the common physical state and declares the two
methods every object must implement.

**Attributes:**

| Attribute | Type | Description |
|---|---|---|
| `id` | `int` | Unique object identifier |
| `mass` | `double` | Object mass |
| `pos_x`, `pos_y` | `double` | Position |
| `acc_x`, `acc_y` | `double` | Acceleration |
| `force_x`, `force_y` | `double` | Accumulated force |
| `speed_x`, `speed_y` | `double` | Velocity |

**Other methods:**

| Method | Description |
|---|---|
| `update(double dt)` | Integration step — applies stored forces to update acceleration, velocity and position |
| `reset()` | Restores object to its initial state — required between benchmark runs |

Force computation is intentionally excluded from the interface. Each problem implements
its own force function which operates over the full collection of objects and deposits
results via `set_force()` before `update(dt)` is called. This separation allows both
phases to be parallelized independently by the execution scheme.

**Usage:**

```cpp
#include "../../shared/include/object_interface.hpp"

class MyObject : public object_interface {
public:
    MyObject(int id, double mass, double x, double y)
        : object_interface(id, mass) {set_position(x, y);}

    void update(double dt) override { /* integration logic */ }
    void reset() override { /* restore initial state */ }
};
```

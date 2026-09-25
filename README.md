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
│   │   ├── object_interface.hpp
│   │   └── metrics_interface.hpp
│   └── src/
│       └── metrics_interface.cpp
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

## Shared Metrics Interface

`shared/include/metrics_interface.hpp` declares the base class every problem uses
to collect the metrics required by the assignment. Implementations live in
`shared/src/metrics_interface.cpp`. One instance represents a
single experimental configuration (execution model + worker count) across
repeated runs. Scalability is obtained by comparing several instances, not by
extra fields on this class.

Problem-specific counters (cache events, pixels shaded, etc.) stay out of this
header: inherit and add members, then `override` `record_time` / `reset` if those
extras must be updated together with the common times.

**Attributes:**

| Attribute | Type | Description |
|---|---|---|
| `model` | `execution_model` | Sequential, fine-grained, or coarse-grained |
| `n_workers` | `int` | Threads or processes used in this configuration |
| `sequential_time` | `double` | Baseline wall time for speedup (`T_seq`) |
| `run_times` | `std::vector<double>` | Wall time of each repetition (seconds) |

**Other methods:**

| Method | Description |
|---|---|
| `record_time(double seconds)` | Append one run; override to also record extra metrics |
| `reset()` | Clears run samples; override to also clear extra metrics |
| `mean_time()` | Mean wall time over recorded runs |
| `stddev_time()` | Sample standard deviation of wall time |
| `speedup()` | `T_seq / mean_time` |
| `efficiency()` | `speedup / n_workers` |
| `ci95_lower()`, `ci95_upper()` | 95 % CI of mean time (`mean ± 1.96 σ / √n`) |

**Usage:**

```cpp
#include "../../shared/include/metrics_interface.hpp"

class MyMetrics : public metrics_interface {
public:
    using metrics_interface::metrics_interface;

    void record_time(double seconds) override {
        metrics_interface::record_time(seconds);
        /* optional: record extra per-run data */
    }

    void reset() override {
        metrics_interface::reset();
        /* optional: clear extra per-run data */
    }
};
```

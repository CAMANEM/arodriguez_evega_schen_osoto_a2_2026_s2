# SPH liquids — interface scaffold

This directory contains the initial interface layer for the SPH fluid
simulation. The current iteration intentionally does not implement neighbor
search, density, pressure, viscosity, boundary handling, or execution schemes.

## Current components

- `SphConfig`: physical, domain, workload, and execution parameters.
- `SphParticle`: SPH particle state connected to `object_interface`.
- `SphFluid`: reproducible particle container and reset boundary.
- `SphMetrics`: liquid-specific metadata connected to `metrics_interface`.
- `SphScheme`: Strategy contract for sequential, fine-grained, coarse-grained,
  SMT, and CMP implementations.

The benchmark-only settings `iterations_per_config`, `profiling_tool`, and
`hardware_smt_enabled` are intentionally not part of `SphConfig`. They belong
to the future benchmark and experiment metadata layer.

## Build

```bash
cmake -S proyect_1-Multithreading/liquids -B build/liquids
cmake --build build/liquids
ctest --test-dir build/liquids --output-on-failure
```

The smoke test checks the object and metrics contracts. It does not claim that
the fluid simulation is physically implemented yet.

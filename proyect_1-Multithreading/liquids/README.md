# SPH liquids — interface scaffold

This directory contains the SPH model, stage contract, sequential baseline,
and independent execution strategies. Each strategy uses the same physical
pipeline so its stage timings can be compared directly.

## Current components

- `SphConfig`: physical, domain, workload, and execution parameters.
- `SphParticle`: SPH particle state connected to `object_interface`.
- `SphFluid`: reproducible particle container and reset boundary.
- `SphMetrics`: liquid-specific metadata connected to `metrics_interface`.
- `SphScheme`: Strategy contract for sequential, fine-grained, coarse-grained,
  SMT, and CMP implementations.
- `SequentialSphScheme`: complete sequential step with explicit neighbor
  search, density, pressure, force, and integration phases.
- `SphScheme::simulate`: repeats the step pipeline according to
  `SphConfig::getTimeSteps()` and returns the accumulated density-phase
  measurement.
- `SphStageRunner`: stage contract with explicit `initialize`, `runStage`,
  `runStep`, and `runAllSteps` operations.
- `strategies/fine_grained`: cooperative fine-grained execution using virtual
  workers, configurable quanta, and round-robin scheduling for every
  computational stage. It intentionally does not create operating-system
  threads; real-thread strategies belong to later SMT/CMP implementations.
- `SphFluid`: deterministic dam-break style initialization using a regular
  particle lattice.
- `SphForces`: normalized Poly6, Spiky-gradient, and viscosity kernels, with
  density/pressure limits, velocity limiting, and boundary damping.

The benchmark-only settings `iterations_per_config`, `profiling_tool`, and
`hardware_smt_enabled` are intentionally not part of `SphConfig`. They belong
to the future benchmark and experiment metadata layer.

## Build

```bash
cmake -S proyect_1-Multithreading/liquids -B build/liquids
cmake --build build/liquids
ctest --test-dir build/liquids --output-on-failure
```

The test checks the object and metrics contracts, deterministic initialization,
stability limits, boundary bounds, and equivalence between sequential and
fine-grained execution. The neighbor search is a distinct stage. The returned
`SphRunMetrics` contains wall time and execution count for every stage, so
benchmarks can report each phase without mixing neighbor-search overhead into
density.

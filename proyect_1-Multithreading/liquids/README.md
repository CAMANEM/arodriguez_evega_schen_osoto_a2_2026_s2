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
- `strategies/coarse_grained`: cooperative coarse-grained execution using
  real `std::thread` workers, static partitions, large chunks, `join()` stage
  barriers, and deterministic optional yields.
- `strategies/smt`: real `std::thread` execution with configurable
  oversubscription (`L * factor`).
- `strategies/cmp`: real `std::thread` execution with one worker per logical
  processor reported by the operating system (`L`).
- `SphFluid`: deterministic dam-break style initialization using a regular
  particle lattice.
- `SphForces`: normalized Poly6, Spiky-gradient, and viscosity kernels, with
  density/pressure limits, velocity limiting, and boundary damping.

The benchmark-only settings `iterations_per_config`, `profiling_tool`, and
`hardware_smt_enabled` are intentionally not part of `SphConfig`. They belong
to the future benchmark and experiment metadata layer.

SMT strategy selection does not toggle hardware SMT. The strategy controls
software oversubscription; the hardware SMT/Hyper-Threading state must be
recorded and changed externally through BIOS/UEFI or the Linux system
interface during the benchmark campaign.

## Build

```bash
cmake -S proyect_1-Multithreading/liquids -B build/liquids
cmake --build build/liquids
ctest --test-dir build/liquids --output-on-failure
```

The test checks the object and metrics contracts, deterministic initialization,
stability limits, boundary bounds, and equivalence between sequential,
fine-grained, coarse-grained, SMT, and CMP execution. The neighbor search is a
distinct stage. The returned `SphRunMetrics` contains wall time and execution
count for every stage, so benchmarks can report each phase without mixing
neighbor-search overhead into density.

## Scripts

From the repository root:

```bash
./proyect_1-Multithreading/scripts/run_liquids.sh test
./proyect_1-Multithreading/scripts/run_liquids.sh benchmark
./proyect_1-Multithreading/scripts/sweep_liquids.sh
```

The sweep varies coarse-grained real-thread counts and also runs SMT and CMP.
The scripts require CMake and a C++17 compiler on the target machine.

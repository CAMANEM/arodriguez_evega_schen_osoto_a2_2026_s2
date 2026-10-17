# Campaign contract — Multithreading Project 1

Shared CLI concepts and CSV columns for comparable campaigns across Boids,
Liquids, Ray Tracing, and N-Body. This phase unifies the **campaign interface**,
not Fine-Grained scheduler implementations.

## Canonical flags

| Concept | Meaning | Canonical flag | Aliases |
|---|---|---|---|
| Model | Execution scheme | `--model` | `--scheme`; `fgmt`→`fine`, `cgmt`→`coarse` |
| Workload | Problem size N | problem-specific | `--particles`, `--boids`, `-n`, `width×height` |
| Steps | Timesteps / frames per measured run | `--steps` | `-s` (N-Body) |
| Runs | Measured repetitions | `--runs` | `-e` (N-Body) |
| Warmup | Discarded cold-start runs | `--warmup` | (default `0`) |
| Workers | Coarse / virtual contexts | `--workers` | `--threads` |
| Quantum | Fine-grained quantum size | `--quantum` | Boids: 1 neighbor (fixed); `--partial` limits contexts |
| SMT factor | Software oversubscription `T = L × F` | `--smt-factor` | `--oversubscribe` |
| Seed | Deterministic input | `--seed` | |
| CSV summary | Aggregated stats | `--output` | |
| CSV raw | Per-run samples | `--raw-output` | |

Models: `sequential | fine | coarse | smt | cmp | all`.

## Model semantics

| Model | Campaign meaning |
|---|---|
| Sequential | Baseline, 1 worker |
| Fine | Interleaved quanta (see exceptions below) |
| Coarse | Real OS threads; context switch on costly stalls |
| SMT | Software oversubscription `T = L × F` (Ray: simulated issue slots) |
| CMP | `T ≈ L` (`hardware_concurrency`) |

### Fine-Grained exceptions (documented, intentional)

| Problem | Implementation in this repo | Campaign alignment |
|---|---|---|
| **Boids** | 1 OS thread + `SteeringContext` round-robin | Reference cooperative Fine |
| **Liquids** | 1 OS thread + virtual workers + `--quantum` | Reference cooperative Fine |
| **Ray Tracing** | Existing `FinegrainedRenderer` (N OS threads + semaphores) | **Do not rewrite**; expose CLI/metrics only |
| **N-Body** | Existing `step_fine` (`BodyContext` + token-ring OS threads) | **Do not rewrite**; expose CLI/metrics only |

## CSV columns

### Summary (`--output`)

```text
model,workload_n,steps,effective_workers,quantum,smt_factor,seed,runs,warmup,mean_s,stddev_s,ci95_lo,ci95_hi,speedup,efficiency
```

Problem-specific extras (e.g. Liquids `stage`, Ray virtual time) may append
columns after these.

### Raw (`--raw-output`)

```text
model,run_index,phase,workload_n,steps,effective_workers,seconds
```

`phase` is `warmup` or `measured`. Warmup rows are evidence of cold start and
are **not** included in mean/CI/speedup of the summary.

## Warmup / cold start

1. Run `--warmup N` iterations first (same workload and model).
2. Do not call `record_time` for warmup samples used in summary stats.
3. Optionally write warmup rows to raw CSV with `phase=warmup`.
4. Then run `--runs M` measured iterations with `phase=measured`.

## Binary roles

| Problem | Demo / interactive | Campaign benchmark |
|---|---|---|
| Boids | `boids` (`--scheme`, GUI, validate) | `boids_benchmark` |
| Liquids | — | `liquids_benchmark` |
| Ray | visual targets | `raytracing_benchmark` |
| N-Body | `n_body` short flags + GUI | same binary; long flags + optional campaign CSV |

Keep legacy N-Body CSV (`iteracion,particulas,...`) intact; campaign CSV is additive.

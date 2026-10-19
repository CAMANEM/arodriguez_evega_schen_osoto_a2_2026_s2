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

## Unified CMake build

Root file: `proyect_1-Multithreading/CMakeLists.txt`.

```bash
cmake -S proyect_1-Multithreading -B build/p1 -DCMAKE_BUILD_TYPE=Release
cmake --build build/p1 --config Release
```

On Windows without MSVC, use MinGW:

```powershell
cmake -S proyect_1-Multithreading -B build/p1 -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build/p1 --config Release
```

Campaign-oriented defaults at the root:

- `BOIDS_BUILD_VISUAL=OFF`, `BOIDS_BUILD_TESTS=OFF`
- `LIQUIDS_BUILD_TESTS=OFF`, `BUILD_TESTING=OFF`
- `NBODY_ENABLE=ON` (requires Raylib)

N-Body Raylib discovery: `find_package(raylib)`, else `-DNBODY_RAYLIB_ROOT=<prefix>`
(or env `RAYLIB_PATH` / `NBODY_RAYLIB_ROOT`), else FetchContent of raylib 5.5.
Windows Makefile default: `C:/raylib/w64devkit`. To skip N-Body:
`-DNBODY_ENABLE=OFF`.

Each problem remains a **separate process binary**, so a leak or unclean shutdown
in one cell cannot poison the next problem's measurements.

## Automated campaign (`campaign_all`)

Scripts:

- `scripts/campaign_all.ps1` (Windows)
- `scripts/campaign_all.sh` (Linux/macOS)

### Smoke (short)

```powershell
.\proyect_1-Multithreading\scripts\campaign_all.ps1 `
  -Runs 2 -Warmup 1 -Workloads 100 -Workers 2
```

```bash
RUNS=2 WARMUP=1 WORKLOADS=100 WORKERS=2 ./proyect_1-Multithreading/scripts/campaign_all.sh
```

### Full matrix (defaults)

- Runs=200, Warmup=5, Steps=10
- Workloads=`100,1000,10000`, Workers=`2,4,8`
- Models: `sequential` once per workload; `fine|coarse|smt|cmp` per worker

Optional filters: `-Problems boids,liquids` / `PROBLEMS=boids,liquids`.
Reuse binaries without rebuild: `-SkipBuild` / `SKIP_BUILD=1`.

### Sequential once per workload

Sequential wall time does not depend on `--workers` (always 1 worker). The
campaign therefore:

1. Runs `--model sequential` **once** per `(problem, workload)` with `workers=1`.
2. Replicates those summary/raw rows into every `n_<N>_w_<W>_*.csv` cell for that
   workload.
3. Runs only `fine`, `coarse`, `smt`, and `cmp` for each worker value.

This keeps CSVs comparable across workers without paying sequential cost
`× |Workers|`.

### Output layout

```text
proyect_1-Multithreading/data/campaign/<timestamp>/
  manifest.csv
  campaign_summary_all.csv   # leading column: problem
  campaign_raw_all.csv       # leading column: problem
  boids|liquids|ray|nbody/
    n_<N>_sequential_summary.csv   # measured once
    n_<N>_sequential_raw.csv
    n_<N>_w_<W>_summary.csv        # sequential rows + parallel models
    n_<N>_w_<W>_raw.csv
```

Ray workload mapping used by the script: `100→160×120`, `1000→240×160`,
`10000→480×320` (`--width` / `--height`).

## Linux `perf` profiling (headless)

Wraps the **same** Release binaries; no C++ changes. Meant for Arch/Linux
physical hosts (not WSL). Exports CSV and PNG without a GUI
(`MPLBACKEND=Agg` inside `plot_perf.py`).

### Scripts

| Script | Role |
|---|---|
| `scripts/run_perf_and_plot.sh` | One-shot: profile + plot |
| `scripts/perf_profile_all.sh` | `perf stat -x,` sweep + optional auto-plot |
| `scripts/plot_perf.py` | Parse raw `perf` CSV → `perf_counters.csv` + PNGs |

### Arch deps (once)

```bash
sudo pacman -S perf cmake base-devel python python-matplotlib python-pandas
sudo sysctl kernel.perf_event_paranoid=1
```

### Default matrix

- `RUNS=1`, `WARMUP=0`, `STEPS=10`
- `WORKLOADS=50,500,5000`, `WORKERS=2,4,8`
- Models: `sequential` once per workload; `fine|coarse|smt|cmp` per worker
- Events: `cycles,instructions,cache-references,cache-misses,context-switches,cpu-migrations,task-clock`

Cell count ≈ `4 problems × 3 workloads × (1 + 3×4) = 156` `perf` runs.

### Run

```bash
# First time (build + profile + figures)
./proyect_1-Multithreading/scripts/run_perf_and_plot.sh

# Reuse binaries
SKIP_BUILD=1 ./proyect_1-Multithreading/scripts/run_perf_and_plot.sh

# After disabling HT/SMT in BIOS (same suite, labeled off)
SKIP_BUILD=1 BIOS_LABEL=off ./proyect_1-Multithreading/scripts/run_perf_and_plot.sh
```

Re-plot only:

```bash
python3 proyect_1-Multithreading/scripts/plot_perf.py \
  --manifest proyect_1-Multithreading/data/perf/<ts>/manifest.csv \
  --outdir proyect_1-Multithreading/data/perf/<ts>
```

For copied campaigns, the manifest may retain the original Linux paths. The
plot script automatically falls back to `<campaign>/raw/<filename>` when an
absolute `perf_csv` path is unavailable, so the copied campaign can be
plotted in place without rerunning `perf`.

The plotting script uses Python's standard library plus `matplotlib`. Install
the direct dependency with:

```bash
python3 -m pip install -r proyect_1-Multithreading/requirements-perf.txt
```

### Output layout

```text
proyect_1-Multithreading/data/perf/<timestamp>/
  host_meta.txt          # lscpu / BIOS label
  manifest.csv
  perf_counters.csv      # cycles, ipc, cache_miss_rate, ...
  raw/*.perf.csv         # perf -x, per cell
  figures/*.png          # IPC, miss rate, context-switches, heatmaps
```

Set `BIOS_LABEL=on|off` so ON vs OFF campaigns stay distinguishable when
merged for the paper. This layer complements (does not replace) the 200-run
wall-time campaign.

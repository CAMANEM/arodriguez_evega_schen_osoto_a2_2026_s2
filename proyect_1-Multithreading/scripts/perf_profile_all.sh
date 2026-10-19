#!/usr/bin/env bash
# Profile all campaign binaries with Linux perf (CLI / headless).
# Does not modify the C++ code: wraps existing Release benchmarks.
#
# Defaults (override via env):
#   RUNS=1 WARMUP=0 STEPS=10
#   WORKLOADS=50,500,5000  WORKERS=2,4,8
#   Models: sequential once per workload; fine|coarse|smt|cmp per worker
#   BIOS_LABEL=on   # set to off after disabling HT/SMT in BIOS
#
# Arch Linux deps:
#   sudo pacman -S perf python python-matplotlib python-pandas
#   # if perf events are restricted:
#   sudo sysctl kernel.perf_event_paranoid=1
#
# Usage:
#   ./proyect_1-Multithreading/scripts/perf_profile_all.sh
#   SKIP_BUILD=1 BIOS_LABEL=off ./proyect_1-Multithreading/scripts/perf_profile_all.sh
#   WORKLOADS=50,500 WORKERS=8 PROBLEMS=boids,nbody SKIP_BUILD=1 ./...

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
P1_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${P1_ROOT}/.." && pwd)"

RUNS="${RUNS:-1}"
WARMUP="${WARMUP:-0}"
STEPS="${STEPS:-10}"
WORKERS="${WORKERS:-2,4,8}"
WORKLOADS="${WORKLOADS:-50,500,5000}"
SMT_FACTOR="${SMT_FACTOR:-2}"
SEED="${SEED:-42}"
QUANTUM="${QUANTUM:-8}"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build/p1}"
PROBLEMS="${PROBLEMS:-boids,liquids,ray,nbody}"
RAYLIB_ROOT="${RAYLIB_ROOT:-}"
SKIP_BUILD="${SKIP_BUILD:-0}"
OUTPUT_DIR="${OUTPUT_DIR:-}"
BIOS_LABEL="${BIOS_LABEL:-on}"
PERF_EVENTS="${PERF_EVENTS:-cycles,instructions,cache-references,cache-misses,context-switches,cpu-migrations,task-clock}"
AUTO_PLOT="${AUTO_PLOT:-1}"

die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
info() { printf '==> %s\n' "$*"; }

command -v perf >/dev/null 2>&1 || die "Missing 'perf'. On Arch: sudo pacman -S perf"
command -v cmake >/dev/null 2>&1 || die "Missing 'cmake'."
[[ "${RUNS}" -gt 0 && "${STEPS}" -gt 0 ]] || die "runs and steps must be positive."
[[ "${WARMUP}" -ge 0 ]] || die "warmup must be >= 0."

if [[ -z "${OUTPUT_DIR}" ]]; then
  stamp="$(date +%Y-%m-%d_%H-%M-%S)"
  OUTPUT_DIR="${P1_ROOT}/data/perf/${stamp}"
fi

RAW_DIR="${OUTPUT_DIR}/raw"
FIG_DIR="${OUTPUT_DIR}/figures"
BENCH_DIR="${RAW_DIR}/bench_side"
mkdir -p "${RAW_DIR}" "${FIG_DIR}" "${BENCH_DIR}"

MANIFEST="${OUTPUT_DIR}/manifest.csv"
printf 'problem,workload_n,workers,model,bios,smt_factor,seed,steps,runs,warmup,perf_csv\n' \
  > "${MANIFEST}"

META="${OUTPUT_DIR}/host_meta.txt"
{
  printf 'timestamp=%s\n' "$(date -Iseconds)"
  printf 'bios_label=%s\n' "${BIOS_LABEL}"
  printf 'hostname=%s\n' "$(hostname 2>/dev/null || echo unknown)"
  printf 'kernel=%s\n' "$(uname -r 2>/dev/null || echo unknown)"
  if command -v lscpu >/dev/null 2>&1; then
    printf '\n--- lscpu ---\n'
    lscpu
  fi
  if [[ -r /proc/cpuinfo ]]; then
    printf '\n--- /proc/cpuinfo (model name) ---\n'
    grep -m1 'model name' /proc/cpuinfo || true
  fi
} > "${META}"

IFS=',' read -r -a WORKER_ARR <<< "${WORKERS}"
IFS=',' read -r -a WORKLOAD_ARR <<< "${WORKLOADS}"
IFS=',' read -r -a PROBLEM_ARR <<< "${PROBLEMS}"

has_problem() {
  local want="$1"
  local p
  for p in "${PROBLEM_ARR[@]}"; do
    [[ "$(echo "${p}" | tr '[:upper:]' '[:lower:]' | xargs)" == "${want}" ]] && return 0
  done
  return 1
}

find_exe() {
  local name="$1"
  local cand
  for cand in \
    "${BUILD_DIR}/${name}" \
    "${BUILD_DIR}/${name}.exe" \
    "${BUILD_DIR}/Release/${name}.exe" \
    "${BUILD_DIR}/Boids/${name}" \
    "${BUILD_DIR}/Boids/${name}.exe" \
    "${BUILD_DIR}/Boids/Release/${name}.exe" \
    "${BUILD_DIR}/liquids/${name}" \
    "${BUILD_DIR}/liquids/${name}.exe" \
    "${BUILD_DIR}/liquids/Release/${name}.exe" \
    "${BUILD_DIR}/ray_traycing/${name}" \
    "${BUILD_DIR}/ray_traycing/${name}.exe" \
    "${BUILD_DIR}/ray_traycing/Release/${name}.exe" \
    "${BUILD_DIR}/n_body/${name}" \
    "${BUILD_DIR}/n_body/${name}.exe" \
    "${BUILD_DIR}/n_body/Release/${name}.exe"
  do
    if [[ -f "${cand}" ]]; then
      printf '%s\n' "${cand}"
      return 0
    fi
  done
  return 1
}

ray_resolution() {
  case "$1" in
    50)    echo "64 64" ;;
    100)   echo "160 120" ;;
    500)   echo "128 128" ;;
    1000)  echo "240 160" ;;
    5000)  echo "320 240" ;;
    10000) echo "480 320" ;;
    *)
      local side
      side=$(awk -v n="$1" 'BEGIN { s=int(sqrt(n)); if (s < 16) s=16; print s }')
      echo "${side} ${side}"
      ;;
  esac
}

# Run one (problem, model, n, w) cell under perf stat → CSV.
run_perf_cell() {
  local problem="$1"
  local exe="$2"
  local model="$3"
  local n="$4"
  local w="$5"
  local tag="${problem}_n${n}_w${w}_${model}"
  local out_csv="${RAW_DIR}/${tag}.perf.csv"
  local summary="${BENCH_DIR}/${tag}_summary.csv"
  local raw="${BENCH_DIR}/${tag}_raw.csv"
  local legacy="${BENCH_DIR}/${tag}_legacy.csv"
  local width height
  local rc=0

  info "${problem} n=${n} w=${w} model=${model} bios=${BIOS_LABEL}"

  case "${problem}" in
    boids)
      perf stat -x, -o "${out_csv}" -e "${PERF_EVENTS}" -- \
        "${exe}" --model "${model}" --boids "${n}" --steps "${STEPS}" \
        --workers "${w}" --smt-factor "${SMT_FACTOR}" --runs "${RUNS}" \
        --warmup "${WARMUP}" --seed "${SEED}" \
        --output "${summary}" --raw-output "${raw}" \
        >/dev/null || rc=$?
      ;;
    liquids)
      perf stat -x, -o "${out_csv}" -e "${PERF_EVENTS}" -- \
        "${exe}" --model "${model}" --particles "${n}" --steps "${STEPS}" \
        --workers "${w}" --quantum "${QUANTUM}" --chunk 64 \
        --smt-factor "${SMT_FACTOR}" --runs "${RUNS}" --warmup "${WARMUP}" \
        --seed "${SEED}" --output "${summary}" --raw-output "${raw}" \
        >/dev/null || rc=$?
      ;;
    ray)
      read -r width height < <(ray_resolution "${n}")
      perf stat -x, -o "${out_csv}" -e "${PERF_EVENTS}" -- \
        "${exe}" --model "${model}" --runs "${RUNS}" --warmup "${WARMUP}" \
        --workers "${w}" --width "${width}" --height "${height}" --seed "${SEED}" \
        --smt-factor "${SMT_FACTOR}" --quantum "${QUANTUM}" \
        --output "${summary}" --raw-output "${raw}" \
        >/dev/null || rc=$?
      ;;
    nbody)
      perf stat -x, -o "${out_csv}" -e "${PERF_EVENTS}" -- \
        "${exe}" -headless --model "${model}" -n "${n}" --steps "${STEPS}" \
        --runs "${RUNS}" --warmup "${WARMUP}" --workers "${w}" \
        --smt-factor "${SMT_FACTOR}" --seed "${SEED}" \
        --output "${summary}" --raw-output "${raw}" -o "${legacy}" \
        >/dev/null || rc=$?
      ;;
    *)
      die "Unknown problem: ${problem}"
      ;;
  esac

  if [[ "${rc}" -ne 0 ]]; then
    die "perf/benchmark failed for ${tag} (exit ${rc}); see ${out_csv}"
  fi
  [[ -s "${out_csv}" ]] || die "Empty perf CSV for ${tag}"

  printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
    "${problem}" "${n}" "${w}" "${model}" "${BIOS_LABEL}" \
    "${SMT_FACTOR}" "${SEED}" "${STEPS}" "${RUNS}" "${WARMUP}" "${out_csv}" \
    >> "${MANIFEST}"
}

if [[ "${SKIP_BUILD}" != "1" ]]; then
  nbody_enable=OFF
  has_problem nbody && nbody_enable=ON

  cmake_args=(
    -S "${P1_ROOT}"
    -B "${BUILD_DIR}"
    -DCMAKE_BUILD_TYPE=Release
    -DBOIDS_BUILD_VISUAL=OFF
    -DBOIDS_BUILD_TESTS=OFF
    -DLIQUIDS_BUILD_TESTS=OFF
    -DBUILD_TESTING=OFF
    -DNBODY_ENABLE="${nbody_enable}"
  )
  if [[ -n "${RAYLIB_ROOT}" ]]; then
    cmake_args+=(-DNBODY_RAYLIB_ROOT="${RAYLIB_ROOT}")
  fi

  info "Configuring unified CMake (${BUILD_DIR})"
  cmake "${cmake_args[@]}"
  info "Building Release"
  cmake --build "${BUILD_DIR}" --config Release \
    -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
fi

BIN_BOIDS=""
BIN_LIQUIDS=""
BIN_RAY=""
BIN_NBODY=""

if has_problem boids; then
  BIN_BOIDS="$(find_exe boids_benchmark)" || die "boids_benchmark not found under ${BUILD_DIR}"
fi
if has_problem liquids; then
  BIN_LIQUIDS="$(find_exe liquids_benchmark)" || die "liquids_benchmark not found under ${BUILD_DIR}"
fi
if has_problem ray; then
  BIN_RAY="$(find_exe raytracing_benchmark)" || die "raytracing_benchmark not found under ${BUILD_DIR}"
fi
if has_problem nbody; then
  BIN_NBODY="$(find_exe n_body)" || die "n_body not found under ${BUILD_DIR} (Raylib required)"
fi

bin_for_problem() {
  case "$1" in
    boids) printf '%s\n' "${BIN_BOIDS}" ;;
    liquids) printf '%s\n' "${BIN_LIQUIDS}" ;;
    ray) printf '%s\n' "${BIN_RAY}" ;;
    nbody) printf '%s\n' "${BIN_NBODY}" ;;
  esac
}

PARALLEL_MODELS=(fine coarse smt cmp)

for problem in boids liquids ray nbody; do
  has_problem "${problem}" || continue
  exe="$(bin_for_problem "${problem}")"

  for N in "${WORKLOAD_ARR[@]}"; do
    # Sequential is independent of worker count: profile once per workload.
    run_perf_cell "${problem}" "${exe}" sequential "${N}" 1

    for W in "${WORKER_ARR[@]}"; do
      for model in "${PARALLEL_MODELS[@]}"; do
        run_perf_cell "${problem}" "${exe}" "${model}" "${N}" "${W}"
      done
    done
  done
done

info "Profiling finished. Output: ${OUTPUT_DIR}"
printf '  Manifest: %s\n' "${MANIFEST}"
printf '  Host meta: %s\n' "${META}"

if [[ "${AUTO_PLOT}" == "1" ]]; then
  if command -v python3 >/dev/null 2>&1; then
    info "Generating consolidated CSV + figures"
    python3 "${SCRIPT_DIR}/plot_perf.py" \
      --manifest "${MANIFEST}" \
      --outdir "${OUTPUT_DIR}" \
      --figures "${FIG_DIR}"
  else
    printf 'WARN: python3 not found; skip plot. Run plot_perf.py later.\n' >&2
  fi
fi

printf 'Done.\n'
printf '  Counters CSV: %s/perf_counters.csv\n' "${OUTPUT_DIR}"
printf '  Figures:      %s\n' "${FIG_DIR}"
printf 'Tip: re-run with BIOS_LABEL=off after disabling HT/SMT for the hardware SMT contrast.\n'

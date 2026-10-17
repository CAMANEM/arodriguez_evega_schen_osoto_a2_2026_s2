#!/usr/bin/env bash
# Build the unified multithreading tree and run a comparable campaign across
# Boids, Liquids, Ray Tracing, and N-Body.
#
# Usage examples:
#   ./scripts/campaign_all.sh
#   RUNS=2 WARMUP=1 WORKLOADS=100 WORKERS=2 ./scripts/campaign_all.sh
#   PROBLEMS=boids,liquids ./scripts/campaign_all.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
P1_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
REPO_ROOT="$(cd "${P1_ROOT}/.." && pwd)"

RUNS="${RUNS:-200}"
WARMUP="${WARMUP:-5}"
STEPS="${STEPS:-10}"
WORKERS="${WORKERS:-2,4,8}"
WORKLOADS="${WORKLOADS:-100,1000,10000}"
SMT_FACTOR="${SMT_FACTOR:-2}"
SEED="${SEED:-42}"
QUANTUM="${QUANTUM:-8}"
BUILD_DIR="${BUILD_DIR:-${REPO_ROOT}/build/p1}"
PROBLEMS="${PROBLEMS:-boids,liquids,ray,nbody}"
RAYLIB_ROOT="${RAYLIB_ROOT:-}"
SKIP_BUILD="${SKIP_BUILD:-0}"
OUTPUT_DIR="${OUTPUT_DIR:-}"

die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
info() { printf '==> %s\n' "$*"; }

command -v cmake >/dev/null 2>&1 || die "Missing 'cmake'."
[[ "${RUNS}" -gt 0 && "${STEPS}" -gt 0 ]] || die "runs and steps must be positive."
[[ "${WARMUP}" -ge 0 ]] || die "warmup must be >= 0."

if [[ -z "${OUTPUT_DIR}" ]]; then
  stamp="$(date +%Y-%m-%d_%H-%M-%S)"
  OUTPUT_DIR="${P1_ROOT}/data/campaign/${stamp}"
fi

mkdir -p "${OUTPUT_DIR}"
SUMMARY_ALL="${OUTPUT_DIR}/campaign_summary_all.csv"
RAW_ALL="${OUTPUT_DIR}/campaign_raw_all.csv"
MANIFEST="${OUTPUT_DIR}/manifest.csv"
printf 'problem,workload_n,workers,summary_csv,raw_csv\n' > "${MANIFEST}"

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
    # Prefer regular files: ${BUILD_DIR}/n_body is a directory, not the binary.
    if [[ -f "${cand}" ]]; then
      printf '%s\n' "${cand}"
      return 0
    fi
  done
  return 1
}

ray_resolution() {
  case "$1" in
    100)  echo "160 120" ;;
    1000) echo "240 160" ;;
    10000) echo "480 320" ;;
    *)
      local side
      side=$(awk -v n="$1" 'BEGIN { s=int(sqrt(n)); if (s < 16) s=16; print s }')
      echo "${side} ${side}"
      ;;
  esac
}

# Prepend problem column and append into merged CSV.
# Args: source_csv problem dest_csv header_flag_varname
merge_csv_with_problem() {
  local source="$1"
  local problem="$2"
  local dest="$3"
  local flag_var="$4"
  [[ -f "${source}" ]] || return 0
  local header
  header="$(head -n 1 "${source}")"
  if [[ "$(eval "echo \${${flag_var}}")" != "1" ]]; then
    printf 'problem,%s\n' "${header}" > "${dest}"
    eval "${flag_var}=1"
  fi
  tail -n +2 "${source}" | while IFS= read -r line || [[ -n "${line}" ]]; do
    [[ -z "${line}" ]] && continue
    printf '%s,%s\n' "${problem}" "${line}"
  done >> "${dest}"
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

summary_header_written=0
raw_header_written=0
NBODY_MODELS=(sequential fine coarse smt cmp)

for problem in boids liquids ray nbody; do
  has_problem "${problem}" || continue
  problem_dir="${OUTPUT_DIR}/${problem}"
  mkdir -p "${problem_dir}"

  for N in "${WORKLOAD_ARR[@]}"; do
    for W in "${WORKER_ARR[@]}"; do
      prefix="${problem_dir}/n_${N}_w_${W}"
      summary="${prefix}_summary.csv"
      raw="${prefix}_raw.csv"
      info "${problem} workload=${N} workers=${W} runs=${RUNS} warmup=${WARMUP} steps=${STEPS}"

      case "${problem}" in
        boids)
          "${BIN_BOIDS}" --model all --boids "${N}" --steps "${STEPS}" --workers "${W}" \
            --smt-factor "${SMT_FACTOR}" --runs "${RUNS}" --warmup "${WARMUP}" \
            --seed "${SEED}" --output "${summary}" --raw-output "${raw}"
          ;;
        liquids)
          "${BIN_LIQUIDS}" --model all --particles "${N}" --steps "${STEPS}" --workers "${W}" \
            --quantum "${QUANTUM}" --chunk 64 --smt-factor "${SMT_FACTOR}" \
            --runs "${RUNS}" --warmup "${WARMUP}" --seed "${SEED}" \
            --output "${summary}" --raw-output "${raw}"
          ;;
        ray)
          read -r width height < <(ray_resolution "${N}")
          "${BIN_RAY}" --model all --runs "${RUNS}" --warmup "${WARMUP}" --workers "${W}" \
            --width "${width}" --height "${height}" --seed "${SEED}" \
            --smt-factor "${SMT_FACTOR}" --quantum "${QUANTUM}" \
            --output "${summary}" --raw-output "${raw}"
          ;;
        nbody)
          : > "${summary}.parts"
          : > "${raw}.parts"
          first_summary=1
          first_raw=1
          for model in "${NBODY_MODELS[@]}"; do
            ms="${prefix}_${model}_summary.csv"
            mr="${prefix}_${model}_raw.csv"
            legacy="${prefix}_${model}_legacy.csv"
            "${BIN_NBODY}" -headless --model "${model}" -n "${N}" --steps "${STEPS}" \
              --runs "${RUNS}" --warmup "${WARMUP}" --workers "${W}" \
              --smt-factor "${SMT_FACTOR}" --seed "${SEED}" \
              --output "${ms}" --raw-output "${mr}" -o "${legacy}"
            if [[ "${first_summary}" -eq 1 ]]; then
              head -n 1 "${ms}" > "${summary}"
              first_summary=0
            fi
            tail -n +2 "${ms}" >> "${summary}"
            if [[ "${first_raw}" -eq 1 ]]; then
              head -n 1 "${mr}" > "${raw}"
              first_raw=0
            fi
            tail -n +2 "${mr}" >> "${raw}"
          done
          rm -f "${summary}.parts" "${raw}.parts"
          ;;
      esac

      merge_csv_with_problem "${summary}" "${problem}" "${SUMMARY_ALL}" summary_header_written
      merge_csv_with_problem "${raw}" "${problem}" "${RAW_ALL}" raw_header_written
      printf '%s,%s,%s,%s,%s\n' "${problem}" "${N}" "${W}" "${summary}" "${raw}" \
        >> "${MANIFEST}"
    done
  done
done

printf 'Campaign completed.\n'
printf '  Output:  %s\n' "${OUTPUT_DIR}"
printf '  Summary: %s\n' "${SUMMARY_ALL}"
printf '  Raw:     %s\n' "${RAW_ALL}"

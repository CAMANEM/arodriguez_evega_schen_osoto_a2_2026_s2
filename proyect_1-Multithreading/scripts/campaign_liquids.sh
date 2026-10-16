#!/usr/bin/env bash
# Run the reproducible Liquids performance matrix.
#
# Defaults: 100, 1000, and 10000 particles; 2, 4, and 8 workers; 200 runs.
# The benchmark uses 10 steps by default so the campaign remains practical.
#
# Usage:
#   ./scripts/campaign_liquids.sh [runs] [steps] [output_dir]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
SOURCE="${ROOT}/proyect_1-Multithreading/liquids"
BUILD_DIR="${ROOT}/build/liquids"
RUNS="${1:-200}"
STEPS="${2:-10}"
OUTPUT_DIR="${3:-${ROOT}/proyect_1-Multithreading/liquids/data/campaign}"
BIN="${BUILD_DIR}/liquids_benchmark"

die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
command -v cmake >/dev/null 2>&1 || die "Missing 'cmake'."
[[ "${RUNS}" -gt 0 && "${STEPS}" -gt 0 ]] || die "runs and steps must be positive."

cmake -S "${SOURCE}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${BUILD_DIR}" --config Release \
  -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
ctest --test-dir "${BUILD_DIR}" --output-on-failure
[[ -x "${BIN}" ]] || die "Benchmark binary not found: ${BIN}"

mkdir -p "${OUTPUT_DIR}"
printf 'particles,threads,summary_csv,raw_csv\n' \
  > "${OUTPUT_DIR}/manifest.csv"

for PARTICLES in 100 1000 10000; do
  for THREADS in 2 4 8; do
    PREFIX="${OUTPUT_DIR}/particles_${PARTICLES}_threads_${THREADS}"
    SUMMARY="${PREFIX}_summary.csv"
    RAW="${PREFIX}_raw.csv"
    printf '==> particles=%s threads=%s runs=%s steps=%s\n' \
      "${PARTICLES}" "${THREADS}" "${RUNS}" "${STEPS}"
    "${BIN}" --model all --particles "${PARTICLES}" --steps "${STEPS}" \
      --threads "${THREADS}" --quantum 8 --chunk 64 --smt-factor 2 \
      --runs "${RUNS}" --output "${SUMMARY}" --raw-output "${RAW}"
    printf '%s,%s,%s,%s\n' "${PARTICLES}" "${THREADS}" \
      "${SUMMARY}" "${RAW}" >> "${OUTPUT_DIR}/manifest.csv"
  done
done

printf 'Campaign completed. Results: %s\n' "${OUTPUT_DIR}"

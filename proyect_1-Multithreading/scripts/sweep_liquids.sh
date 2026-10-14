#!/usr/bin/env bash
# Run a small scalability sweep for real-thread coarse, SMT, and CMP.
#
# Usage:
#   ./scripts/sweep_liquids.sh [benchmark_binary] [runs] [particles] [steps]

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${1:-${ROOT}/build/liquids/liquids_benchmark}"
RUNS="${2:-5}"
PARTICLES="${3:-1000}"
STEPS="${4:-10}"
OUTPUT_DIR="${ROOT}/proyect_1-Multithreading/liquids/data/sweeps"

if [[ ! -x "${BIN}" ]]; then
  echo "Benchmark binary not found: ${BIN}" >&2
  echo "Build it with ./scripts/run_liquids.sh build" >&2
  exit 1
fi

mkdir -p "${OUTPUT_DIR}"
for THREADS in 1 2 4 8; do
  OUTPUT="${OUTPUT_DIR}/coarse_${THREADS}threads.csv"
  echo "==> coarse threads=${THREADS}"
  "${BIN}" --model coarse --particles "${PARTICLES}" --steps "${STEPS}" \
    --threads "${THREADS}" --chunk 64 --runs "${RUNS}" --output "${OUTPUT}"
done

echo "==> SMT"
"${BIN}" --model smt --particles "${PARTICLES}" --steps "${STEPS}" \
  --smt-factor 2 --runs "${RUNS}" \
  --output "${OUTPUT_DIR}/smt.csv"

echo "==> CMP"
"${BIN}" --model cmp --particles "${PARTICLES}" --steps "${STEPS}" \
  --runs "${RUNS}" --output "${OUTPUT_DIR}/cmp.csv"

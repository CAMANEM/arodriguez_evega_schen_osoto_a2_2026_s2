#!/usr/bin/env bash
# Build, test, and run the SPH benchmark.
#
# Usage:
#   ./scripts/run_liquids.sh build
#   ./scripts/run_liquids.sh test
#   ./scripts/run_liquids.sh benchmark
#   ./scripts/run_liquids.sh campaign
#   ./scripts/run_liquids.sh all

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
LIQUIDS_SRC="${REPO_ROOT}/proyect_1-Multithreading/liquids"
BUILD_DIR="${REPO_ROOT}/build/liquids"
MODE="${1:-benchmark}"
BIN="${BUILD_DIR}/liquids_benchmark"

info() { printf '==> %s\n' "$*"; }
die() { printf 'ERROR: %s\n' "$*" >&2; exit 1; }
need() { command -v "$1" >/dev/null 2>&1 || die "Missing '$1'."; }

configure_and_build() {
  need cmake
  info "Configuring Liquids in ${BUILD_DIR}"
  cmake -S "${LIQUIDS_SRC}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  info "Building Liquids..."
  cmake --build "${BUILD_DIR}" --config Release \
    -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 4)"
}

find_binary() {
  if [[ -x "${BUILD_DIR}/liquids_benchmark" ]]; then
    printf '%s\n' "${BUILD_DIR}/liquids_benchmark"
  elif [[ -x "${BUILD_DIR}/Release/liquids_benchmark" ]]; then
    printf '%s\n' "${BUILD_DIR}/Release/liquids_benchmark"
  else
    return 1
  fi
}

run_tests() {
  info "Running CTest..."
  ctest --test-dir "${BUILD_DIR}" --output-on-failure
}

run_benchmark() {
  local benchmark
  benchmark="$(find_binary)" || die "liquids_benchmark was not found."
  mkdir -p "${REPO_ROOT}/proyect_1-Multithreading/liquids/data"
  (
    cd "${REPO_ROOT}"
    "${benchmark}" --model all --particles 1000 --steps 10 \
      --threads 4 --quantum 8 --chunk 64 --smt-factor 2 --runs 5 \
    --output proyect_1-Multithreading/liquids/data/benchmark.csv
  )
}

case "${MODE}" in
  build)
    configure_and_build
    ;;
  test)
    configure_and_build
    run_tests
    ;;
  benchmark)
    configure_and_build
    run_tests
    run_benchmark
    ;;
  campaign)
    "${SCRIPT_DIR}/campaign_liquids.sh"
    ;;
  all)
    configure_and_build
    run_tests
    run_benchmark
    ;;
  *)
    printf 'Usage: %s [build|test|benchmark|campaign|all]\n' "$0" >&2
    exit 1
    ;;
esac

info "Liquids ${MODE} completed."

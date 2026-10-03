#!/usr/bin/env bash
# Compila y ejecuta la Demo 2 de Boids (ejecutable unificado `boids`).
#
# Uso:
#   ./scripts/run_demo2.sh                  # build + tests + compare (sin UI)
#   ./scripts/run_demo2.sh all              # + UI secuencial y CMP
#   ./scripts/run_demo2.sh benchmark        # --scheme compare
#   ./scripts/run_demo2.sh sequential       # headless infinito secuencial
#   ./scripts/run_demo2.sh cmp              # headless infinito CMP
#   ./scripts/run_demo2.sh sequential-ui    # --scheme sequential --gui
#   ./scripts/run_demo2.sh cmp-ui           # --scheme cmp --gui
#   ./scripts/run_demo2.sh build|test

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BOIDS_SRC="${REPO_ROOT}/proyect_1-Multithreading/Boids"
BUILD_DIR="${REPO_ROOT}/build/boids"
MODE="${1:-benchmark}"

info() { printf '==> %s\n' "$*"; }
die()  { printf 'ERROR: %s\n' "$*" >&2; exit 1; }

need() { command -v "$1" >/dev/null 2>&1 || die "Falta '$1'. Ejecute scripts/setup_linux.sh primero."; }

configure_and_build() {
  need cmake
  if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
    die "Falta compilador C++. Ejecute scripts/setup_linux.sh primero."
  fi

  info "Configurando CMake en ${BUILD_DIR}"
  cmake -S "${BOIDS_SRC}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release -DBOIDS_BUILD_VISUAL=ON
  info "Compilando Release..."
  cmake --build "${BUILD_DIR}" --config Release -j"$(nproc 2>/dev/null || echo 4)"
}

run_tests() {
  info "Ejecutando ctest..."
  ctest --test-dir "${BUILD_DIR}" -C Release --output-on-failure
}

find_bin() {
  local name="$1"
  if [[ -x "${BUILD_DIR}/${name}" ]]; then
    echo "${BUILD_DIR}/${name}"
  elif [[ -x "${BUILD_DIR}/Release/${name}" ]]; then
    echo "${BUILD_DIR}/Release/${name}"
  else
    return 1
  fi
}

run_boids() {
  local bin
  bin="$(find_bin boids)" || die "No se encontro boids. Compile primero."
  (
    cd "${REPO_ROOT}"
    "${bin}" "$@"
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
  benchmark|no-ui|compare)
    configure_and_build
    run_tests
    info "boids --scheme compare"
    run_boids --scheme compare
    ;;
  sequential)
    configure_and_build
    info "boids --scheme sequential --forever"
    run_boids --scheme sequential --forever
    ;;
  cmp)
    configure_and_build
    info "boids --scheme cmp --forever"
    run_boids --scheme cmp --forever
    ;;
  sequential-ui|ui|visual)
    configure_and_build
    info "boids --scheme sequential --gui"
    run_boids --scheme sequential --gui
    ;;
  cmp-ui)
    configure_and_build
    info "boids --scheme cmp --gui"
    run_boids --scheme cmp --gui
    ;;
  all)
    configure_and_build
    run_tests
    run_boids --scheme compare
    info "UI secuencial (cierre la ventana para continuar)..."
    run_boids --scheme sequential --gui || true
    info "UI CMP dummy..."
    run_boids --scheme cmp --gui || true
    ;;
  *)
    cat <<EOF
Uso: $0 [build|test|benchmark|sequential|cmp|sequential-ui|cmp-ui|all]
EOF
    exit 1
    ;;
esac

info "Listo (${MODE})."

#!/usr/bin/env bash
# Barrido de --oversubscribe para la aproximación SMT de Boids.
# Uso: ./sweep_smt_oversubscribe.sh [ruta/al/boids] [boids] [steps]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${1:-$ROOT/build/boids/boids}"
BOIDS="${2:-200}"
STEPS="${3:-50}"
SEED="${4:-42}"

if [[ ! -x "$BIN" ]]; then
  echo "No se encontró el binario: $BIN" >&2
  echo "Compile primero (cmake --build build/boids)." >&2
  exit 1
fi

echo "bin=$BIN boids=$BOIDS steps=$STEPS seed=$SEED"
echo "scheme,F,exit"
for F in 1 2 4 8; do
  echo "==> smt oversubscribe=$F"
  "$BIN" --scheme smt --oversubscribe "$F" --no-gui \
    --boids "$BOIDS" --steps "$STEPS" --seed "$SEED" --validate
  echo "smt,$F,$?"
done

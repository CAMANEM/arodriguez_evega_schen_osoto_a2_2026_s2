#!/usr/bin/env bash
# Barrido de --boids para CMP (T = L automático).
# Uso: ./sweep_cmp_boids.sh [ruta/al/boids] [steps] [seed]
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BIN="${1:-$ROOT/build/boids/boids}"
STEPS="${2:-50}"
SEED="${3:-42}"

if [[ ! -x "$BIN" ]]; then
  echo "No se encontró el binario: $BIN" >&2
  echo "Compile primero (cmake --build build/boids)." >&2
  exit 1
fi

echo "bin=$BIN steps=$STEPS seed=$SEED (scheme=cmp, T=L)"
echo "scheme,boids,exit"
for N in 50 100 200 400; do
  echo "==> cmp boids=$N"
  "$BIN" --scheme cmp --no-gui \
    --boids "$N" --steps "$STEPS" --seed "$SEED" --validate
  echo "cmp,$N,$?"
done

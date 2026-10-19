#!/usr/bin/env bash
# One-shot: profile with perf + generate CSV/PNG figures (Arch Linux / headless).
#
# Default matrix:
#   RUNS=1 WARMUP=0 STEPS=10
#   WORKLOADS=50,500,5000  WORKERS=2,4,8
#   problems: boids,liquids,ray,nbody
#   models: sequential (1x/workload) + fine,coarse,smt,cmp per worker
#
# Arch setup (once):
#   sudo pacman -S perf cmake base-devel python python-matplotlib python-pandas
#   sudo sysctl kernel.perf_event_paranoid=1
#
# Examples:
#   ./proyect_1-Multithreading/scripts/run_perf_and_plot.sh
#   SKIP_BUILD=1 ./proyect_1-Multithreading/scripts/run_perf_and_plot.sh
#   SKIP_BUILD=1 BIOS_LABEL=off ./proyect_1-Multithreading/scripts/run_perf_and_plot.sh

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export AUTO_PLOT="${AUTO_PLOT:-1}"

exec "${SCRIPT_DIR}/perf_profile_all.sh" "$@"

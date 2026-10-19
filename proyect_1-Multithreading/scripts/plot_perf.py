#!/usr/bin/env python3
"""Consolidate perf CSV cells and emit headless PNG figures (Agg backend).

Reads the manifest produced by perf_profile_all.sh, parses each
`perf stat -x,` file, writes perf_counters.csv, and saves plots under --figures.

The raw CSV paths in a copied campaign may still point to the original Linux
workspace. When such a path is unavailable, this script resolves the file by
its basename under the manifest's sibling `raw/` directory.

Usage:
  python3 scripts/plot_perf.py --manifest data/perf/<ts>/manifest.csv \\
      --outdir data/perf/<ts> --figures data/perf/<ts>/figures
"""

from __future__ import annotations

import argparse
import csv
import math
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

EVENT_ALIASES = {
    "cycles": "cycles",
    "instructions": "instructions",
    "cache-references": "cache_references",
    "cache-misses": "cache_misses",
    "context-switches": "context_switches",
    "cpu-migrations": "cpu_migrations",
    "task-clock": "task_clock",
}

MODEL_ORDER = ["sequential", "fine", "coarse", "smt", "cmp"]
MODEL_COLORS = {
    "sequential": "#555555",
    "fine": "#1f77b4",
    "coarse": "#ff7f0e",
    "smt": "#2ca02c",
    "cmp": "#d62728",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Plot Linux perf campaign results.")
    parser.add_argument("--manifest", required=True, type=Path, help="manifest.csv path")
    parser.add_argument(
        "--outdir",
        required=True,
        type=Path,
        help="directory for perf_counters.csv",
    )
    parser.add_argument(
        "--figures",
        type=Path,
        default=None,
        help="directory for PNG figures (default: <outdir>/figures)",
    )
    return parser.parse_args()


def _normalize_event(name: str) -> str | None:
    base = name.strip().lower()
    # Strip PMU / modifier suffixes: cycles:u, cpu-cycles, etc.
    base = base.split(":")[0]
    base = base.replace("cpu-", "")
    if base in EVENT_ALIASES:
        return EVENT_ALIASES[base]
    for key, alias in EVENT_ALIASES.items():
        if key in base or base in key:
            return alias
    return None


def _parse_number(raw: str) -> float | None:
    text = (raw or "").strip().replace(",", "")
    if not text or text in {"<not counted>", "<not supported>", "nan", "NaN"}:
        return None
    # task-clock often appears as "123.45" with unit in next column
    try:
        return float(text)
    except ValueError:
        return None


def resolve_perf_csv(manifest_path: Path, raw_path: str) -> Path:
    """Resolve a manifest path, including paths from a copied Linux campaign."""
    path = Path(raw_path)
    if path.is_file():
        return path

    copied_path = manifest_path.parent / "raw" / path.name
    if copied_path.is_file():
        return copied_path

    raise FileNotFoundError(
        f"perf CSV not found: {raw_path} "
        f"(also checked {copied_path})"
    )


def parse_perf_stat_csv(path: Path) -> dict[str, float]:
    """Parse `perf stat -x,` output into a flat event → value map."""
    values: dict[str, float] = {}
    if not path.is_file():
        return values

    with path.open(encoding="utf-8", errors="replace") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = next(csv.reader([line]))
            if len(parts) < 3:
                continue
            value = _parse_number(parts[0])
            event_field = parts[2] if len(parts) > 2 else ""
            # Some older/newer layouts put the event in column 3 with empty unit
            # in column 2; occasionally value is in col0 and event still col2.
            alias = _normalize_event(event_field)
            if alias is None and len(parts) > 1:
                alias = _normalize_event(parts[1])
            if alias is None or value is None:
                continue
            values[alias] = value
    return values


def load_manifest(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def consolidate(
    rows: list[dict[str, str]], manifest_path: Path
) -> list[dict[str, object]]:
    consolidated: list[dict[str, object]] = []
    for row in rows:
        perf_path = resolve_perf_csv(manifest_path, row["perf_csv"])
        events = parse_perf_stat_csv(perf_path)
        cycles = events.get("cycles")
        instructions = events.get("instructions")
        refs = events.get("cache_references")
        misses = events.get("cache_misses")

        ipc = (
            (instructions / cycles)
            if cycles and instructions and cycles > 0
            else math.nan
        )
        miss_rate = (
            (misses / refs) if refs and misses is not None and refs > 0 else math.nan
        )

        consolidated.append(
            {
                "problem": row["problem"],
                "workload_n": int(row["workload_n"]),
                "workers": int(row["workers"]),
                "model": row["model"],
                "bios": row["bios"],
                "smt_factor": row.get("smt_factor", ""),
                "seed": row.get("seed", ""),
                "steps": row.get("steps", ""),
                "runs": row.get("runs", ""),
                "warmup": row.get("warmup", ""),
                "cycles": events.get("cycles", math.nan),
                "instructions": events.get("instructions", math.nan),
                "ipc": ipc,
                "cache_references": events.get("cache_references", math.nan),
                "cache_misses": events.get("cache_misses", math.nan),
                "cache_miss_rate": miss_rate,
                "context_switches": events.get("context_switches", math.nan),
                "cpu_migrations": events.get("cpu_migrations", math.nan),
                "task_clock": events.get("task_clock", math.nan),
                "perf_csv": str(perf_path),
            }
        )
    return consolidated


COUNTER_FIELDS = [
    "problem",
    "workload_n",
    "workers",
    "model",
    "bios",
    "smt_factor",
    "seed",
    "steps",
    "runs",
    "warmup",
    "cycles",
    "instructions",
    "ipc",
    "cache_references",
    "cache_misses",
    "cache_miss_rate",
    "context_switches",
    "cpu_migrations",
    "task_clock",
    "perf_csv",
]


def write_counters(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=COUNTER_FIELDS)
        writer.writeheader()
        for row in rows:
            out = {key: row.get(key, "") for key in COUNTER_FIELDS}
            writer.writerow(out)


def _finite(value: object) -> bool:
    try:
        return math.isfinite(float(value))  # type: ignore[arg-type]
    except (TypeError, ValueError):
        return False


def _model_sort_key(model: str) -> tuple[int, str]:
    try:
        return (MODEL_ORDER.index(model), model)
    except ValueError:
        return (len(MODEL_ORDER), model)


def plot_metric_by_model(
    rows: list[dict[str, object]],
    metric: str,
    ylabel: str,
    title: str,
    output_path: Path,
    *,
    group_workers: bool = True,
) -> None:
    """Grouped bar chart: models on x (or model×workers), one figure."""
    if not rows:
        return

    # Filter finite metric values
    usable = [r for r in rows if _finite(r.get(metric))]
    if not usable:
        return

    if group_workers:
        # sequential is w=1 only; show it once, then parallel models per worker
        labels: list[str] = []
        values: list[float] = []
        colors: list[str] = []

        seq = [r for r in usable if r["model"] == "sequential"]
        if seq:
            # Prefer the single sequential row; if several, take mean
            labels.append("sequential")
            values.append(sum(float(r[metric]) for r in seq) / len(seq))
            colors.append(MODEL_COLORS["sequential"])

        workers = sorted({int(r["workers"]) for r in usable if r["model"] != "sequential"})
        parallel = [m for m in MODEL_ORDER if m != "sequential"]
        for w in workers:
            for model in parallel:
                subset = [
                    r
                    for r in usable
                    if r["model"] == model and int(r["workers"]) == w
                ]
                if not subset:
                    continue
                labels.append(f"{model}\nw={w}")
                values.append(sum(float(r[metric]) for r in subset) / len(subset))
                colors.append(MODEL_COLORS.get(model, "#333333"))
    else:
        by_model: dict[str, list[float]] = defaultdict(list)
        for row in usable:
            by_model[str(row["model"])].append(float(row[metric]))  # type: ignore[arg-type]
        labels = sorted(by_model.keys(), key=_model_sort_key)
        values = [sum(by_model[m]) / len(by_model[m]) for m in labels]
        colors = [MODEL_COLORS.get(m, "#333333") for m in labels]

    if not labels:
        return

    width = min(22, max(8, len(labels) * 0.55))
    fig, axis = plt.subplots(figsize=(width, 5.5), constrained_layout=True)
    xpos = list(range(len(labels)))
    axis.bar(xpos, values, color=colors, alpha=0.85)
    axis.set_xticks(xpos)
    axis.set_xticklabels(labels, rotation=45, ha="right", fontsize=8)
    axis.set_ylabel(ylabel)
    axis.set_title(title)
    axis.grid(True, axis="y", alpha=0.25)
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def plot_ipc_heatmap(rows: list[dict[str, object]], output_path: Path) -> None:
    """Heatmap of IPC for parallel models: rows=model, cols=workers (mean over workloads)."""
    parallel = [r for r in rows if r["model"] != "sequential" and _finite(r.get("ipc"))]
    if not parallel:
        return

    workers = sorted({int(r["workers"]) for r in parallel})
    models = [m for m in MODEL_ORDER if m != "sequential"]
    models = [m for m in models if any(r["model"] == m for r in parallel)]
    if not models or not workers:
        return

    matrix: list[list[float]] = []
    for model in models:
        row_vals: list[float] = []
        for w in workers:
            subset = [
                float(r["ipc"])  # type: ignore[arg-type]
                for r in parallel
                if r["model"] == model and int(r["workers"]) == w and _finite(r["ipc"])
            ]
            row_vals.append(sum(subset) / len(subset) if subset else math.nan)
        matrix.append(row_vals)

    fig, axis = plt.subplots(figsize=(7, 4.5), constrained_layout=True)
    image = axis.imshow(matrix, aspect="auto", cmap="viridis")
    axis.set_xticks(range(len(workers)))
    axis.set_xticklabels([str(w) for w in workers])
    axis.set_yticks(range(len(models)))
    axis.set_yticklabels(models)
    axis.set_xlabel("workers")
    axis.set_ylabel("model")
    axis.set_title("Mean IPC (avg over workloads)")
    for i, model in enumerate(models):
        for j, _w in enumerate(workers):
            val = matrix[i][j]
            if math.isfinite(val):
                axis.text(j, i, f"{val:.2f}", ha="center", va="center", color="w", fontsize=8)
    fig.colorbar(image, ax=axis, fraction=0.046, pad=0.04, label="IPC")
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def plot_workload_lines(
    rows: list[dict[str, object]],
    metric: str,
    ylabel: str,
    title: str,
    output_path: Path,
    workers: int,
) -> None:
    """Line plot: workload_n vs metric, one line per model, fixed workers."""
    usable = [
        r
        for r in rows
        if _finite(r.get(metric))
        and (
            r["model"] == "sequential"
            or int(r["workers"]) == workers
        )
    ]
    if not usable:
        return

    fig, axis = plt.subplots(figsize=(8.5, 5), constrained_layout=True)
    for model in MODEL_ORDER:
        subset = [r for r in usable if r["model"] == model]
        if not subset:
            continue
        subset = sorted(subset, key=lambda r: int(r["workload_n"]))
        xs = [int(r["workload_n"]) for r in subset]
        ys = [float(r[metric]) for r in subset]  # type: ignore[arg-type]
        axis.plot(
            xs,
            ys,
            marker="o",
            linewidth=2,
            color=MODEL_COLORS.get(model, "#333333"),
            label=model,
        )
    axis.set_xscale("log")
    axis.set_xlabel("workload N (log)")
    axis.set_ylabel(ylabel)
    axis.set_title(f"{title} (workers={workers}; sequential once)")
    axis.grid(True, which="both", alpha=0.25)
    axis.legend(title="model", ncols=2)
    fig.savefig(output_path, dpi=160)
    plt.close(fig)


def generate_figures(rows: list[dict[str, object]], fig_dir: Path) -> list[Path]:
    fig_dir.mkdir(parents=True, exist_ok=True)
    written: list[Path] = []

    problems = sorted({str(r["problem"]) for r in rows})
    bios_labels = sorted({str(r["bios"]) for r in rows})

    for problem in problems:
        for bios in bios_labels:
            subset = [r for r in rows if r["problem"] == problem and r["bios"] == bios]
            if not subset:
                continue
            prefix = f"{problem}_bios-{bios}"

            path = fig_dir / f"{prefix}_ipc_by_model.png"
            plot_metric_by_model(
                subset,
                "ipc",
                "IPC (instructions / cycles)",
                f"{problem}: IPC by model (bios={bios})",
                path,
            )
            if path.is_file():
                written.append(path)

            path = fig_dir / f"{prefix}_cache_miss_rate.png"
            plot_metric_by_model(
                subset,
                "cache_miss_rate",
                "Cache miss rate",
                f"{problem}: cache miss rate (bios={bios})",
                path,
            )
            if path.is_file():
                written.append(path)

            path = fig_dir / f"{prefix}_context_switches.png"
            plot_metric_by_model(
                subset,
                "context_switches",
                "Context switches",
                f"{problem}: context switches (bios={bios})",
                path,
            )
            if path.is_file():
                written.append(path)

            path = fig_dir / f"{prefix}_ipc_heatmap.png"
            plot_ipc_heatmap(subset, path)
            if path.is_file():
                written.append(path)

            workers = sorted(
                {int(r["workers"]) for r in subset if r["model"] != "sequential"}
            )
            for w in workers:
                path = fig_dir / f"{prefix}_ipc_vs_workload_w{w}.png"
                plot_workload_lines(
                    subset,
                    "ipc",
                    "IPC",
                    f"{problem}: IPC vs workload",
                    path,
                    workers=w,
                )
                if path.is_file():
                    written.append(path)

    # Cross-problem overview at a representative cell if present
    overview = [
        r
        for r in rows
        if r["model"] == "cmp" and int(r["workers"]) == 8 and _finite(r.get("ipc"))
    ]
    if overview:
        path = fig_dir / "overview_cmp_w8_ipc_by_problem.png"
        by_problem: dict[str, list[float]] = defaultdict(list)
        for row in overview:
            by_problem[str(row["problem"])].append(float(row["ipc"]))  # type: ignore[arg-type]
        labels = sorted(by_problem.keys())
        values = [sum(by_problem[p]) / len(by_problem[p]) for p in labels]
        fig, axis = plt.subplots(figsize=(8, 4.5), constrained_layout=True)
        axis.bar(labels, values, color="#1f77b4", alpha=0.85)
        axis.set_ylabel("IPC")
        axis.set_title("CMP workers=8: mean IPC by problem")
        axis.grid(True, axis="y", alpha=0.25)
        fig.savefig(path, dpi=160)
        plt.close(fig)
        written.append(path)

    return written


def main() -> int:
    args = parse_args()
    if not args.manifest.is_file():
        print(f"ERROR: manifest not found: {args.manifest}", file=sys.stderr)
        return 1

    fig_dir = args.figures if args.figures is not None else args.outdir / "figures"
    args.outdir.mkdir(parents=True, exist_ok=True)

    manifest_rows = load_manifest(args.manifest)
    if not manifest_rows:
        print("ERROR: empty manifest", file=sys.stderr)
        return 1

    try:
        rows = consolidate(manifest_rows, args.manifest)
    except FileNotFoundError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1
    counters_path = args.outdir / "perf_counters.csv"
    write_counters(counters_path, rows)
    print(f"Wrote {counters_path} ({len(rows)} rows)")

    missing_events = [
        r for r in rows if not _finite(r.get("cycles")) or not _finite(r.get("instructions"))
    ]
    if missing_events:
        print(
            f"WARN: {len(missing_events)} cells missing cycles/instructions "
            "(check perf permissions / event names).",
            file=sys.stderr,
        )

    written = generate_figures(rows, fig_dir)
    print(f"Wrote {len(written)} figures under {fig_dir}")
    for path in written:
        print(f"  {path.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

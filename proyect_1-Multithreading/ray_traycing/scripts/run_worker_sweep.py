"""Run a physical-host worker sweep and generate benchmark analysis plots."""

from __future__ import annotations

import argparse
import csv
import math
import os
import platform
import statistics
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt


RAYTRACING_DIR = Path(__file__).resolve().parents[1]
PIXEL_COUNT = 240 * 160
MODELS = ("fgmt", "cgmt", "smt", "cmp")
MODEL_COLORS = {
    "fgmt": "#0072B2",
    "cgmt": "#D55E00",
    "smt": "#009E73",
    "cmp": "#CC79A7",
}
CONVERGENCE_CI_LIMIT_PERCENT = 5.0
Z_95 = 1.96


def benchmark_candidates() -> list[Path]:
    """Return likely benchmark executable locations for supported build layouts."""
    executable = "raytracing_benchmark.exe" if os.name == "nt" else "raytracing_benchmark"
    return [
        RAYTRACING_DIR / "build-msys2" / executable,
        RAYTRACING_DIR / "build" / "Release" / executable,
        RAYTRACING_DIR / "build" / executable,
    ]


def reject_known_virtualized_hosts() -> None:
    """Refuse common Linux container and WSL environments."""
    if platform.system() != "Linux":
        return

    release = platform.release().lower()
    container_markers = (
        Path("/.dockerenv"),
        Path("/run/.containerenv"),
        Path("/.singularity.d"),
    )
    if "microsoft" in release or os.environ.get("WSL_DISTRO_NAME"):
        raise RuntimeError("WSL is not allowed for the required physical-host measurements")
    if any(marker.exists() for marker in container_markers) or os.environ.get("container"):
        raise RuntimeError("Containers are not allowed for the required physical-host measurements")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run at least 200 frames for each worker count on a physical host, "
            "then create speedup, distribution, and convergence plots."
        )
    )
    parser.add_argument(
        "--workers",
        nargs="+",
        type=int,
        default=[1, 2, 4, 8],
        metavar="N",
        help="worker/context counts to benchmark (default: 1 2 4 8)",
    )
    parser.add_argument(
        "--runs",
        type=int,
        default=200,
        help="repetitions per model and worker count; minimum is 200 (default: 200)",
    )
    parser.add_argument(
        "--benchmark",
        type=Path,
        help="path to raytracing_benchmark (auto-detected from build directories by default)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=RAYTRACING_DIR / "data" / "worker_sweep",
        help="directory for consolidated CSV files and PNG plots",
    )
    parser.add_argument(
        "--confirm-physical-host",
        action="store_true",
        help="confirm that measurements are running directly on physical hardware",
    )
    args = parser.parse_args()

    if args.runs < 200:
        parser.error("--runs must be at least 200 to satisfy the project's experiment requirement")
    if not args.workers or any(workers < 1 or workers > PIXEL_COUNT for workers in args.workers):
        parser.error(f"--workers values must be between 1 and {PIXEL_COUNT}")
    if len(set(args.workers)) != len(args.workers):
        parser.error("--workers values must not contain duplicates")
    if not args.confirm_physical_host:
        parser.error(
            "confirm direct physical-hardware execution with --confirm-physical-host; "
            "do not run under a VM, container, or WSL"
        )
    return args


def resolve_benchmark(path: Path | None) -> Path:
    if path is not None:
        resolved = path.expanduser().resolve()
        if not resolved.is_file():
            raise FileNotFoundError(f"benchmark executable not found: {resolved}")
        return resolved

    for candidate in benchmark_candidates():
        if candidate.is_file():
            return candidate.resolve()
    options = "\n".join(f"  {candidate}" for candidate in benchmark_candidates())
    raise FileNotFoundError(
        "could not find raytracing_benchmark; build the project or pass --benchmark. "
        f"Checked:\n{options}"
    )


def read_csv(path: Path) -> tuple[list[str], list[dict[str, str]]]:
    with path.open("r", newline="", encoding="utf-8-sig") as input_file:
        reader = csv.DictReader(input_file)
        if not reader.fieldnames:
            raise ValueError(f"CSV has no header: {path}")
        return list(reader.fieldnames), list(reader)


def write_csv(path: Path, fieldnames: list[str], rows: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", newline="", encoding="utf-8") as output_file:
        writer = csv.DictWriter(output_file, fieldnames=fieldnames, extrasaction="ignore")
        writer.writeheader()
        writer.writerows(rows)


def summarize(values: list[float]) -> dict[str, float]:
    mean = statistics.fmean(values)
    stddev = statistics.stdev(values) if len(values) > 1 else 0.0
    half_width = Z_95 * stddev / math.sqrt(len(values))
    return {
        "mean": mean,
        "stddev": stddev,
        "ci95_lower": mean - half_width,
        "ci95_upper": mean + half_width,
        "ci95_half_width_percent": (100.0 * half_width / abs(mean)) if mean else 0.0,
    }


def add_virtual_time_analysis(
    summary_rows: list[dict[str, str]],
    frame_rows: list[dict[str, str]],
) -> list[dict[str, str]]:
    """Derive virtual-time summaries and speedups from individual frame samples."""
    grouped: dict[tuple[str, int], list[float]] = {}
    for row in frame_rows:
        virtual_time_s = float(row["virtual_time_ns"]) / 1_000_000_000
        row["virtual_time_s"] = str(virtual_time_s)
        key = (row["model"], int(row["configured_workers"]))
        grouped.setdefault(key, []).append(virtual_time_s)

    for summary_row in summary_rows:
        key = (summary_row["model"], int(summary_row["configured_workers"]))
        values = grouped.get(key)
        if not values:
            raise ValueError(f"no virtual-time samples for {key[0]} with {key[1]} workers")
        stats = summarize(values)
        summary_row["mean_virtual_time_s"] = str(stats["mean"])
        summary_row["stddev_virtual_time_s"] = str(stats["stddev"])
        summary_row["ci95_virtual_time_lower_s"] = str(stats["ci95_lower"])
        summary_row["ci95_virtual_time_upper_s"] = str(stats["ci95_upper"])

        worker_count = key[1]
        baseline_values = grouped.get(("sequential", worker_count))
        if not baseline_values:
            raise ValueError(f"no sequential virtual-time baseline for {worker_count} workers")
        baseline_stats = summarize(baseline_values)
        if stats["mean"] <= 0.0 or baseline_stats["mean"] <= 0.0:
            raise ValueError("virtual execution time must be positive to calculate speedup")

        speedup = baseline_stats["mean"] / stats["mean"]
        if key[0] == "sequential":
            half_width = 0.0
        else:
            baseline_relative_error = baseline_stats["stddev"] / (
                math.sqrt(len(baseline_values)) * baseline_stats["mean"]
            )
            model_relative_error = stats["stddev"] / (
                math.sqrt(len(values)) * stats["mean"]
            )
            half_width = Z_95 * speedup * math.sqrt(
                baseline_relative_error**2 + model_relative_error**2
            )

        summary_row["virtual_speedup"] = str(speedup)
        summary_row["virtual_speedup_ci95_lower"] = str(max(0.0, speedup - half_width))
        summary_row["virtual_speedup_ci95_upper"] = str(speedup + half_width)
    return summary_rows


def run_campaign(
    benchmark: Path,
    workers: list[int],
    runs: int,
    output_dir: Path,
) -> tuple[list[dict[str, str]], list[dict[str, str]]]:
    summary_rows: list[dict[str, str]] = []
    frame_rows: list[dict[str, str]] = []
    summary_fields: list[str] | None = None
    frame_fields: list[str] | None = None

    with tempfile.TemporaryDirectory(
        prefix=".worker_sweep_", dir=output_dir
    ) as temporary_directory:
        temporary_path = Path(temporary_directory)
        for worker_count in workers:
            summary_path = temporary_path / f"benchmark_{worker_count}_workers.csv"
            command = [
                str(benchmark),
                "--model",
                "all",
                "--workers",
                str(worker_count),
                "--runs",
                str(runs),
                "--frame-csv",
                "--output",
                str(summary_path),
            ]
            print(f"\nBenchmark: workers={worker_count}, runs={runs}")
            subprocess.run(command, cwd=RAYTRACING_DIR, check=True)

            current_summary_fields, current_summary_rows = read_csv(summary_path)
            if summary_fields is None:
                summary_fields = current_summary_fields
            elif current_summary_fields != summary_fields:
                raise ValueError("benchmark summary CSV columns changed during the sweep")
            summary_rows.extend(
                {"configured_workers": str(worker_count), **row}
                for row in current_summary_rows
            )

            for model in ("sequential", *MODELS):
                frame_path = summary_path.with_name(
                    f"{summary_path.stem}_{model}_frames.csv"
                )
                current_frame_fields, current_frame_rows = read_csv(frame_path)
                if frame_fields is None:
                    frame_fields = current_frame_fields
                elif current_frame_fields != frame_fields:
                    raise ValueError("benchmark frame CSV columns changed during the sweep")
                frame_rows.extend(
                    {"configured_workers": str(worker_count), **row}
                    for row in current_frame_rows
                )

    if summary_fields is None or frame_fields is None:
        raise RuntimeError("no benchmark samples were collected")
    return summary_rows, frame_rows


def write_convergence_csv(frame_rows: list[dict[str, str]], output_path: Path) -> list[dict[str, Any]]:
    grouped: dict[tuple[str, int], list[float]] = {}
    for row in frame_rows:
        key = (row["model"], int(row["configured_workers"]))
        grouped.setdefault(key, []).append(float(row["virtual_time_s"]))

    results: list[dict[str, Any]] = []
    fields = [
        "model",
        "configured_workers",
        "runs",
        "mean_virtual_time_s",
        "stddev_virtual_time_s",
        "ci95_lower_s",
        "ci95_upper_s",
        "ci95_half_width_percent",
        "stable_at_5_percent_ci",
    ]
    for (model, worker_count), values in sorted(grouped.items()):
        stats = summarize(values)
        results.append(
            {
                "model": model,
                "configured_workers": worker_count,
                "runs": len(values),
                "mean_virtual_time_s": stats["mean"],
                "stddev_virtual_time_s": stats["stddev"],
                "ci95_lower_s": stats["ci95_lower"],
                "ci95_upper_s": stats["ci95_upper"],
                "ci95_half_width_percent": stats["ci95_half_width_percent"],
                "stable_at_5_percent_ci": (
                    "yes"
                    if stats["ci95_half_width_percent"] <= CONVERGENCE_CI_LIMIT_PERCENT
                    else "no"
                ),
            }
        )
    write_csv(output_path, fields, results)
    return results


def plot_speedup(summary_rows: list[dict[str, str]], output_path: Path) -> None:
    fig, axis = plt.subplots(figsize=(9, 5.5), constrained_layout=True)
    for model in MODELS:
        rows = sorted(
            (row for row in summary_rows if row["model"] == model),
            key=lambda row: int(row["configured_workers"]),
        )
        x = [int(row["configured_workers"]) for row in rows]
        means = [float(row["virtual_speedup"]) for row in rows]
        lower = [
            max(0.0, mean - float(row["virtual_speedup_ci95_lower"]))
            for row, mean in zip(rows, means)
        ]
        upper = [
            max(0.0, float(row["virtual_speedup_ci95_upper"]) - mean)
            for row, mean in zip(rows, means)
        ]
        axis.errorbar(
            x,
            means,
            yerr=[lower, upper],
            color=MODEL_COLORS[model],
            marker="o",
            linewidth=2,
            capsize=4,
            label=model.upper(),
        )

    axis.axhline(1.0, color="#555555", linestyle="--", linewidth=1, label="sequential (1x)")
    axis.set_title("Speedup basado en tiempo virtual vs. baseline secuencial")
    axis.set_xlabel("Cantidad configurada de workers / contextos")
    axis.set_ylabel("Speedup virtual (VT secuencial medio / VT del modelo)")
    axis.set_xticks(sorted({int(row["configured_workers"]) for row in summary_rows}))
    axis.grid(True, axis="both", alpha=0.25)
    axis.legend(title="Modelo / color", ncols=2)
    fig.savefig(output_path, dpi=180)
    plt.close(fig)


def plot_execution_distributions(frame_rows: list[dict[str, str]], output_path: Path) -> None:
    grouped: dict[tuple[str, int], list[float]] = {}
    sequential_values: list[float] = []
    for row in frame_rows:
        model = row["model"]
        virtual_time = float(row["virtual_time_s"])
        if model == "sequential":
            sequential_values.append(virtual_time)
        else:
            key = (model, int(row["configured_workers"]))
            grouped.setdefault(key, []).append(virtual_time)

    configurations = sorted(
        grouped,
        key=lambda item: (MODELS.index(item[0]), item[1]),
    )
    labels = ["sequential baseline"] + [
        f"{model.upper()} ({worker_count})"
        for model, worker_count in configurations
    ]
    values = [sequential_values] + [grouped[key] for key in configurations]
    width = min(24, max(12, len(labels) * 0.65))
    fig, axis = plt.subplots(figsize=(width, 6), constrained_layout=True)
    boxplot = axis.boxplot(values, labels=labels, patch_artist=True, showfliers=False)
    for patch, configuration in zip(boxplot["boxes"], [None, *configurations]):
        if configuration is None:
            patch.set_facecolor("#777777")
        else:
            model = configuration[0]
            patch.set_facecolor(MODEL_COLORS[model])
        patch.set_alpha(0.55)

    axis.set_title("Distribución del tiempo virtual por frame")
    axis.set_xlabel("Modelo (cantidad configurada de workers/contextos)")
    axis.set_ylabel("Tiempo virtual por frame (s)")
    axis.tick_params(axis="x", labelrotation=45)
    axis.grid(True, axis="y", alpha=0.25)
    fig.savefig(output_path, dpi=180)
    plt.close(fig)


def plot_convergence(frame_rows: list[dict[str, str]], output_path: Path) -> None:
    groups: dict[tuple[str, int], list[float]] = {}
    for row in frame_rows:
        key = (row["model"], int(row["configured_workers"]))
        groups.setdefault(key, []).append(float(row["virtual_time_s"]))

    fig, axes = plt.subplots(3, 2, figsize=(13, 12), constrained_layout=True)
    model_order = ("sequential", *MODELS)
    line_styles = ("-", "--", ":", "-.")
    for axis, model in zip(axes.flat, model_order):
        model_groups = sorted(
            ((worker_count, values) for (name, worker_count), values in groups.items()
             if name == model),
            key=lambda group: group[0],
        )
        if not model_groups:
            axis.set_visible(False)
            continue

        for index, (worker_count, values) in enumerate(model_groups):
            cumulative_means: list[float] = []
            total = 0.0
            for sample_index, value in enumerate(values, start=1):
                total += value
                cumulative_means.append(total / sample_index)
            final_mean = cumulative_means[-1]
            label = (
                f"baseline batch {worker_count}"
                if model == "sequential"
                else f"{worker_count} workers/contexts"
            )
            axis.plot(
                range(1, len(cumulative_means) + 1),
                cumulative_means,
                color="#777777" if model == "sequential" else MODEL_COLORS[model],
                linestyle=line_styles[index % len(line_styles)],
                linewidth=1.5,
                label=label,
            )
            axis.axhline(
                final_mean,
                color="#777777" if model == "sequential" else MODEL_COLORS[model],
                linestyle=":",
                linewidth=1,
                alpha=0.7,
            )
        axis.set_title(f"{model.upper()} - convergencia de la media")
        axis.set_xlabel("Número de ejecuciones acumuladas")
        axis.set_ylabel("Media acumulada de tiempo virtual (s)")
        axis.grid(True, alpha=0.25)
        axis.legend(fontsize=8)

    axes.flat[-1].set_visible(False)
    fig.savefig(output_path, dpi=180)
    plt.close(fig)


def main() -> int:
    args = parse_args()
    try:
        reject_known_virtualized_hosts()
        benchmark = resolve_benchmark(args.benchmark)
        output_dir = args.output_dir.expanduser().resolve()
        output_dir.mkdir(parents=True, exist_ok=True)

        summary_rows, frame_rows = run_campaign(
            benchmark, args.workers, args.runs, output_dir
        )
        summary_rows = add_virtual_time_analysis(summary_rows, frame_rows)
        summary_fields = ["configured_workers"] + [
            field for field in summary_rows[0] if field != "configured_workers"
        ]
        frame_fields = ["configured_workers"] + [
            field for field in frame_rows[0] if field != "configured_workers"
        ]
        write_csv(output_dir / "worker_sweep_summary.csv", summary_fields, summary_rows)
        write_csv(output_dir / "worker_sweep_frames.csv", frame_fields, frame_rows)
        convergence_rows = write_convergence_csv(
            frame_rows, output_dir / "convergence.csv"
        )
        plot_speedup(summary_rows, output_dir / "speedup_vs_workers.png")
        plot_execution_distributions(
            frame_rows, output_dir / "virtual_time_distributions.png"
        )
        plot_convergence(frame_rows, output_dir / "convergence.png")

        print(f"\nResults written to: {output_dir}")
        print("  speedup_vs_workers.png")
        print("  virtual_time_distributions.png")
        print("  convergence.png")
        print("  worker_sweep_summary.csv")
        print("  worker_sweep_frames.csv")
        print("  convergence.csv")
        unstable = [
            row for row in convergence_rows
            if row["stable_at_5_percent_ci"] == "no"
        ]
        if unstable:
            print(
                f"Convergence check: {len(unstable)} configuration(s) exceed "
                f"the {CONVERGENCE_CI_LIMIT_PERCENT:.0f}% relative 95% CI threshold; "
                "consider increasing --runs."
            )
        else:
            print(
                f"Convergence check: all configurations are within the "
                f"{CONVERGENCE_CI_LIMIT_PERCENT:.0f}% relative 95% CI threshold."
            )
    except (OSError, RuntimeError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

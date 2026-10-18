#!/usr/bin/env python3
"""
Genera 3 imagenes por variante HT (ht_off / ht_on):

  plots/comparison.png   — cuadricula 3x3 (filas=N, cols=W); cada celda contiene
                           la serie temporal MEDIA de todos los problemas y todas
                           las metodologias. Color = problema, estilo = metodologia.
  plots/performance.png  — X=N, Y=tiempo medio; 3 subgraficas (W=2,4,8);
                           todos los problemas y metodologias.
  plots/scalability.png  — X=W, Y=tiempo medio; 3 subgraficas (N=50,500,5000);
                           todos los problemas y metodologias paralelas + ref. secuencial.

Estructura de entrada:
  results/
    ht_off/campaign/{timestamp}/{problema}/*.csv
    ht_on/campaign/{timestamp}/{problema}/*.csv
"""

import csv
import re
import sys
from pathlib import Path


# ─── DEPENDENCIAS ─────────────────────────────────────────────────────────────

def _ensure_deps():
    try:
        import matplotlib
        import numpy
        return
    except ImportError:
        pass

    exe = sys.executable
    is_msys2 = any(tok in exe.lower() for tok in ("msys64", "msys2", "mingw", "ucrt"))
    if is_msys2:
        print(
            "ERROR: faltan dependencias (matplotlib, numpy).\n"
            "Instalalas con pacman:\n\n"
            "  pacman -S mingw-w64-ucrt-x86_64-python-matplotlib "
            "mingw-w64-ucrt-x86_64-python-numpy\n"
        )
        sys.exit(1)

    import subprocess
    try:
        subprocess.check_call([exe, "-m", "pip", "install", "matplotlib", "numpy"])
    except subprocess.CalledProcessError:
        print(f"ERROR: instala manualmente:\n  {exe} -m pip install matplotlib numpy")
        sys.exit(1)


_ensure_deps()

import matplotlib                              # noqa: E402
matplotlib.use("Agg")
import matplotlib.lines as mlines             # noqa: E402
import matplotlib.patches as mpatches        # noqa: E402
import matplotlib.pyplot as plt               # noqa: E402
import numpy as np                            # noqa: E402

# ─── RUTAS ────────────────────────────────────────────────────────────────────

SCRIPTS_DIR = Path(__file__).resolve().parent
RESULTS_DIR = SCRIPTS_DIR.parent / "results"

# ─── PATRONES DE NOMBRE DE ARCHIVO ────────────────────────────────────────────

_SEQ_RAW = re.compile(r"^n_(\d+)_sequential_raw\.csv$")
_PAR_RAW = re.compile(r"^n_(\d+)_w_(\d+)_(fine|coarse|smt|cmp)_raw\.csv$")
_RAY_SEQ = re.compile(r"^n_(\d+)_sequential_summary_sequential_frames\.csv$")
_RAY_PAR = re.compile(r"^n_(\d+)_w_(\d+)_(fine|coarse|smt|cmp)_summary_\3_frames\.csv$")

# ─── ETIQUETAS, COLORES Y PARAMETROS ──────────────────────────────────────────

PROBLEMS   = ["boids", "liquids", "ray", "nbody"]
MODELS_ALL = ["sequential", "fine", "coarse", "smt", "cmp"]
MODELS_PAR = ["fine", "coarse", "smt", "cmp"]
NS = [50, 500, 5000]
WS = [2, 4, 8]

PROBLEM_LABELS = {
    "boids":   "Boids",
    "liquids": "Liquids",
    "ray":     "Ray Tracing",
    "nbody":   "N-Body",
}
MODEL_LABELS = {
    "sequential": "Secuencial",
    "fine":       "Fine-Grained",
    "coarse":     "Coarse-Grained",
    "smt":        "SMT",
    "cmp":        "CMP",
}

# Color por metodologia (para graficas centradas en metodologia)
METH_COLORS = {
    "sequential": "#4C72B0",
    "fine":       "#DD8452",
    "coarse":     "#55A868",
    "smt":        "#C44E52",
    "cmp":        "#8172B2",
}

# Color por problema
PROB_COLORS = {
    "boids":   "#1f77b4",   # azul
    "liquids": "#ff7f0e",   # naranja
    "ray":     "#2ca02c",   # verde
    "nbody":   "#d62728",   # rojo
}

# Estilo de linea por metodologia
LINESTYLES = {
    "sequential": "-",
    "fine":       "--",
    "coarse":     "-.",
    "smt":        ":",
    "cmp":        (0, (5, 1)),   # guion denso
}

# ─── LECTURA DE DATOS ─────────────────────────────────────────────────────────


def read_times(
    path: Path,
    is_ray: bool = False,
    is_liquids: bool = False,
    model_filter: str = None,
) -> list:
    time_col = "execution_time_s" if is_ray else "seconds"
    times = []
    if not path.is_file():
        return times
    try:
        with open(path, newline="", encoding="utf-8") as f:
            reader = csv.DictReader(f)
            for row in reader:
                if row.get("phase", "").strip() != "measured":
                    continue
                if is_liquids and row.get("stage", "").strip() != "total":
                    continue
                if model_filter and row.get("model", "").strip() != model_filter:
                    continue
                try:
                    times.append(float(row[time_col]))
                except (ValueError, KeyError):
                    pass
    except Exception as exc:
        print(f"  AVISO al leer {path.name}: {exc}")
    return times


def collect_problem_data(prob_dir: Path, prob: str) -> dict:
    """Retorna { model: { n: { w: [times] } } }. Sequential usa w=0."""
    is_liquids = prob == "liquids"
    is_ray     = prob == "ray"
    data = {m: {} for m in MODELS_ALL}

    for csv_path in sorted(prob_dir.iterdir()):
        if csv_path.suffix != ".csv":
            continue
        name = csv_path.name

        m_seq     = _SEQ_RAW.match(name)
        m_par     = _PAR_RAW.match(name)
        m_ray_seq = _RAY_SEQ.match(name)
        m_ray_par = _RAY_PAR.match(name)

        if m_seq:
            n_val, w_val, model = int(m_seq.group(1)), 0, "sequential"
            times = read_times(csv_path, is_liquids=is_liquids, model_filter="sequential")
        elif m_par:
            n_val = int(m_par.group(1))
            w_val = int(m_par.group(2))
            model = m_par.group(3)
            times = read_times(csv_path, is_liquids=is_liquids, model_filter=model)
        elif m_ray_seq:
            n_val, w_val, model = int(m_ray_seq.group(1)), 0, "sequential"
            times = read_times(csv_path, is_ray=True)
        elif m_ray_par:
            n_val = int(m_ray_par.group(1))
            w_val = int(m_ray_par.group(2))
            model = m_ray_par.group(3)
            times = read_times(csv_path, is_ray=True)
        else:
            continue

        data[model].setdefault(n_val, {})[w_val] = times

    return data


def collect_all_data(ht_dir: Path) -> dict:
    """Retorna { prob: { model: { n: { w: [times] } } } }."""
    campaign_dir = ht_dir / "campaign"
    if not campaign_dir.is_dir():
        return {}

    all_data = {}
    for run_dir in sorted(campaign_dir.iterdir()):
        if not run_dir.is_dir():
            continue
        for prob_dir in sorted(run_dir.iterdir()):
            if not prob_dir.is_dir():
                continue
            prob = prob_dir.name
            all_data[prob] = collect_problem_data(prob_dir, prob)

    return all_data


# ─── UTILIDADES DE GRAFICACION ────────────────────────────────────────────────


def _style_ax(ax: plt.Axes, subtitle: str, fontsize: int = 9):
    ax.set_title(subtitle, fontsize=fontsize, fontweight="bold", pad=5)
    ax.yaxis.grid(True, linestyle="--", alpha=0.4, color="lightgray", linewidth=0.6)
    ax.set_axisbelow(True)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)
    ax.tick_params(labelsize=7)


def _draw_mean_band(ax, times, color, linestyle):
    """Dibuja media (linea) + banda de desvio estandar. Sin puntos individuales."""
    arr    = np.array(times)
    x      = np.arange(1, len(arr) + 1)
    mean_v = float(np.mean(arr))
    std_v  = float(np.std(arr))

    ax.plot(x, arr, color=color, linewidth=0.35, alpha=0.08, zorder=1,
            linestyle=linestyle)
    ax.axhline(mean_v, color=color, linewidth=1.4, linestyle=linestyle,
               alpha=0.92, zorder=3)
    ax.fill_between(x, mean_v - std_v, mean_v + std_v,
                    color=color, alpha=0.05, zorder=2)


def _avg_across(lists: list) -> list:
    """Promedio elemento a elemento de varias listas de tiempos (alineadas por indice de ejecucion)."""
    valid = [l for l in lists if l]
    if not valid:
        return []
    min_len = min(len(l) for l in valid)
    arr = np.array([l[:min_len] for l in valid])
    return list(np.mean(arr, axis=0))


def _save_fig(fig, out_path: Path):
    """Guarda la figura en PNG (150 dpi) y SVG."""
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=150, bbox_inches="tight")
    svg_path = out_path.with_suffix(".svg")
    fig.savefig(svg_path, bbox_inches="tight")
    print(f"    {out_path.name}  +  {svg_path.name}")


def _add_figure_legend(fig):
    """Leyenda con dos secciones: colores (problema) y estilos (metodologia)."""
    prob_handles = [
        mlines.Line2D([], [], color=PROB_COLORS[p], linewidth=2.5,
                      linestyle="-", label=PROBLEM_LABELS[p])
        for p in PROBLEMS
    ]
    method_handles = [
        mlines.Line2D([], [], color="dimgray", linewidth=1.8,
                      linestyle=LINESTYLES[m], label=MODEL_LABELS[m])
        for m in MODELS_ALL
    ]
    sep = mlines.Line2D([], [], color="none", label=" ")

    all_handles = [
        mlines.Line2D([], [], color="none", label="Problema:"),
        *prob_handles,
        sep,
        mlines.Line2D([], [], color="none", label="Metodologia:"),
        *method_handles,
    ]
    fig.legend(
        handles=all_handles,
        loc="upper center",
        bbox_to_anchor=(0.5, 1.0),
        ncol=2 + max(len(PROBLEMS), len(MODELS_ALL)),
        fontsize=8,
        framealpha=0.9,
        handlelength=2.5,
        columnspacing=0.9,
    )


# ─── IMAGEN 1: COMPARISON (serie temporal, todos los problemas) ───────────────


def generate_comparison_grid(all_data: dict, out_path: Path):
    """
    Cuadricula 3x3 (filas=N, cols=W).
    Cada celda: media de todos los problemas x todas las metodologias.
    Color = problema, estilo de linea = metodologia.
    """
    fig, axes = plt.subplots(
        len(NS), len(WS),
        figsize=(5.5 * len(WS), 4.0 * len(NS)),
        squeeze=False,
    )
    fig.patch.set_facecolor("white")

    for r, n_val in enumerate(NS):
        for c, w_val in enumerate(WS):
            ax = axes[r][c]
            has_any = False

            for prob in PROBLEMS:
                prob_data = all_data.get(prob, {})
                color     = PROB_COLORS[prob]

                seq_times = prob_data.get("sequential", {}).get(n_val, {}).get(0, [])
                if seq_times:
                    _draw_mean_band(ax, seq_times, color, LINESTYLES["sequential"])
                    has_any = True

                for model in MODELS_PAR:
                    times = prob_data.get(model, {}).get(n_val, {}).get(w_val, [])
                    if times:
                        _draw_mean_band(ax, times, color, LINESTYLES[model])
                        has_any = True

            if not has_any:
                ax.text(0.5, 0.5, "sin datos", transform=ax.transAxes,
                        ha="center", va="center", fontsize=9, color="lightgray")

            _style_ax(ax, f"N={n_val}  |  W={w_val}")
            if r == len(NS) - 1:
                ax.set_xlabel("Numero de prueba", fontsize=8)
            if c == 0:
                ax.set_ylabel("Tiempo de ejecucion (s)", fontsize=8)

    _add_figure_legend(fig)
    fig.suptitle(
        "Comparativa de metodologias y problemas - serie temporal",
        fontsize=13, fontweight="bold", y=1.05,
    )
    fig.tight_layout(rect=[0, 0, 1, 0.96])
    _save_fig(fig, out_path)
    plt.close(fig)


# ─── IMAGEN 2: PERFORMANCE (X=N, Y=tiempo medio) ─────────────────────────────


def generate_performance_graph(all_data: dict, plots_dir: Path):
    """
    X=N (log), Y=tiempo medio (log).
    3 subgraficas (una por W); cada una contiene todos los problemas x metodologias.
    Sequential aparece igual en las tres columnas (no depende de W).
    """
    fig, axes = plt.subplots(
        1, len(WS),
        figsize=(5.5 * len(WS), 4.5),
        squeeze=False,
    )
    fig.patch.set_facecolor("white")

    for c, w_val in enumerate(WS):
        ax = axes[0][c]

        for prob in PROBLEMS:
            prob_data = all_data.get(prob, {})
            color = PROB_COLORS[prob]

            for model in MODELS_ALL:
                model_data = prob_data.get(model, {})
                xs, ys = [], []
                for n_val in NS:
                    w_key = 0 if model == "sequential" else w_val
                    times = model_data.get(n_val, {}).get(w_key, [])
                    if times:
                        xs.append(n_val)
                        ys.append(float(np.mean(times)))

                if len(xs) >= 2:
                    ax.plot(xs, ys, color=color, linewidth=1.4,
                            linestyle=LINESTYLES[model],
                            marker="o", markersize=4, zorder=3)
                elif len(xs) == 1:
                    ax.scatter(xs, ys, s=25, color=color, zorder=4)

        _style_ax(ax, f"W = {w_val}")
        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_xticks(NS)
        ax.set_xticklabels([str(n) for n in NS], fontsize=7)
        ax.set_xlabel("Particulas (N)", fontsize=8)
        if c == 0:
            ax.set_ylabel("Tiempo medio (s)", fontsize=8)

    _add_figure_legend(fig)
    fig.suptitle(
        "Rendimiento: Tiempo de ejecucion vs Cantidad de particulas (N)",
        fontsize=13, fontweight="bold", y=1.05,
    )
    fig.tight_layout(rect=[0, 0, 1, 0.88])
    _save_fig(fig, plots_dir / "performance.png")
    plt.close(fig)


# ─── IMAGEN 3: SCALABILITY (X=W, Y=tiempo medio) ─────────────────────────────


def generate_scalability_graph(all_data: dict, plots_dir: Path):
    """
    X=W (hilos), Y=tiempo medio.
    3 subgraficas (una por N); cada una contiene todos los problemas x metodologias.
    Sequential aparece como linea horizontal de referencia.
    """
    fig, axes = plt.subplots(
        1, len(NS),
        figsize=(5.5 * len(NS), 4.5),
        squeeze=False,
    )
    fig.patch.set_facecolor("white")

    for c, n_val in enumerate(NS):
        ax = axes[0][c]

        for prob in PROBLEMS:
            prob_data = all_data.get(prob, {})
            color = PROB_COLORS[prob]

            seq_times = prob_data.get("sequential", {}).get(n_val, {}).get(0, [])
            if seq_times:
                mean_seq = float(np.mean(seq_times))
                ax.axhline(mean_seq, color=color, linewidth=1.4,
                           linestyle=LINESTYLES["sequential"], alpha=0.80, zorder=2)

            for model in MODELS_PAR:
                xs, ys = [], []
                for w_val in WS:
                    times = prob_data.get(model, {}).get(n_val, {}).get(w_val, [])
                    if times:
                        xs.append(w_val)
                        ys.append(float(np.mean(times)))

                if len(xs) >= 2:
                    ax.plot(xs, ys, color=color, linewidth=1.4,
                            linestyle=LINESTYLES[model],
                            marker="o", markersize=4, zorder=3)
                elif len(xs) == 1:
                    ax.scatter(xs, ys, s=25, color=color, zorder=4)

        _style_ax(ax, f"N = {n_val}")
        ax.set_xticks(WS)
        ax.set_xticklabels([f"W={w}" for w in WS], fontsize=7)
        ax.set_xlabel("Hilos (W)", fontsize=8)
        if c == 0:
            ax.set_ylabel("Tiempo medio (s)", fontsize=8)

    _add_figure_legend(fig)
    fig.suptitle(
        "Escalabilidad: Tiempo de ejecucion vs Cantidad de hilos (W)",
        fontsize=13, fontweight="bold", y=1.05,
    )
    fig.tight_layout(rect=[0, 0, 1, 0.88])
    _save_fig(fig, plots_dir / "scalability.png")
    plt.close(fig)


# ─── IMAGEN 4: COMPARISON_METHODOLOGIES (agrega problemas, muestra metodologias) ──


def generate_comparison_methodologies(all_data: dict, out_path: Path):
    """
    Cuadricula 3x3 (filas=N, cols=W).
    Omite la dimension de problema: para cada metodologia concatena los tiempos
    de TODOS los problemas y muestra media + banda. 5 lineas por celda.
    Color = metodologia.
    """
    fig, axes = plt.subplots(
        len(NS), len(WS),
        figsize=(5.5 * len(WS), 4.0 * len(NS)),
        squeeze=False,
    )
    fig.patch.set_facecolor("white")

    for r, n_val in enumerate(NS):
        for c, w_val in enumerate(WS):
            ax = axes[r][c]
            has_any = False

            for model in MODELS_ALL:
                lists = []
                for prob in PROBLEMS:
                    prob_data = all_data.get(prob, {})
                    w_key = 0 if model == "sequential" else w_val
                    times = prob_data.get(model, {}).get(n_val, {}).get(w_key, [])
                    if times:
                        lists.append(times)

                averaged = _avg_across(lists)
                if averaged:
                    _draw_mean_band(ax, averaged, METH_COLORS[model], "-")
                    has_any = True

            if not has_any:
                ax.text(0.5, 0.5, "sin datos", transform=ax.transAxes,
                        ha="center", va="center", fontsize=9, color="lightgray")

            _style_ax(ax, f"N={n_val}  |  W={w_val}")
            if r == len(NS) - 1:
                ax.set_xlabel("Muestra (todos los problemas)", fontsize=8)
            if c == 0:
                ax.set_ylabel("Tiempo de ejecucion (s)", fontsize=8)

    meth_handles = [
        mlines.Line2D([], [], color=METH_COLORS[m], linewidth=2.5,
                      linestyle="-", label=MODEL_LABELS[m])
        for m in MODELS_ALL
    ]
    fig.legend(handles=meth_handles, loc="upper center", bbox_to_anchor=(0.5, 1.0),
               ncol=len(MODELS_ALL), fontsize=9, framealpha=0.9, handlelength=2.5)
    fig.suptitle(
        "Comparativa de metodologias (agregado sobre todos los problemas)",
        fontsize=13, fontweight="bold", y=1.05,
    )
    fig.tight_layout(rect=[0, 0, 1, 0.96])
    _save_fig(fig, out_path)
    plt.close(fig)


# ─── IMAGEN 5: COMPARISON_PROBLEMS (agrega metodologias, muestra problemas) ───


def generate_comparison_problems(all_data: dict, out_path: Path):
    """
    Cuadricula 3x3 (filas=N, cols=W).
    Omite la dimension de metodologia: para cada problema concatena los tiempos
    de TODAS las metodologias y muestra media + banda. 4 lineas por celda.
    Color = problema.
    """
    fig, axes = plt.subplots(
        len(NS), len(WS),
        figsize=(5.5 * len(WS), 4.0 * len(NS)),
        squeeze=False,
    )
    fig.patch.set_facecolor("white")

    for r, n_val in enumerate(NS):
        for c, w_val in enumerate(WS):
            ax = axes[r][c]
            has_any = False

            for prob in PROBLEMS:
                prob_data = all_data.get(prob, {})
                lists = []

                seq_times = prob_data.get("sequential", {}).get(n_val, {}).get(0, [])
                if seq_times:
                    lists.append(seq_times)
                for model in MODELS_PAR:
                    times = prob_data.get(model, {}).get(n_val, {}).get(w_val, [])
                    if times:
                        lists.append(times)

                averaged = _avg_across(lists)
                if averaged:
                    _draw_mean_band(ax, averaged, PROB_COLORS[prob], "-")
                    has_any = True

            if not has_any:
                ax.text(0.5, 0.5, "sin datos", transform=ax.transAxes,
                        ha="center", va="center", fontsize=9, color="lightgray")

            _style_ax(ax, f"N={n_val}  |  W={w_val}")
            if r == len(NS) - 1:
                ax.set_xlabel("Muestra (todas las metodologias)", fontsize=8)
            if c == 0:
                ax.set_ylabel("Tiempo de ejecucion (s)", fontsize=8)

    prob_handles = [
        mlines.Line2D([], [], color=PROB_COLORS[p], linewidth=2.5,
                      linestyle="-", label=PROBLEM_LABELS[p])
        for p in PROBLEMS
    ]
    fig.legend(handles=prob_handles, loc="upper center", bbox_to_anchor=(0.5, 1.0),
               ncol=len(PROBLEMS), fontsize=9, framealpha=0.9, handlelength=2.5)
    fig.suptitle(
        "Comparativa de problemas (agregado sobre todas las metodologias)",
        fontsize=13, fontweight="bold", y=1.05,
    )
    fig.tight_layout(rect=[0, 0, 1, 0.96])
    _save_fig(fig, out_path)
    plt.close(fig)


# ─── IMAGENES POR PROBLEMA ({problema}_comparison) ───────────────────────────


def generate_per_problem_grids(all_data: dict, plots_dir: Path) -> int:
    """
    Genera {problema}_comparison.(png/svg) para cada problema.
    Cuadricula 3x3 (filas=N, cols=W), sharey por fila.
    Cada celda: boxplot (caja Q1-Q3 + mediana + bigotes) con todos los puntos
    individuales superpuestos con jitter. Sin linea de media ni banda.
    Color = metodologia.
    """
    count = 0

    for prob in PROBLEMS:
        prob_data  = all_data.get(prob, {})
        prob_label = PROBLEM_LABELS.get(prob, prob.capitalize())

        fig, axes = plt.subplots(
            len(NS), len(WS),
            figsize=(5.5 * len(WS), 4.0 * len(NS)),
            sharey="row",
            squeeze=False,
        )
        fig.patch.set_facecolor("white")

        for r, n_val in enumerate(NS):
            seq_times = prob_data.get("sequential", {}).get(n_val, {}).get(0, [])

            for c, w_val in enumerate(WS):
                ax = axes[r][c]
                has_any = False

                # Sequential (misma serie para toda la fila)
                if seq_times:
                    x = np.arange(1, len(seq_times) + 1)
                    ax.plot(x, seq_times, color=METH_COLORS["sequential"],
                            linewidth=0.6, alpha=0.75, zorder=2,
                            marker="o", markersize=2.5, markeredgewidth=0)
                    has_any = True

                # Metodologias paralelas
                for model in MODELS_PAR:
                    times = prob_data.get(model, {}).get(n_val, {}).get(w_val, [])
                    if times:
                        x = np.arange(1, len(times) + 1)
                        ax.plot(x, times, color=METH_COLORS[model],
                                linewidth=0.6, alpha=0.75, zorder=2,
                                marker="o", markersize=2.5, markeredgewidth=0)
                        has_any = True

                if not has_any:
                    ax.text(0.5, 0.5, "sin datos", transform=ax.transAxes,
                            ha="center", va="center", fontsize=9, color="lightgray")

                _style_ax(ax, f"N={n_val}  |  W={w_val}")
                if r == len(NS) - 1:
                    ax.set_xlabel("Numero de ejecucion", fontsize=8)
                if c == 0:
                    ax.set_ylabel("Tiempo de ejecucion (s)", fontsize=8)

        meth_handles = [
            mpatches.Patch(facecolor=METH_COLORS[m], alpha=0.70, label=MODEL_LABELS[m])
            for m in MODELS_ALL
        ]
        fig.legend(
            handles=meth_handles,
            loc="upper center",
            bbox_to_anchor=(0.5, 1.0),
            ncol=len(MODELS_ALL),
            fontsize=9,
            framealpha=0.9,
        )
        fig.suptitle(
            f"{prob_label} — Distribucion de tiempos por metodologia",
            fontsize=13, fontweight="bold", y=1.05,
        )
        fig.tight_layout(rect=[0, 0, 1, 0.96])
        _save_fig(fig, plots_dir / f"{prob}_comparison.png")
        plt.close(fig)
        count += 2  # PNG + SVG

    return count


# ─── PROCESAMIENTO ────────────────────────────────────────────────────────────


def process_ht_variant(ht_dir: Path) -> int:
    all_data = collect_all_data(ht_dir)
    if not all_data:
        print("  Sin datos encontrados.")
        return 0

    plots_dir = ht_dir / "plots"

    generate_comparison_grid(all_data, plots_dir / "comparison.png")
    generate_comparison_methodologies(all_data, plots_dir / "comparison_methodologies.png")
    generate_comparison_problems(all_data, plots_dir / "comparison_problems.png")
    generate_performance_graph(all_data, plots_dir)
    generate_scalability_graph(all_data, plots_dir)
    per_prob = generate_per_problem_grids(all_data, plots_dir)

    return 10 + per_prob  # 5 combinadas + 4 por problema (PNG + SVG cada una)


# ─── MAIN ─────────────────────────────────────────────────────────────────────


def main():
    if not RESULTS_DIR.is_dir():
        print(f"ERROR: directorio results no encontrado en {RESULTS_DIR}")
        sys.exit(1)

    total = 0
    for variant in ["ht_off", "ht_on"]:
        ht_dir = RESULTS_DIR / variant
        if not ht_dir.is_dir():
            print(f"  No existe: {ht_dir}")
            continue

        print(f"\n{'=' * 55}")
        print(f"  {variant.upper()}")
        print(f"{'=' * 55}")
        count = process_ht_variant(ht_dir)
        total += count
        print(f"  -> {count} imagenes en {ht_dir / 'plots'}")

    print(f"\n{'=' * 55}")
    print(f"  Total general: {total} imagenes")


if __name__ == "__main__":
    main()

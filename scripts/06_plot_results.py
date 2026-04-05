import csv
import json
from collections import defaultdict
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt

INPUT_CSV = Path("data/outputs/batch_metrics/batch_curve_long.csv")
OUTPUT_PLOT = Path("paper/figures/fig3_boundary_curve.png")
OUTPUT_JSON = Path("paper/figures/fig3_boundary_curve_stats.json")

FIGURE_SIZE = (6, 4)
FIGURE_DPI = 300
PLOT_MARKER_SIZE = 4
FILL_ALPHA = 0.2
GRID_ALPHA = 0.6
Y_AXIS_MAX = 1.05


def _load_curve_data(csv_path: Path) -> dict:
    data_by_tol = defaultdict(list)
    with open(csv_path, "r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                t = int(row["tolerance_px"])
                f1 = float(row["boundary_f1"])
                if np.isfinite(f1):
                    data_by_tol[t].append(f1)
            except (ValueError, KeyError):
                continue
    return data_by_tol


def _compute_stats(data_by_tol: dict) -> tuple:
    tolerances = sorted(data_by_tol.keys())
    means, sds, ns = [], [], []
    for t in tolerances:
        scores = np.array(data_by_tol[t], dtype=float)
        ns.append(len(scores))
        means.append(float(np.mean(scores)))
        sds.append(float(np.std(scores, ddof=1)) if len(scores) > 1 else 0.0)
    return tolerances, np.array(means), np.array(sds), ns


def main():
    if not INPUT_CSV.exists():
        print(f"Error: {INPUT_CSV} not found.")
        return

    data_by_tol = _load_curve_data(INPUT_CSV)
    if not data_by_tol:
        print("Warning: No valid data found in CSV. Nothing to plot.")
        return

    tolerances, means, sds, ns = _compute_stats(data_by_tol)
    lower = np.clip(means - sds, 0.0, 1.0)
    upper = np.clip(means + sds, 0.0, 1.0)

    OUTPUT_PLOT.parent.mkdir(parents=True, exist_ok=True)
    plt.figure(figsize=FIGURE_SIZE, dpi=FIGURE_DPI)
    plt.plot(tolerances, means, linewidth=2, label="Mean Boundary F1", marker="o", markersize=PLOT_MARKER_SIZE)
    plt.fill_between(tolerances, lower, upper, alpha=FILL_ALPHA, label="±1 SD")
    plt.xlabel("Spatial Tolerance (pixels)")
    plt.ylabel("Boundary F1 Score")
    plt.title(f"Boundary Adherence Consistency (N ≥ {min(ns)})")
    plt.ylim(0.0, Y_AXIS_MAX)
    plt.xticks(tolerances)
    plt.grid(True, linestyle="--", alpha=GRID_ALPHA)
    plt.legend(loc="lower right")
    plt.tight_layout()
    plt.savefig(OUTPUT_PLOT, dpi=FIGURE_DPI, bbox_inches="tight")

    with open(OUTPUT_JSON, "w") as f:
        json.dump({"tolerances": tolerances, "mean_f1": list(means), "sd_f1": list(sds)}, f, indent=2)

    print(f"Figure saved to: {OUTPUT_PLOT}")


if __name__ == "__main__":
    main()

import csv
import collections
import json
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt

INPUT_CSV = Path("data/outputs/batch_metrics/batch_curve_long.csv")
OUTPUT_PLOT = Path("paper/figures/fig3_boundary_curve.png")
OUTPUT_JSON = Path("paper/figures/fig3_boundary_curve_stats.json")

def main():
    if not INPUT_CSV.exists():
        print(f"Error: {INPUT_CSV} not found.")
        return

    data_by_tol = collections.defaultdict(list)
    with open(INPUT_CSV, "r", newline="") as f:
        reader = csv.DictReader(f)
        for row in reader:
            try:
                t = int(row["tolerance_px"])
                f1 = float(row["boundary_f1"])
                if np.isfinite(f1): data_by_tol[t].append(f1)
            except: continue

    if not data_by_tol: return

    tolerances = sorted(data_by_tol.keys())
    means, sds, ns = [], [], []

    for t in tolerances:
        scores = np.array(data_by_tol[t], dtype=float)
        ns.append(len(scores))
        means.append(float(np.mean(scores)))
        sds.append(float(np.std(scores, ddof=1)) if len(scores) > 1 else 0.0)

    means = np.array(means); sds = np.array(sds)
    lower = np.clip(means - sds, 0.0, 1.0)
    upper = np.clip(means + sds, 0.0, 1.0)

    OUTPUT_PLOT.parent.mkdir(parents=True, exist_ok=True)
    plt.figure(figsize=(6, 4), dpi=300)
    plt.plot(tolerances, means, linewidth=2, label="Mean Boundary F1", marker="o", markersize=4)
    plt.fill_between(tolerances, lower, upper, alpha=0.2, label="±1 SD")
    plt.xlabel("Spatial Tolerance (pixels)")
    plt.ylabel("Boundary F1 Score")
    plt.title(f"Boundary Adherence Consistency (N ≥ {min(ns)})")
    plt.ylim(0.0, 1.05)
    plt.xticks(tolerances)
    plt.grid(True, linestyle="--", alpha=0.6)
    plt.legend(loc="lower right")
    plt.tight_layout()
    plt.savefig(OUTPUT_PLOT, dpi=300, bbox_inches="tight")
    
    with open(OUTPUT_JSON, "w") as f:
        json.dump({"tolerances": tolerances, "mean_f1": list(means), "sd_f1": list(sds)}, f, indent=2)

    print(f"Figure saved to: {OUTPUT_PLOT}")

if __name__ == "__main__":
    main()

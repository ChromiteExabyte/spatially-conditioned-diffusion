import csv
import json
import math
import re
import statistics
from pathlib import Path
import sys
import os

# Add project root to path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import numpy as np
import rasterio
from PIL import Image
from tqdm import tqdm

from src.config import TOLERANCES
from src.analysis import boundary_adherence_curve
from src.metrics import calculate_boundary_adherence

# --- Configuration ---
MASK_DIR = Path("data/rasters")
IMAGE_DIR = Path("data/outputs/images")
OUT_DIR = Path("data/outputs/batch_metrics")

# Regex to extract site_id from image filename (e.g., "site_01_v2.png" -> "site_01")
SITE_ID_REGEX = re.compile(r"^(site_\d+)", re.IGNORECASE)

def load_mask(path: Path) -> np.ndarray:
    with rasterio.open(path) as ds:
        return ds.read(1)

def load_image(path: Path) -> np.ndarray:
    return np.array(Image.open(path).convert("RGB"))

def extract_site_id(image_stem: str) -> str | None:
    m = SITE_ID_REGEX.match(image_stem)
    return m.group(1) if m else None

def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    image_paths = sorted(list(IMAGE_DIR.glob("*.png"))) + sorted(list(IMAGE_DIR.glob("*.jpg")))
    if not image_paths:
        print(f"No images found in {IMAGE_DIR}")
        return

    results = []
    curve_rows = []

    print(f"Found {len(image_paths)} images. Starting batch evaluation...")

    for img_path in tqdm(image_paths):
        site_id = extract_site_id(img_path.stem)
        if site_id is None:
            print(f"Warning: Could not extract site_id from {img_path.name}. Skipping.")
            continue

        mask_path = MASK_DIR / f"{site_id}.tif"
        if not mask_path.exists():
            print(f"Warning: Mask {mask_path.name} not found for {img_path.name}")
            continue

        try:
            mask = load_mask(mask_path)
            img = load_image(img_path)

            if mask.shape != img.shape[:2]:
                print(f"Skipping {img_path.name}: Shape mismatch")
                continue

            tolerances, f1_scores, auc = boundary_adherence_curve(mask, img, TOLERANCES)

            # Sanity check at middle tolerance
            sanity_t = 3 if 3 in TOLERANCES else TOLERANCES[len(TOLERANCES) // 2]
            sanity = calculate_boundary_adherence(mask, img, tolerance_px=sanity_t)

            results.append({
                "filename": img_path.name,
                "site_id": site_id,
                "mask_source": mask_path.name,
                "auc_bas": float(auc),
                "f1_at_t3": float(sanity["boundary_f1"])
            })

            for t, f1 in zip(tolerances, f1_scores):
                curve_rows.append({
                    "filename": img_path.name,
                    "site_id": site_id,
                    "tolerance_px": int(t),
                    "boundary_f1": float(f1),
                })

        except Exception as e:
            print(f"Error processing {img_path.name}: {e}")

    if not results:
        return

    # Stats
    aucs = [r["auc_bas"] for r in results]
    n = len(aucs)
    mean_auc = statistics.mean(aucs)
    std_auc = statistics.stdev(aucs) if n > 1 else 0.0

    # Save
    csv_path = OUT_DIR / "batch_results.csv"
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)

    curve_csv_path = OUT_DIR / "batch_curve_long.csv"
    with open(curve_csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["filename", "site_id", "tolerance_px", "boundary_f1"])
        writer.writeheader()
        writer.writerows(curve_rows)

    summary = {
        "n_samples": n,
        "mean_auc": round(mean_auc, 6),
        "std_auc": round(std_auc, 6),
        "pairing_regex": SITE_ID_REGEX.pattern
    }
    with open(OUT_DIR / "batch_summary.json", "w") as f:
        json.dump(summary, f, indent=2)

    print(f"\n--- Batch Complete ---")
    print(f"Mean AUC-BAS: {mean_auc:.4f} ± {std_auc:.4f}")
    print(f"Saved results to {OUT_DIR}")

if __name__ == "__main__":
    main()

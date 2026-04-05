import csv
import json
import re
import statistics
from pathlib import Path
import sys

import numpy as np
import rasterio
from PIL import Image
from tqdm import tqdm

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from src.config import TOLERANCES
from src.analysis import boundary_adherence_curve
from src.metrics import calculate_boundary_adherence

# --- Configuration ---
MASK_DIR = Path("data/rasters")
IMAGE_DIR = Path("data/outputs/images")
OUT_DIR = Path("data/outputs/batch_metrics")
SITE_ID_REGEX = re.compile(r"^(site_\d+)", re.IGNORECASE)
SANITY_TOLERANCE_PX = 3
SUMMARY_PRECISION = 6


def load_mask(path: Path) -> np.ndarray:
    with rasterio.open(path) as ds:
        return ds.read(1)


def load_image(path: Path) -> np.ndarray:
    return np.array(Image.open(path).convert("RGB"))


def extract_site_id(image_stem: str) -> str | None:
    m = SITE_ID_REGEX.match(image_stem)
    return m.group(1) if m else None


def _process_image(img_path: Path, tolerances: list) -> tuple | None:
    site_id = extract_site_id(img_path.stem)
    if site_id is None:
        print(f"Warning: Could not extract site_id from {img_path.name}. Skipping.")
        return None

    mask_path = MASK_DIR / f"{site_id}.tif"
    if not mask_path.exists():
        print(f"Warning: Mask {mask_path.name} not found for {img_path.name}")
        return None

    mask = load_mask(mask_path)
    img = load_image(img_path)

    if mask.shape != img.shape[:2]:
        print(f"Skipping {img_path.name}: Shape mismatch")
        return None

    tols, f1_scores, auc = boundary_adherence_curve(mask, img, tolerances)

    sanity_t = SANITY_TOLERANCE_PX if SANITY_TOLERANCE_PX in tolerances else tolerances[len(tolerances) // 2]
    sanity = calculate_boundary_adherence(mask, img, tolerance_px=sanity_t)

    result = {
        "filename": img_path.name,
        "site_id": site_id,
        "mask_source": mask_path.name,
        "auc_bas": float(auc),
        "f1_at_t3": float(sanity["boundary_f1"]),
    }
    curve_rows = [
        {"filename": img_path.name, "site_id": site_id, "tolerance_px": int(t), "boundary_f1": float(f1)}
        for t, f1 in zip(tols, f1_scores)
    ]
    return result, curve_rows


def _write_outputs(results: list, curve_rows: list, out_dir: Path) -> None:
    with open(out_dir / "batch_results.csv", "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=results[0].keys())
        writer.writeheader()
        writer.writerows(results)

    with open(out_dir / "batch_curve_long.csv", "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["filename", "site_id", "tolerance_px", "boundary_f1"])
        writer.writeheader()
        writer.writerows(curve_rows)

    aucs = [r["auc_bas"] for r in results]
    n = len(aucs)
    mean_auc = statistics.mean(aucs)
    std_auc = statistics.stdev(aucs) if n > 1 else 0.0

    summary = {
        "n_samples": n,
        "mean_auc": round(mean_auc, SUMMARY_PRECISION),
        "std_auc": round(std_auc, SUMMARY_PRECISION),
        "pairing_regex": SITE_ID_REGEX.pattern,
    }
    with open(out_dir / "batch_summary.json", "w") as f:
        json.dump(summary, f, indent=2)

    print(f"\n--- Batch Complete ---")
    print(f"Mean AUC-BAS: {mean_auc:.4f} ± {std_auc:.4f}")
    print(f"Saved results to {out_dir}")


def main():
    OUT_DIR.mkdir(parents=True, exist_ok=True)

    image_paths = sorted(list(IMAGE_DIR.glob("*.png"))) + sorted(list(IMAGE_DIR.glob("*.jpg")))
    if not image_paths:
        print(f"No images found in {IMAGE_DIR}")
        return

    print(f"Found {len(image_paths)} images. Starting batch evaluation...")

    results, curve_rows = [], []
    for img_path in tqdm(image_paths):
        try:
            output = _process_image(img_path, TOLERANCES)
            if output is not None:
                result, rows = output
                results.append(result)
                curve_rows.extend(rows)
        except Exception as e:
            print(f"Error processing {img_path.name}: {e}")

    if not results:
        print("No results to save.")
        return

    _write_outputs(results, curve_rows, OUT_DIR)


if __name__ == "__main__":
    main()

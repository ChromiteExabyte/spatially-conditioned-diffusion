# Boundary-Adherent GIS Evaluation Tools

### 🌍 Project Overview
This repository focuses on **evaluating boundary adherence** between GIS-derived masks and AI-generated imagery. It provides metrics and batch tooling to quantify how well generated terrain or land-cover imagery respects geographic boundaries.

**Target Use Case:** High-fidelity environmental visualization for urban planning and natural resource modeling (e.g., MNRF/MENDM land-cover simulation).

---

### ✅ What Is Implemented
1. **Boundary Adherence Metrics (AUC-BAS)**:
   - Calculates boundary precision/recall/F1 between a GIS mask and an AI-generated image.
   - Measures Boundary F1 across a 1–5 pixel tolerance sweep and integrates with an AUC-BAS score.
2. **Batch Evaluation Pipeline**:
   - Pairs mask rasters with generated images and produces CSV/JSON summaries.
   - Generates per-tolerance curve data for plotting.
3. **Plotting Utility**:
   - Creates a mean boundary-adherence curve with standard deviation shading.
4. **Synthetic Test Data**:
   - Creates a simple square mask + slightly offset image for quick metric sanity checks.

---

### 📊 Performance & Accuracy
We quantify success using the **Area Under the Boundary Adherence Curve (AUC-BAS)**, which measures how well generated terrain respects vector boundaries across different tolerance thresholds.

*Quantitative results depend on your dataset and are not included in this repo.*

---
### 🚀 Quickstart

Create synthetic data and run the evaluation pipeline:
```bash
python make_test_data.py
python scripts/05_batch_evaluate.py
python scripts/06_plot_results.py
```

Outputs are written to:
- `data/outputs/batch_metrics/` (CSV + JSON summaries)
- `paper/figures/fig3_boundary_curve.png` (plot)

---
### 🔎 Input Expectations

**Mask rasters**
- 2D label arrays (height × width). Multi-class integer labels are supported.
- Float masks in the 0–1 range are treated as binary (threshold at 0.5).
- Mask boundaries are derived from label transitions (not Canny edges).

**AI-generated images**
- Either grayscale (2D) or RGB (3D).
- Canny edges are computed on the image (RGB images are converted to grayscale).

**Batch evaluation pairing**
- Images are matched to masks by site ID extracted from filenames like `site_01_v2.png` → `site_01.tif`.
- If you use a different naming pattern, update `SITE_ID_REGEX` in `scripts/05_batch_evaluate.py`.

---
### 🧭 Repository Layout

- `src/metrics.py`: boundary adherence metric implementation.
- `src/analysis.py`: AUC-BAS curve calculation helper.
- `scripts/05_batch_evaluate.py`: batch evaluation runner.
- `scripts/06_plot_results.py`: plotting utility for mean tolerance curves.
- `make_test_data.py`: synthetic data generator.

---
### 🚧 Not Yet Implemented
The following items are **conceptual or planned** and are not yet present in this repo:
- Vector preprocessing / rasterization workflows.
- Diffusion model training or inference.
- Example datasets or notebooks.

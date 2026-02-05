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

### 🚧 Not Yet Implemented
The following items are **conceptual or planned** and are not yet present in this repo:
- Vector preprocessing / rasterization workflows.
- Diffusion model training or inference.
- Example datasets or notebooks.

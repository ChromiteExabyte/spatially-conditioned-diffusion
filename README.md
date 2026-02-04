# Spatially-Conditioned Diffusion: Boundary-Constrained Terrain Synthesis

### 🌍 Project Overview
This repository implements a "Geo-Generative" pipeline that bridges the gap between traditional **GIS (Geographic Information Systems)** and **Generative AI**. Unlike standard image-to-image models that often "hallucinate" over geographical boundaries, this workflow uses **Deterministic Spatial Synthesis** to ensure that generated imagery adheres strictly to input vector geometry.

**Target Use Case:** High-fidelity environmental visualization for urban planning and natural resource modeling (e.g., MNRF/MENDM land-cover simulation).

---

### 🛠️ Core Technical Pipeline
1. **Vector-to-Tensor Discretization**: 
   - Converts `.shp` or GeoJSON layers into a high-resolution ($1024 \times 1024$) integer grid.
   - Enforces a **Priority Rasterization** queue to resolve topological overlaps (e.g., islands within lakes).
   - Projects all data to **UTM Zone 17N (EPSG:26917)** to ensure 1 pixel ≈ 1 meter ground accuracy.
   
2. **Latent Geometry Injection**:
   - Injects the one-hot encoded spatial mask into the early denoising stages of a Diffusion model (**Diffusion Model: TBD**).
   - Uses the geometry as a "structural lock," allowing the AI to synthesize texture (forest, wetland, water) without drifting across coordinate-defined boundaries.

3. **Quantitative Validation (AUC-BAS)**:
   - Evaluates "drift-resistance" using a **Boundary Adherence Score (BAS)**.
   - Measures the F1-score of generated edges against the ground-truth vector mask across a 1–5 pixel tolerance sweep.

---

### 📊 Performance & Accuracy
We quantify success using the **Area Under the Boundary Adherence Curve (AUC-BAS)**, which measures how well generated terrain respects vector boundaries across different tolerance thresholds.

*Quantitative results will be added as experiments complete.*

---
### 🚀 Project Status

⚠️ **This repository is in active development.** The pipeline is currently being validated on internal datasets.

**Planned Components:**
- [ ] Vector preprocessing scripts
- [ ] Diffusion model training pipeline  
- [ ] Inference & validation tools
- [ ] Example datasets & notebooks

**Current Focus:** Establishing baseline boundary adherence metrics.

Interested in contributing or testing early versions? Open an issue or reach out.

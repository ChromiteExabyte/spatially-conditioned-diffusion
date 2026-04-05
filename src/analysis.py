import numpy as np
from numpy.typing import NDArray
from .metrics import calculate_boundary_adherence

def boundary_adherence_curve(
    mask_gt: NDArray,
    image_ai: NDArray,
    tolerances: list[int],
) -> tuple[list[int], list[float], float]:
    tolerances = list(tolerances)
    f1_scores = []

    for t in tolerances:
        scores = calculate_boundary_adherence(mask_gt, image_ai, tolerance_px=t)
        f1_scores.append(scores["boundary_f1"])

    auc = float(np.trapezoid(f1_scores, tolerances))
    return tolerances, f1_scores, auc

import numpy as np
from skimage.feature import canny
from skimage.color import rgb2gray
from scipy.ndimage import binary_dilation

def calculate_boundary_adherence(mask_gt: np.ndarray, image_ai: np.ndarray, tolerance_px: int = 3) -> dict:
    edges_gt = canny(mask_gt.astype(float), sigma=1.0)

    if image_ai.ndim == 3:
        image_gray = rgb2gray(image_ai)
    else:
        image_gray = image_ai

    edges_ai = canny(image_gray, sigma=2.0)

    buffer_mask = binary_dilation(edges_gt, iterations=tolerance_px)

    ai_edges_in_buffer = np.sum(edges_ai & buffer_mask)
    total_ai_edges = np.sum(edges_ai)
    precision = ai_edges_in_buffer / (total_ai_edges + 1e-8)

    ai_buffer_mask = binary_dilation(edges_ai, iterations=tolerance_px)
    gt_edges_captured = np.sum(edges_gt & ai_buffer_mask)
    total_gt_edges = np.sum(edges_gt)
    recall = gt_edges_captured / (total_gt_edges + 1e-8)

    f1 = 2 * (precision * recall) / (precision + recall + 1e-8)

    return {
        "boundary_precision": round(float(precision), 4),
        "boundary_recall": round(float(recall), 4),
        "boundary_f1": round(float(f1), 4),
        "tolerance_used": int(tolerance_px),
    }

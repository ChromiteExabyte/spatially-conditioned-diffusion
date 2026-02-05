import numpy as np
from skimage.feature import canny
from skimage.color import rgb2gray
from skimage.segmentation import find_boundaries
from scipy.ndimage import binary_dilation

def _mask_to_boundary_edges(mask_gt: np.ndarray) -> np.ndarray:
    if mask_gt.ndim != 2:
        raise ValueError(f"mask_gt must be 2D; got shape {mask_gt.shape}")

    if np.issubdtype(mask_gt.dtype, np.floating):
        mask_min = float(np.nanmin(mask_gt))
        mask_max = float(np.nanmax(mask_gt))
        if 0.0 <= mask_min and mask_max <= 1.0:
            labels = (mask_gt > 0.5).astype(np.uint8)
        else:
            labels = mask_gt.astype(np.int64)
    else:
        labels = mask_gt.astype(np.int64)

    return find_boundaries(labels, mode="thick")

def _image_to_edges(image_ai: np.ndarray, sigma: float = 2.0) -> np.ndarray:
    if image_ai.ndim == 3:
        image_gray = rgb2gray(image_ai)
    else:
        image_gray = image_ai

    return canny(image_gray, sigma=sigma)

def calculate_boundary_adherence(
    mask_gt: np.ndarray,
    image_ai: np.ndarray,
    tolerance_px: int = 3,
    image_edge_sigma: float = 2.0,
) -> dict:
    """Compute boundary adherence between a ground-truth mask and AI image.

    The ground-truth boundary is derived from mask label transitions, while AI
    boundaries come from Canny edges on the image.
    """
    edges_gt = _mask_to_boundary_edges(mask_gt)

    edges_ai = _image_to_edges(image_ai, sigma=image_edge_sigma)

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

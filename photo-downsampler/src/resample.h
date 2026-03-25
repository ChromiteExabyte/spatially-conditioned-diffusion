#pragma once
#include "image.h"

typedef enum {
    ALGO_NEAREST,    /* pixel art / lo-fi */
    ALGO_BOX,        /* area average — best for large ratios */
    ALGO_BILINEAR,   /* smooth, fast */
    ALGO_BICUBIC,    /* sharp, good for mild downscale */
    ALGO_LANCZOS,    /* highest quality, default */
} ResampleAlgo;

/*
 * Crop (or pad) src to 1:1, then resize to size×size.
 *
 * pad=false  center-crop the shorter dimension (no distortion, no bars)
 * pad=true   letterbox/pillarbox with black bars
 */
[[nodiscard]] Image *image_to_square(const Image *src, int size,
                                     ResampleAlgo algo, bool pad);

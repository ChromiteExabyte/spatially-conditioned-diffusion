#pragma once
#include "image.h"

/*
 * Vignette: smooth radial darkening towards the edges.
 * strength: 0.0 (none) … 1.0 (heavy)
 */
void effect_vignette(Image *img, float strength);

/*
 * Film grain: pseudo-random luminance noise via xorshift PRNG.
 * strength: 0.0 (none) … 1.0 (heavy)
 * seed:     0 uses a default seed
 */
void effect_grain(Image *img, float strength, unsigned int seed);

/*
 * Bayer 8×8 ordered dithering — retro halftone aesthetic.
 * Quantizes each channel to 8 levels.
 */
void effect_dither_ordered(Image *img);

/*
 * Floyd-Steinberg error-diffusion dithering — bold graphic look.
 * Quantizes each channel to 4 levels.
 */
void effect_dither_fs(Image *img);

#include "effects.h"
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

/* =========================================================================
   Vignette
   ========================================================================= */

void effect_vignette(Image *img, float strength)
{
    auto cx = img->width  * 0.5f;
    auto cy = img->height * 0.5f;
    /* Normalize so distance = 1 at the corner */
    auto r  = sqrtf(cx * cx + cy * cy);
    auto ch = img->channels;

    for (int y = 0; y < img->height; y++) {
        auto dy = (y - cy) / r;
        for (int x = 0; x < img->width; x++) {
            auto dx = (x - cx) / r;
            auto v  = 1.0f - strength * (dx*dx + dy*dy);
            if (v < 0.0f) v = 0.0f;

            auto p = img->data + ((size_t)y * (size_t)img->width + (size_t)x) * (size_t)ch;
            for (int c = 0; c < ch; c++)
                p[c] = (uint8_t)(p[c] * v + 0.5f);
        }
    }
}

/* =========================================================================
   Film grain  (xorshift32 for speed + reproducibility)
   ========================================================================= */

static uint32_t xorshift(uint32_t *s)
{
    *s ^= *s << 13;
    *s ^= *s >> 17;
    *s ^= *s << 5;
    return *s;
}

void effect_grain(Image *img, float strength, unsigned int seed)
{
    uint32_t state = seed ? (uint32_t)seed : 0xdeadbeef;
    auto ch  = img->channels;
    auto n   = img->width * img->height * ch;
    auto amp = (int)(strength * 64.0f + 0.5f);
    if (amp == 0) return;

    for (int i = 0; i < n; i++) {
        /* Sum of two uniforms → triangle distribution, zero-mean */
        auto r0    = (int32_t)(xorshift(&state) & 0xFF) - 128;
        auto r1    = (int32_t)(xorshift(&state) & 0xFF) - 128;
        auto noise = (r0 + r1) * amp / 256;
        img->data[i] = (uint8_t)CLAMP((int32_t)img->data[i] + noise, 0, 255);
    }
}

/* =========================================================================
   Ordered dithering  (8×8 Bayer matrix)
   ========================================================================= */

/* Standard 64-entry Bayer threshold map, values 0–63 */
static constexpr uint8_t BAYER8[64] = {
     0, 32,  8, 40,  2, 34, 10, 42,
    48, 16, 56, 24, 50, 18, 58, 26,
    12, 44,  4, 36, 14, 46,  6, 38,
    60, 28, 52, 20, 62, 30, 54, 22,
     3, 35, 11, 43,  1, 33,  9, 41,
    51, 19, 59, 27, 49, 17, 57, 25,
    15, 47,  7, 39, 13, 45,  5, 37,
    63, 31, 55, 23, 61, 29, 53, 21,
};

void effect_dither_ordered(Image *img)
{
    auto ch = img->channels;
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            auto bias = (int)BAYER8[(y & 7) * 8 + (x & 7)] * 4 - 128;
            auto p = img->data + ((size_t)y * (size_t)img->width + (size_t)x) * (size_t)ch;
            for (int c = 0; c < ch; c++) {
                auto v = (int)p[c] + bias;
                v = (v / 32) * 32;   /* quantize to 8 levels */
                p[c] = (uint8_t)CLAMP(v, 0, 224);
            }
        }
    }
}

/* =========================================================================
   Floyd-Steinberg error-diffusion dithering
   ========================================================================= */

void effect_dither_fs(Image *img)
{
    auto w  = img->width;
    auto h  = img->height;
    auto ch = img->channels;

    auto buf = (float *)malloc((size_t)w * (size_t)h * (size_t)ch * sizeof(float));
    if (!buf) return;
    for (int i = 0; i < w * h * ch; i++) buf[i] = (float)img->data[i];

    /* Quantize to 4 levels: 0, 85, 170, 255 */
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            for (int c = 0; c < ch; c++) {
                auto idx     = ((size_t)y * (size_t)w + (size_t)x) * (size_t)ch + (size_t)c;
                auto old_val = buf[idx];
                auto new_val = roundf(old_val / 85.0f) * 85.0f;
                new_val = CLAMP(new_val, 0.0f, 255.0f);
                buf[idx] = new_val;

                auto err = old_val - new_val;
                if (x + 1 < w)
                    buf[((size_t)y * (size_t)w + (size_t)(x+1)) * (size_t)ch + (size_t)c] += err * (7.0f/16);
                if (y + 1 < h) {
                    if (x > 0)
                        buf[((size_t)(y+1) * (size_t)w + (size_t)(x-1)) * (size_t)ch + (size_t)c] += err * (3.0f/16);
                    buf[((size_t)(y+1) * (size_t)w + (size_t)x) * (size_t)ch + (size_t)c] += err * (5.0f/16);
                    if (x + 1 < w)
                        buf[((size_t)(y+1) * (size_t)w + (size_t)(x+1)) * (size_t)ch + (size_t)c] += err * (1.0f/16);
                }
            }
        }
    }

    for (int i = 0; i < w * h * ch; i++)
        img->data[i] = (uint8_t)CLAMP((int)(buf[i] + 0.5f), 0, 255);
    free(buf);
}

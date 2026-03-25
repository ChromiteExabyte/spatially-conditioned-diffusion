/*
 * wasm_api.c — Emscripten-exported entry points for the browser build.
 *
 * JS calls:
 *   process_image(ptr, len, size, algo, pad, vignette_pct, grain_pct, dither)
 *   get_output_ptr() → pointer to PNG bytes
 *   get_output_len() → byte count
 *   free_output()    → release internal buffer
 *
 * Memory ownership: the output buffer is held internally until free_output()
 * or the next process_image() call, whichever comes first.
 */
#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#  define WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#  define WASM_EXPORT
#endif

#include "image.h"
#include "resample.h"
#include "effects.h"
#include <stdlib.h>

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

static uint8_t *g_output     = nullptr;
static int      g_output_len = 0;

static void drop_output(void)
{
    image_free_mem(g_output);
    g_output     = nullptr;
    g_output_len = 0;
}

WASM_EXPORT
int process_image(
    const uint8_t *in_data, int in_len,
    int out_size,
    int algo_idx,   /* ResampleAlgo value */
    int pad,        /* 0 = crop, 1 = pad  */
    int vignette_pct,
    int grain_pct,
    int dither      /* 0 = none, 1 = ordered, 2 = fs */
)
{
    drop_output();

    auto src = image_load_mem(in_data, in_len);
    if (!src) return -1;

    auto algo = (ResampleAlgo)CLAMP(algo_idx, ALGO_NEAREST, ALGO_LANCZOS);
    auto out  = image_to_square(src, out_size, algo, (bool)pad);
    image_free(src);
    if (!out) return -2;

    if (vignette_pct > 0) effect_vignette(out, vignette_pct / 100.0f);
    if (grain_pct    > 0) effect_grain(out, grain_pct / 100.0f, 0);
    switch (dither) {
    case 1: effect_dither_ordered(out); break;
    case 2: effect_dither_fs(out);      break;
    }

    g_output = image_to_png_mem(out, &g_output_len);
    image_free(out);
    return g_output ? 0 : -3;
}

WASM_EXPORT uint8_t *get_output_ptr(void) { return g_output; }
WASM_EXPORT int      get_output_len(void) { return g_output_len; }
WASM_EXPORT void     free_output(void)    { drop_output(); }

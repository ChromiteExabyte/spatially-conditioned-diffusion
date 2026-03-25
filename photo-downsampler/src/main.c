#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "resample.h"
#include "effects.h"

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [options] <input> <output>\n"
        "\n"
        "Aesthetic photo downsampler — always outputs a 1:1 (square) image.\n"
        "Supported formats: JPEG, PNG, BMP, TGA, GIF in · JPEG, PNG out\n"
        "\n"
        "Resize:\n"
        "  -s <pixels>       Output size (default: 512)\n"
        "  -a <algo>         lanczos (default) · bicubic · bilinear · box · nearest\n"
        "  --pad             Pad to square with black instead of center-cropping\n"
        "\n"
        "Aesthetic effects (applied in order listed):\n"
        "  --vignette <f>    Radial darkening  0.0–1.0  (try 0.5)\n"
        "  --grain    <f>    Film grain        0.0–1.0  (try 0.15)\n"
        "  --dither-ordered  Bayer 8×8 ordered dithering  (retro halftone)\n"
        "  --dither-fs       Floyd-Steinberg dithering     (bold graphic)\n"
        "\n"
        "Output:\n"
        "  -q <1-100>        JPEG quality (default: 85)\n"
        "  -h, --help\n"
        "\n"
        "Examples:\n"
        "  %s photo.jpg out.jpg\n"
        "  %s -s 256 -a nearest --dither-ordered photo.jpg retro.png\n"
        "  %s -s 1024 --vignette 0.55 --grain 0.12 portrait.png aesthetic.jpg\n",
        prog, prog, prog, prog);
}

int main(int argc, char *argv[])
{
    int          size       = 512;
    int          quality    = 85;
    bool         pad        = false;
    bool         dither_ord = false;
    bool         dither_fs  = false;
    float        vignette   = 0.0f;
    float        grain      = 0.0f;
    ResampleAlgo algo       = ALGO_LANCZOS;
    const char  *input      = nullptr;
    const char  *output     = nullptr;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            usage(argv[0]); return 0;

        } else if (!strcmp(argv[i], "-s")) {
            if (++i >= argc) { fputs("-s requires a value\n", stderr); return 1; }
            size = atoi(argv[i]);
            if (size <= 0) { fputs("size must be > 0\n", stderr); return 1; }

        } else if (!strcmp(argv[i], "-q")) {
            if (++i >= argc) { fputs("-q requires a value\n", stderr); return 1; }
            quality = CLAMP(atoi(argv[i]), 1, 100);

        } else if (!strcmp(argv[i], "-a")) {
            if (++i >= argc) { fputs("-a requires a value\n", stderr); return 1; }
            if      (!strcmp(argv[i], "nearest"))  algo = ALGO_NEAREST;
            else if (!strcmp(argv[i], "box"))       algo = ALGO_BOX;
            else if (!strcmp(argv[i], "bilinear"))  algo = ALGO_BILINEAR;
            else if (!strcmp(argv[i], "bicubic"))   algo = ALGO_BICUBIC;
            else if (!strcmp(argv[i], "lanczos"))   algo = ALGO_LANCZOS;
            else {
                fprintf(stderr, "Unknown algorithm '%s'\n", argv[i]);
                return 1;
            }

        } else if (!strcmp(argv[i], "--pad"))            { pad = true;
        } else if (!strcmp(argv[i], "--dither-ordered")) { dither_ord = true;
        } else if (!strcmp(argv[i], "--dither-fs"))      { dither_fs  = true;

        } else if (!strcmp(argv[i], "--vignette")) {
            if (++i >= argc) { fputs("--vignette requires a value\n", stderr); return 1; }
            vignette = strtof(argv[i], nullptr);

        } else if (!strcmp(argv[i], "--grain")) {
            if (++i >= argc) { fputs("--grain requires a value\n", stderr); return 1; }
            grain = strtof(argv[i], nullptr);

        } else if (!input)  { input  = argv[i];
        } else if (!output) { output = argv[i];
        } else {
            fprintf(stderr, "Unexpected argument: %s\n", argv[i]);
            usage(argv[0]); return 1;
        }
    }

    if (!input || !output) { usage(argv[0]); return 1; }

    /* Load */
    auto src = image_load(input);
    if (!src) { fprintf(stderr, "Failed to load: %s\n", input); return 1; }
    fprintf(stderr, "Loaded  %s  (%d×%d)\n", input, src->width, src->height);

    /* Square crop/pad + resize */
    auto out = image_to_square(src, size, algo, pad);
    image_free(src);
    if (!out) { fputs("Resize failed (OOM?)\n", stderr); return 1; }

    /* Aesthetic effects */
    if (vignette  > 0.0f) effect_vignette(out, vignette);
    if (grain     > 0.0f) effect_grain(out, grain, 0);
    if (dither_ord)       effect_dither_ordered(out);
    if (dither_fs)        effect_dither_fs(out);

    /* Save */
    if (image_save(out, output, quality) != 0) {
        fprintf(stderr, "Failed to save: %s\n", output);
        image_free(out); return 1;
    }
    fprintf(stderr, "Saved   %s  (%d×%d)\n", output, out->width, out->height);

    image_free(out);
    return 0;
}

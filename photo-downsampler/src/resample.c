#include "resample.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

constexpr double PI = 3.14159265358979323846;

#define CLAMP(x, lo, hi) ((x) < (lo) ? (lo) : (x) > (hi) ? (hi) : (x))

/* =========================================================================
   Separable filter infrastructure
   ========================================================================= */

typedef struct {
    int    *idx;   /* source pixel indices */
    double *wt;    /* normalized weights   */
    int     n;
} Contrib;

typedef double (*KernelFn)(double x, void *ctx);

static Contrib *make_contribs(int src_size, int dst_size,
                               KernelFn kernel, void *ctx, double support)
{
    auto c = (Contrib *)calloc((size_t)dst_size, sizeof(Contrib));
    if (!c) return nullptr;

    auto scale  = (double)dst_size / src_size;
    auto fscale = scale < 1.0 ? scale : 1.0;   /* contract when downsampling */
    auto supp   = support / fscale;

    for (int i = 0; i < dst_size; i++) {
        auto center = (i + 0.5) / scale - 0.5;
        auto left   = (int)ceil(center - supp);
        auto right  = (int)floor(center + supp);
        auto n      = right - left + 1;

        c[i].idx = (int *)   malloc((size_t)n * sizeof(int));
        c[i].wt  = (double *)malloc((size_t)n * sizeof(double));
        c[i].n   = n;
        if (!c[i].idx || !c[i].wt) { c[i].n = 0; continue; }

        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            auto src_j = CLAMP(left + j, 0, src_size - 1);
            auto w     = kernel((center - (left + j)) * fscale, ctx);
            c[i].idx[j] = src_j;
            c[i].wt[j]  = w;
            sum += w;
        }
        if (sum != 0.0)
            for (int j = 0; j < n; j++) c[i].wt[j] /= sum;
    }
    return c;
}

static void free_contribs(Contrib *c, int n)
{
    if (!c) return;
    for (int i = 0; i < n; i++) { free(c[i].idx); free(c[i].wt); }
    free(c);
}

static Image *apply_contribs(const Image *src,
                              Contrib *hc, int dst_w,
                              Contrib *vc, int dst_h)
{
    auto ch = src->channels;

    /* Horizontal pass → float buffer [src_h × dst_w × ch] */
    auto tmp = (float *)malloc((size_t)src->height * (size_t)dst_w
                               * (size_t)ch * sizeof(float));
    if (!tmp) return nullptr;

    for (int y = 0; y < src->height; y++) {
        for (int x = 0; x < dst_w; x++) {
            auto out = tmp + ((size_t)y * (size_t)dst_w + (size_t)x) * (size_t)ch;
            for (int c = 0; c < ch; c++) out[c] = 0.0f;
            for (int k = 0; k < hc[x].n; k++) {
                auto p = src->data +
                    ((size_t)y * (size_t)src->width + (size_t)hc[x].idx[k]) * (size_t)ch;
                auto w = hc[x].wt[k];
                for (int c = 0; c < ch; c++) out[c] += (float)(p[c] * w);
            }
        }
    }

    /* Vertical pass → output */
    auto dst = image_new(dst_w, dst_h, ch);
    if (!dst) { free(tmp); return nullptr; }

    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            double acc[4] = {0, 0, 0, 0};
            for (int k = 0; k < vc[y].n; k++) {
                auto p = tmp + ((size_t)vc[y].idx[k] * (size_t)dst_w + (size_t)x) * (size_t)ch;
                auto w = vc[y].wt[k];
                for (int c = 0; c < ch; c++) acc[c] += p[c] * w;
            }
            auto out = dst->data + ((size_t)y * (size_t)dst_w + (size_t)x) * (size_t)ch;
            for (int c = 0; c < ch; c++)
                out[c] = (uint8_t)CLAMP((int)(acc[c] + 0.5), 0, 255);
        }
    }

    free(tmp);
    return dst;
}

/* =========================================================================
   Lanczos-3 kernel
   ========================================================================= */

static double sinc(double x)
{
    if (x == 0.0) return 1.0;
    x *= PI;
    return sin(x) / x;
}

static double lanczos_kernel(double x, void *ctx)
{
    auto a = *(int *)ctx;
    if (x < 0.0) x = -x;
    return x < a ? sinc(x) * sinc(x / a) : 0.0;
}

static Image *resize_lanczos(const Image *src, int dst_w, int dst_h)
{
    int a = 3;
    auto hc = make_contribs(src->width,  dst_w, lanczos_kernel, &a, a);
    auto vc = make_contribs(src->height, dst_h, lanczos_kernel, &a, a);
    auto dst = (hc && vc) ? apply_contribs(src, hc, dst_w, vc, dst_h) : nullptr;
    free_contribs(hc, dst_w);
    free_contribs(vc, dst_h);
    return dst;
}

/* =========================================================================
   Mitchell-Netravali bicubic (B=1/3, C=1/3)
   ========================================================================= */

static double mitchell_kernel(double x, [[maybe_unused]] void *ctx)
{
    constexpr double B = 1.0 / 3.0;
    constexpr double C = 1.0 / 3.0;
    x = fabs(x);
    if (x < 1.0)
        return ((12 - 9*B - 6*C)*x*x*x + (-18 + 12*B + 6*C)*x*x + (6 - 2*B)) / 6.0;
    if (x < 2.0)
        return ((-B - 6*C)*x*x*x + (6*B + 30*C)*x*x +
                (-12*B - 48*C)*x + (8*B + 24*C)) / 6.0;
    return 0.0;
}

static Image *resize_bicubic(const Image *src, int dst_w, int dst_h)
{
    auto hc  = make_contribs(src->width,  dst_w, mitchell_kernel, nullptr, 2.0);
    auto vc  = make_contribs(src->height, dst_h, mitchell_kernel, nullptr, 2.0);
    auto dst = (hc && vc) ? apply_contribs(src, hc, dst_w, vc, dst_h) : nullptr;
    free_contribs(hc, dst_w);
    free_contribs(vc, dst_h);
    return dst;
}

/* =========================================================================
   Bilinear
   ========================================================================= */

static Image *resize_bilinear(const Image *src, int dst_w, int dst_h)
{
    auto dst = image_new(dst_w, dst_h, src->channels);
    if (!dst) return nullptr;
    auto ch = src->channels;

    for (int y = 0; y < dst_h; y++) {
        auto fy = (y + 0.5) * src->height / (double)dst_h - 0.5;
        auto y0 = CLAMP((int)fy,     0, src->height - 1);
        auto y1 = CLAMP(y0 + 1,      0, src->height - 1);
        auto dy = fy - floor(fy);

        for (int x = 0; x < dst_w; x++) {
            auto fx = (x + 0.5) * src->width / (double)dst_w - 0.5;
            auto x0 = CLAMP((int)fx, 0, src->width - 1);
            auto x1 = CLAMP(x0 + 1, 0, src->width - 1);
            auto dx = fx - floor(fx);

            auto p00 = src->data + ((size_t)y0 * (size_t)src->width + (size_t)x0) * (size_t)ch;
            auto p01 = src->data + ((size_t)y0 * (size_t)src->width + (size_t)x1) * (size_t)ch;
            auto p10 = src->data + ((size_t)y1 * (size_t)src->width + (size_t)x0) * (size_t)ch;
            auto p11 = src->data + ((size_t)y1 * (size_t)src->width + (size_t)x1) * (size_t)ch;
            auto out = dst->data + ((size_t)y  * (size_t)dst_w  + (size_t)x)  * (size_t)ch;

            for (int c = 0; c < ch; c++) {
                auto v = p00[c]*(1-dx)*(1-dy) + p01[c]*dx*(1-dy)
                       + p10[c]*(1-dx)*dy     + p11[c]*dx*dy;
                out[c] = (uint8_t)CLAMP((int)(v + 0.5), 0, 255);
            }
        }
    }
    return dst;
}

/* =========================================================================
   Nearest neighbour  (pixel-art / lo-fi)
   ========================================================================= */

static Image *resize_nearest(const Image *src, int dst_w, int dst_h)
{
    auto dst = image_new(dst_w, dst_h, src->channels);
    if (!dst) return nullptr;
    auto ch = src->channels;

    for (int y = 0; y < dst_h; y++) {
        auto sy = CLAMP((int)((y + 0.5) * src->height / dst_h), 0, src->height - 1);
        for (int x = 0; x < dst_w; x++) {
            auto sx = CLAMP((int)((x + 0.5) * src->width / dst_w), 0, src->width - 1);
            memcpy(dst->data + ((size_t)y * (size_t)dst_w + (size_t)x) * (size_t)ch,
                   src->data + ((size_t)sy * (size_t)src->width + (size_t)sx) * (size_t)ch,
                   (size_t)ch);
        }
    }
    return dst;
}

/* =========================================================================
   Box filter  (area average)
   ========================================================================= */

static Image *resize_box(const Image *src, int dst_w, int dst_h)
{
    if (dst_w > src->width || dst_h > src->height)
        return resize_bilinear(src, dst_w, dst_h);

    auto dst = image_new(dst_w, dst_h, src->channels);
    if (!dst) return nullptr;
    auto ch = src->channels;

    auto sx = (double)src->width  / dst_w;
    auto sy = (double)src->height / dst_h;

    for (int y = 0; y < dst_h; y++) {
        auto iy0 = (int)(y * sy);
        auto iy1 = CLAMP((int)ceil((y + 1) * sy) - 1, 0, src->height - 1);
        for (int x = 0; x < dst_w; x++) {
            auto ix0 = (int)(x * sx);
            auto ix1 = CLAMP((int)ceil((x + 1) * sx) - 1, 0, src->width - 1);
            double acc[4] = {0, 0, 0, 0};
            int count = 0;
            for (int iy = iy0; iy <= iy1; iy++)
                for (int ix = ix0; ix <= ix1; ix++) {
                    auto p = src->data + ((size_t)iy * (size_t)src->width + (size_t)ix) * (size_t)ch;
                    for (int c = 0; c < ch; c++) acc[c] += p[c];
                    count++;
                }
            auto out = dst->data + ((size_t)y * (size_t)dst_w + (size_t)x) * (size_t)ch;
            for (int c = 0; c < ch; c++)
                out[c] = (uint8_t)(acc[c] / count + 0.5);
        }
    }
    return dst;
}

/* =========================================================================
   Crop / pad helpers
   ========================================================================= */

static Image *crop_square(const Image *src)
{
    auto side = src->width < src->height ? src->width : src->height;
    auto ox   = (src->width  - side) / 2;
    auto oy   = (src->height - side) / 2;
    auto ch   = src->channels;

    auto out = image_new(side, side, ch);
    if (!out) return nullptr;
    for (int y = 0; y < side; y++)
        memcpy(out->data + (size_t)y * (size_t)side * (size_t)ch,
               src->data + ((size_t)(y + oy) * (size_t)src->width + (size_t)ox) * (size_t)ch,
               (size_t)side * (size_t)ch);
    return out;
}

static Image *pad_square(const Image *src)
{
    auto side = src->width > src->height ? src->width : src->height;
    auto ox   = (side - src->width)  / 2;
    auto oy   = (side - src->height) / 2;
    auto ch   = src->channels;

    auto out = image_new(side, side, ch);
    if (!out) return nullptr;
    memset(out->data, 0, (size_t)side * (size_t)side * (size_t)ch);
    for (int y = 0; y < src->height; y++)
        memcpy(out->data + ((size_t)(y + oy) * (size_t)side + (size_t)ox) * (size_t)ch,
               src->data + (size_t)y * (size_t)src->width * (size_t)ch,
               (size_t)src->width * (size_t)ch);
    return out;
}

/* =========================================================================
   Public API
   ========================================================================= */

static Image *do_resize(const Image *src, int w, int h, ResampleAlgo algo)
{
    switch (algo) {
    case ALGO_NEAREST:  return resize_nearest(src, w, h);
    case ALGO_BOX:      return resize_box(src, w, h);
    case ALGO_BILINEAR: return resize_bilinear(src, w, h);
    case ALGO_BICUBIC:  return resize_bicubic(src, w, h);
    case ALGO_LANCZOS:
    default:            return resize_lanczos(src, w, h);
    }
}

Image *image_to_square(const Image *src, int size, ResampleAlgo algo, bool pad)
{
    auto sq = pad ? pad_square(src) : crop_square(src);
    if (!sq) return nullptr;
    if (sq->width == size) return sq;
    auto out = do_resize(sq, size, size, algo);
    image_free(sq);
    return out;
}

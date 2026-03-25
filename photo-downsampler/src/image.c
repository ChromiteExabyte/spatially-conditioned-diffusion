/*
 * image.c — load/save via bundled stb_image (public domain).
 *
 * All images are normalized to 8-bit RGB on load.
 * No external system libraries required.
 */
#include "image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Pull in stb implementations exactly once in this translation unit. */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_ONLY_TGA
#define STBI_ONLY_GIF
#include "../vendor/stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../vendor/stb_image_write.h"

/* =========================================================================
   Lifecycle
   ========================================================================= */

Image *image_new(int width, int height, int channels)
{
    auto img = (Image *)calloc(1, sizeof(Image));
    if (!img) return nullptr;

    *img = (Image){
        .width    = width,
        .height   = height,
        .channels = channels,
        .data     = malloc((size_t)width * (size_t)height * (size_t)channels),
    };
    if (!img->data) { free(img); return nullptr; }
    return img;
}

void image_free(Image *img)
{
    if (!img) return;
    free(img->data);
    free(img);
}

void image_free_mem(void *ptr) { free(ptr); }

/* =========================================================================
   Load
   ========================================================================= */

Image *image_load(const char *path)
{
    int w, h, orig;
    uint8_t *data = stbi_load(path, &w, &h, &orig, 3);
    if (!data) {
        fprintf(stderr, "load '%s': %s\n", path, stbi_failure_reason());
        return nullptr;
    }
    auto img = (Image *)calloc(1, sizeof(Image));
    if (!img) { stbi_image_free(data); return nullptr; }
    *img = (Image){ .width = w, .height = h, .channels = 3, .data = data };
    return img;
}

Image *image_load_mem(const uint8_t *data, int len)
{
    int w, h, orig;
    uint8_t *pixels = stbi_load_from_memory(data, len, &w, &h, &orig, 3);
    if (!pixels) {
        fprintf(stderr, "load_mem: %s\n", stbi_failure_reason());
        return nullptr;
    }
    auto img = (Image *)calloc(1, sizeof(Image));
    if (!img) { stbi_image_free(pixels); return nullptr; }
    *img = (Image){ .width = w, .height = h, .channels = 3, .data = pixels };
    return img;
}

/* =========================================================================
   Save — file
   ========================================================================= */

/* Case-insensitive ASCII extension match (avoids POSIX strcasecmp). */
static bool ext_is(const char *path, const char *ext)
{
    const char *dot = strrchr(path, '.');
    if (!dot) return false;
    dot++;
    while (*dot && *ext) {
        char a = *dot | 0x20u;   /* ASCII toLower */
        char b = *ext | 0x20u;
        if (a != b) return false;
        dot++; ext++;
    }
    return !*dot && !*ext;
}

int image_save(const Image *img, const char *path, int jpeg_quality)
{
    if (ext_is(path, "png")) {
        auto stride = img->width * img->channels;
        bool ok = stbi_write_png(path, img->width, img->height,
                                 img->channels, img->data, stride);
        return ok ? 0 : -1;
    }
    if (ext_is(path, "jpg") || ext_is(path, "jpeg")) {
        bool ok = stbi_write_jpg(path, img->width, img->height,
                                 img->channels, img->data, jpeg_quality);
        return ok ? 0 : -1;
    }
    fprintf(stderr, "Unsupported output format (use .png or .jpg)\n");
    return -1;
}

/* =========================================================================
   Save — in-memory PNG  (used by WASM API)
   ========================================================================= */

uint8_t *image_to_png_mem(const Image *img, int *out_len)
{
    auto stride = img->width * img->channels;
    return stbi_write_png_to_mem(img->data, stride,
                                 img->width, img->height,
                                 img->channels, out_len);
}

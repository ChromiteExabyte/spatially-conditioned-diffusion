#pragma once
#include <stddef.h>
#include <stdint.h>

typedef struct Image {
    uint8_t *data;
    int      width;
    int      height;
    int      channels;   /* always 3 (RGB) after load */
} Image;

[[nodiscard]] Image   *image_new(int width, int height, int channels);
[[nodiscard]] Image   *image_load(const char *path);
[[nodiscard]] Image   *image_load_mem(const uint8_t *data, int len);
[[nodiscard]] uint8_t *image_to_png_mem(const Image *img, int *out_len);

int  image_save(const Image *img, const char *path, int jpeg_quality);
void image_free(Image *img);
void image_free_mem(void *ptr);   /* free memory returned by image_to_png_mem */

static inline uint8_t *image_px(Image *img, int x, int y) {
    return img->data + ((size_t)y * (size_t)img->width + (size_t)x)
                     * (size_t)img->channels;
}

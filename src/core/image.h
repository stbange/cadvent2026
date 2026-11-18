/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_IMAGE_H
#define CADV_CORE_IMAGE_H

#include <stdbool.h>
#include <stdint.h>

/* Tightly packed 8-bit RGB image, row 0 at the top. */
typedef struct cadv_image {
    int width;
    int height;
    uint8_t *rgb;
} cadv_image;

bool cadv_image_alloc(cadv_image *img, int width, int height);
void cadv_image_free(cadv_image *img);
/* Mirrors rows; GL readback delivers the bottom row first. */
void cadv_image_flip_rows(cadv_image *img);
/* Returns false if (x, y) is outside the image. */
bool cadv_image_get(const cadv_image *img, int x, int y, uint8_t out_rgb[3]);
/* Converts tightly packed RGBA8 into an RGB image of the same size. */
bool cadv_image_from_rgba(cadv_image *img, int width, int height, const uint8_t *rgba);

/* Binary PPM (P6, maxval 255). */
bool cadv_image_write_ppm(const cadv_image *img, const char *path);
bool cadv_image_read_ppm(cadv_image *img, const char *path);

#endif

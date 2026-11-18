/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_TEXDATA_H
#define CADV_GFX_TEXDATA_H

#include <stdbool.h>
#include <stdint.h>

/* RGBA8 pixel data, row 0 at the top. */
typedef struct cadv_texdata {
    int w, h;
    uint8_t *rgba;
} cadv_texdata;

bool cadv_texdata_alloc(cadv_texdata *t, int w, int h);
void cadv_texdata_free(cadv_texdata *t);
void cadv_texdata_set(cadv_texdata *t, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
/* Coordinates wrap around (textures tile). */
const uint8_t *cadv_texdata_get(const cadv_texdata *t, int x, int y);
/* Loads PNG/JPG/TGA/BMP via stb_image; always returns RGBA8. */
bool cadv_texdata_load(cadv_texdata *t, const char *path);
bool cadv_texdata_load_memory(cadv_texdata *t, const uint8_t *data, int len);

#endif

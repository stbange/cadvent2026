/* SPDX-License-Identifier: MIT */
#include "gfx/texdata.h"

#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_TGA
#define STBI_ONLY_BMP
#define STBI_NO_STDIO_WIDE
#include "stb_image.h" /* GCOVR_EXCL_LINE */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

bool cadv_texdata_alloc(cadv_texdata *t, int w, int h)
{
    memset(t, 0, sizeof *t);
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) {
        return false;
    }
    t->rgba = (uint8_t *)calloc((size_t)w * (size_t)h, 4u);
    if (!t->rgba) {    /* GCOVR_EXCL_BR_LINE: allocation failure */
        return false;  /* GCOVR_EXCL_LINE */
    }
    t->w = w;
    t->h = h;
    return true;
}

void cadv_texdata_free(cadv_texdata *t)
{
    free(t->rgba);
    memset(t, 0, sizeof *t);
}

static int wrap(int v, int n)
{
    v %= n;
    return v < 0 ? v + n : v;
}

void cadv_texdata_set(cadv_texdata *t, int x, int y, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    uint8_t *p = t->rgba + ((size_t)wrap(y, t->h) * (size_t)t->w + (size_t)wrap(x, t->w)) * 4u;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = a;
}

const uint8_t *cadv_texdata_get(const cadv_texdata *t, int x, int y)
{
    return t->rgba + ((size_t)wrap(y, t->h) * (size_t)t->w + (size_t)wrap(x, t->w)) * 4u;
}

static bool adopt(cadv_texdata *t, unsigned char *px, int w, int h)
{
    if (!px) {
        return false;
    }
    if (!cadv_texdata_alloc(t, w, h)) { /* GCOVR_EXCL_BR_LINE: stb limits are larger */
        stbi_image_free(px);            /* GCOVR_EXCL_LINE */
        return false;                   /* GCOVR_EXCL_LINE */
    }
    memcpy(t->rgba, px, (size_t)w * (size_t)h * 4u);
    stbi_image_free(px);
    return true;
}

bool cadv_texdata_load(cadv_texdata *t, const char *path)
{
    int w = 0;
    int h = 0;
    int n = 0;
    unsigned char *px;
    memset(t, 0, sizeof *t);
    px = stbi_load(path, &w, &h, &n, 4); /* must run before w/h are read */
    return adopt(t, px, w, h);
}

bool cadv_texdata_load_memory(cadv_texdata *t, const uint8_t *data, int len)
{
    int w = 0;
    int h = 0;
    int n = 0;
    unsigned char *px;
    memset(t, 0, sizeof *t);
    px = stbi_load_from_memory(data, len, &w, &h, &n, 4);
    return adopt(t, px, w, h);
}

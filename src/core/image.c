/* SPDX-License-Identifier: MIT */
#include "core/image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool cadv_image_alloc(cadv_image *img, int width, int height)
{
    memset(img, 0, sizeof *img);
    if (width <= 0 || height <= 0 || width > 16384 || height > 16384) {
        return false;
    }
    img->rgb = (uint8_t *)calloc((size_t)width * (size_t)height, 3u);
    if (!img->rgb) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return false; /* GCOVR_EXCL_LINE: allocation failure not reproducible in tests */
    }
    img->width = width;
    img->height = height;
    return true;
}

void cadv_image_free(cadv_image *img)
{
    free(img->rgb);
    memset(img, 0, sizeof *img);
}

void cadv_image_flip_rows(cadv_image *img)
{
    size_t stride = (size_t)img->width * 3u;
    int top = 0;
    int bottom = img->height - 1;
    uint8_t tmp[3 * 64];
    while (top < bottom) {
        uint8_t *a = img->rgb + (size_t)top * stride;
        uint8_t *b = img->rgb + (size_t)bottom * stride;
        size_t done = 0;
        while (done < stride) {
            size_t n = stride - done < sizeof tmp ? stride - done : sizeof tmp;
            memcpy(tmp, a + done, n);
            memcpy(a + done, b + done, n);
            memcpy(b + done, tmp, n);
            done += n;
        }
        ++top;
        --bottom;
    }
}

bool cadv_image_get(const cadv_image *img, int x, int y, uint8_t out_rgb[3])
{
    const uint8_t *p;
    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return false;
    }
    p = img->rgb + ((size_t)y * (size_t)img->width + (size_t)x) * 3u;
    out_rgb[0] = p[0];
    out_rgb[1] = p[1];
    out_rgb[2] = p[2];
    return true;
}

bool cadv_image_from_rgba(cadv_image *img, int width, int height, const uint8_t *rgba)
{
    size_t i;
    size_t n;
    if (!rgba || !cadv_image_alloc(img, width, height)) {
        return false;
    }
    n = (size_t)width * (size_t)height;
    for (i = 0; i < n; ++i) {
        img->rgb[i * 3u + 0u] = rgba[i * 4u + 0u];
        img->rgb[i * 3u + 1u] = rgba[i * 4u + 1u];
        img->rgb[i * 3u + 2u] = rgba[i * 4u + 2u];
    }
    return true;
}

bool cadv_image_write_ppm(const cadv_image *img, const char *path)
{
    FILE *f;
    size_t n;
    bool ok;
    if (!img->rgb || !path) {
        return false;
    }
    f = fopen(path, "wb");
    if (!f) {
        return false;
    }
    n = (size_t)img->width * (size_t)img->height * 3u;
    ok = fprintf(f, "P6\n%d %d\n255\n", img->width, img->height) > 0;
    ok = ok && fwrite(img->rgb, 1, n, f) == n;
    ok = (fclose(f) == 0) && ok;
    return ok;
}

/* Reads the next header integer, skipping whitespace and '#' comments. */
static bool read_header_int(FILE *f, int *out)
{
    int c;
    long v = 0;
    int digits = 0;
    for (;;) {
        c = fgetc(f);
        if (c == '#') {
            while (c != '\n' && c != EOF) {
                c = fgetc(f);
            }
        } else if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            break;
        }
    }
    while (c >= '0' && c <= '9' && digits < 6) {
        v = v * 10 + (c - '0');
        ++digits;
        c = fgetc(f);
    }
    if (digits == 0 || (c != ' ' && c != '\t' && c != '\r' && c != '\n')) {
        return false;
    }
    *out = (int)v;
    return true;
}

bool cadv_image_read_ppm(cadv_image *img, const char *path)
{
    FILE *f;
    char magic[2];
    int w = 0;
    int h = 0;
    int maxval = 0;
    size_t n;
    memset(img, 0, sizeof *img);
    f = fopen(path, "rb");
    if (!f) {
        return false;
    }
    if (fread(magic, 1, 2, f) != 2 || magic[0] != 'P' || magic[1] != '6' ||
        !read_header_int(f, &w) || !read_header_int(f, &h) ||
        !read_header_int(f, &maxval) || maxval != 255 || !cadv_image_alloc(img, w, h)) {
        fclose(f);
        return false;
    }
    n = (size_t)w * (size_t)h * 3u;
    if (fread(img->rgb, 1, n, f) != n) {
        cadv_image_free(img);
        fclose(f);
        return false;
    }
    fclose(f);
    return true;
}

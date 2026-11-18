/* SPDX-License-Identifier: MIT */
#include "gfx/palette.h"

#include <stddef.h>

#include "core/mathx.h"

void cadv_palette_from_ramps(cadv_palette *p, const cadv_ramp *ramps, int count)
{
    int per;
    int r;
    int i;
    if (count < 1 || count > CADV_PALETTE_SIZE || CADV_PALETTE_SIZE % count != 0) {
        count = 1;
    }
    per = CADV_PALETTE_SIZE / count;
    for (r = 0; r < count; ++r) {
        for (i = 0; i < per; ++i) {
            float t = per > 1 ? (float)i / (float)(per - 1) : 0.0f;
            cadv_rgba a = ramps[r].dark;
            cadv_rgba b = ramps[r].light;
            p->c[r * per + i] = cadv_rgba_make((uint8_t)(cadv_lerpf(a.r, b.r, t) + 0.5f),
                                               (uint8_t)(cadv_lerpf(a.g, b.g, t) + 0.5f),
                                               (uint8_t)(cadv_lerpf(a.b, b.b, t) + 0.5f), 255);
        }
    }
}

void cadv_palette_hall(cadv_palette *p)
{
    static const cadv_ramp ramps[16] = {
        {{4, 5, 12, 255}, {60, 70, 120, 255}},     /* night blue */
        {{20, 20, 24, 255}, {170, 165, 160, 255}}, /* basalt / stone */
        {{40, 6, 10, 255}, {200, 40, 50, 255}},    /* crimson banners */
        {{4, 24, 12, 255}, {60, 170, 80, 255}},    /* pine green */
        {{60, 36, 4, 255}, {255, 214, 90, 255}},   /* gold */
        {{70, 20, 0, 255}, {255, 190, 110, 255}},  /* candle light */
        {{10, 40, 60, 255}, {200, 245, 255, 255}}, /* ice */
        {{40, 24, 14, 255}, {190, 140, 90, 255}},  /* wood */
        {{30, 30, 40, 255}, {250, 250, 255, 255}}, /* snow / marble */
        {{50, 30, 24, 255}, {250, 200, 170, 255}}, /* skin / warm stone */
        {{20, 0, 30, 255}, {180, 100, 220, 255}},  /* violet portal light */
        {{0, 30, 40, 255}, {80, 220, 230, 255}},   /* teal portal light */
        {{30, 10, 0, 255}, {255, 120, 40, 255}},   /* fire */
        {{0, 0, 0, 255}, {255, 255, 255, 255}},    /* neutral grey */
        {{10, 14, 30, 255}, {120, 140, 200, 255}}, /* moonlight */
        {{2, 2, 4, 255}, {96, 94, 100, 255}},      /* fine dark greys for night scenes */
    };
    cadv_palette_from_ramps(p, ramps, 16);
}

int cadv_palette_nearest(const cadv_palette *p, uint8_t r, uint8_t g, uint8_t b)
{
    int best = 0;
    long bestd = 0x7fffffffL;
    int i;
    /* luma + chroma distance: hue shifts cost more than brightness steps, so
     * dark greys never turn purple or orange */
    long y = 299L * r + 587L * g + 114L * b;
    for (i = 0; i < CADV_PALETTE_SIZE; ++i) {
        long py = 299L * p->c[i].r + 587L * p->c[i].g + 114L * p->c[i].b;
        long dy = (py - y) / 1000;
        long dcb = ((long)p->c[i].b * 1000 - py) / 1000 - ((long)b * 1000 - y) / 1000;
        long dcr = ((long)p->c[i].r * 1000 - py) / 1000 - ((long)r * 1000 - y) / 1000;
        long d = 2 * dy * dy + 3 * (dcb * dcb + dcr * dcr);
        if (d < bestd) {
            bestd = d;
            best = i;
        }
    }
    return best;
}

void cadv_palette_build_lut(const cadv_palette *p, uint8_t *rgb_out)
{
    int r;
    int g;
    int b;
    for (b = 0; b < CADV_LUT_DIM; ++b) {
        for (g = 0; g < CADV_LUT_DIM; ++g) {
            for (r = 0; r < CADV_LUT_DIM; ++r) {
                uint8_t cr = (uint8_t)((r * 255) / (CADV_LUT_DIM - 1));
                uint8_t cg = (uint8_t)((g * 255) / (CADV_LUT_DIM - 1));
                uint8_t cb = (uint8_t)((b * 255) / (CADV_LUT_DIM - 1));
                const cadv_rgba *c = &p->c[cadv_palette_nearest(p, cr, cg, cb)];
                uint8_t *o = rgb_out + (((size_t)b * CADV_LUT_DIM + (size_t)g) * CADV_LUT_DIM + (size_t)r) * 3u;
                o[0] = c->r;
                o[1] = c->g;
                o[2] = c->b;
            }
        }
    }
}

float cadv_bayer4(int x, int y)
{
    static const int m[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    return (float)m[(y & 3) * 4 + (x & 3)] / 16.0f;
}

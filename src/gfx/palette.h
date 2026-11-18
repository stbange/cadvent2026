/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_PALETTE_H
#define CADV_GFX_PALETTE_H

#include <stdint.h>

#include "gfx/mesh.h"

/* 256-colour palettes that give each portal its 90s colour mood. The renderer
 * quantises every pixel to the active palette through a 32^3 lookup table. */
#define CADV_PALETTE_SIZE 256
#define CADV_LUT_DIM 32

typedef struct cadv_palette {
    cadv_rgba c[CADV_PALETTE_SIZE];
} cadv_palette;

typedef struct cadv_ramp {
    cadv_rgba dark;
    cadv_rgba light;
} cadv_ramp;

/* Fills the palette with 'count' ramps of 256/count shades each (count must
 * divide 256 and be 1..256). Index 0 is the first ramp's darkest colour. */
void cadv_palette_from_ramps(cadv_palette *p, const cadv_ramp *ramps, int count);
/* The hall's palette: night blues, stone, crimson, pine, gold, candle light, ice ... */
void cadv_palette_hall(cadv_palette *p);
/* Index of the nearest colour (perceptually weighted RGB distance). */
int cadv_palette_nearest(const cadv_palette *p, uint8_t r, uint8_t g, uint8_t b);
/* Builds the LUT: CADV_LUT_DIM^3 RGB8 entries, index = (b*32 + g)*32 + r. */
void cadv_palette_build_lut(const cadv_palette *p, uint8_t *rgb_out);
/* 4x4 Bayer threshold in [0,1). */
float cadv_bayer4(int x, int y);

#endif

/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_PROCGEN_H
#define CADV_GFX_PROCGEN_H

#include <stdbool.h>
#include <stdint.h>

#include "gfx/mesh.h"
#include "gfx/texdata.h"

/* Procedural, seamlessly tiling textures. All project art that is generated
 * here is original and covered by CC BY 4.0. */

/* Tileable value noise in [0,1] with integer lattice 'period'. */
float cadv_noise2(uint32_t seed, float x, float y, int period);
/* Fractal sum of 'octaves' noise layers, in [0,1]. */
float cadv_fbm2(uint32_t seed, float x, float y, int period, int octaves);

typedef enum cadv_tex_kind {
    CADV_TEX_STONE_BRICKS = 0,
    CADV_TEX_PLANKS,
    CADV_TEX_FLOOR_TILES,
    CADV_TEX_ICE,
    CADV_TEX_CLOTH,
    CADV_TEX_METAL,
    CADV_TEX_PLASTER,
    CADV_TEX_NOISE,
    CADV_TEX_KIND_COUNT
} cadv_tex_kind;

/* Generates a size x size texture (size must be a power of two, 8..256). */
bool cadv_procgen_texture(cadv_texdata *out, cadv_tex_kind kind, int size, uint32_t seed,
                          cadv_rgba base, cadv_rgba accent);

#endif

/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_VOXEL_H
#define CADV_GFX_VOXEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gfx/mesh.h"
#include "gfx/texdata.h"

/* Voxel model: cells hold a palette index (0 = empty). Y is up. */
typedef struct cadv_voxels {
    int w, h, d; /* x, y, z sizes */
    uint8_t *cells;
    cadv_rgba pal[256]; /* pal[i] is the colour of index i (pal[0] unused) */
} cadv_voxels;

bool cadv_voxels_alloc(cadv_voxels *v, int w, int h, int d);
void cadv_voxels_free(cadv_voxels *v);
uint8_t cadv_voxels_get(const cadv_voxels *v, int x, int y, int z); /* 0 outside */
void cadv_voxels_set(cadv_voxels *v, int x, int y, int z, uint8_t c);
int cadv_voxels_count(const cadv_voxels *v);

/* MagicaVoxel .vox (first model; SIZE, XYZI, RGBA chunks). The file's Z-up
 * axis becomes our Y-up axis. Without RGBA a neutral grey ramp is used. */
bool cadv_vox_parse(cadv_voxels *v, const uint8_t *data, size_t len);
bool cadv_vox_load(cadv_voxels *v, const char *path);
/* Writes a .vox file image; returns bytes needed (snprintf-style). */
size_t cadv_vox_write(const cadv_voxels *v, uint8_t *out, size_t cap);

/* Greedy meshing: merges coplanar same-colour faces into large quads.
 * 'scale' = world size of one voxel; the model is centred on X/Z with its base
 * at y = 0. */
void cadv_voxels_mesh(const cadv_voxels *v, float scale, cadv_mesh *out);

/* Bakes 'dirs' views (rotating around Y, view 0 looks at the model's front,
 * i.e. from +Z) into a horizontal strip atlas. Each frame is fw x fh pixels
 * with 'ppv' pixels per voxel. 'normals' (optional) receives encoded normals. */
bool cadv_voxels_bake_sprites(const cadv_voxels *v, int dirs, int ppv, cadv_texdata *color,
                              cadv_texdata *normals, int *fw, int *fh);

#endif

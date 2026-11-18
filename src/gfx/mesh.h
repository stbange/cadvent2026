/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_MESH_H
#define CADV_GFX_MESH_H

#include <stdbool.h>
#include <stdint.h>

#include "core/mathx.h"

/* CPU-side triangle mesh with a single interleaved vertex format shared by
 * meshes, voxels and sprites. Built here, uploaded by gpu_mesh. */
typedef struct cadv_vertex {
    float pos[3];
    float normal[3];
    float uv[2];
    uint8_t color[4];
} cadv_vertex;

typedef struct cadv_mesh {
    cadv_vertex *v;
    uint32_t *idx;
    uint32_t nv, ni;
    uint32_t cap_v, cap_i;
    bool oom; /* set if any allocation failed; the mesh is then incomplete */
} cadv_mesh;

typedef struct cadv_rgba { uint8_t r, g, b, a; } cadv_rgba;

cadv_rgba cadv_rgba_make(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

void cadv_mesh_init(cadv_mesh *m);
void cadv_mesh_free(cadv_mesh *m);
void cadv_mesh_clear(cadv_mesh *m);
/* Returns the index of the new vertex. */
uint32_t cadv_mesh_vertex(cadv_mesh *m, cadv_vec3 p, cadv_vec3 n, float u, float v, cadv_rgba c);
void cadv_mesh_tri(cadv_mesh *m, uint32_t a, uint32_t b, uint32_t c);
/* Quad from 4 corners in counter-clockwise order (seen from the front). UVs
 * span (0,0)-(us,vs) so textures can tile. */
void cadv_mesh_quad(cadv_mesh *m, cadv_vec3 a, cadv_vec3 b, cadv_vec3 c, cadv_vec3 d, float us,
                    float vs, cadv_rgba col);
/* Axis-aligned box; 'uv_scale' texels per world unit (0 = 0..1 per face).
 * 'inward' flips faces (for rooms seen from inside). */
void cadv_mesh_box(cadv_mesh *m, cadv_vec3 lo, cadv_vec3 hi, float uv_scale, cadv_rgba col,
                   bool inward);
/* Vertical n-sided prism (columns), centre at base. */
void cadv_mesh_prism(cadv_mesh *m, cadv_vec3 base, float radius, float height, int sides,
                     float uv_scale, cadv_rgba col);
/* Half-round arch frame in the XY plane at 'center' (bottom middle), facing +Z,
 * of given inner width/height, thickness (frame width) and depth. */
void cadv_mesh_arch(cadv_mesh *m, cadv_vec3 center, float width, float height, float frame,
                    float depth, int segments, cadv_rgba col);
/* Appends 'src' transformed by 'xf' (normals use the upper 3x3, renormalised). */
void cadv_mesh_append(cadv_mesh *m, const cadv_mesh *src, cadv_mat4 xf);
/* Bounding box of all vertices; returns false for an empty mesh. */
bool cadv_mesh_bounds(const cadv_mesh *m, cadv_vec3 *lo, cadv_vec3 *hi);

#endif

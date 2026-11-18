/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_GL_UTIL_H
#define CADV_GFX_GL_UTIL_H

#include <stdbool.h>
#include <stdint.h>

#include "gfx/mesh.h"
#include "gfx/shader_src.h"
#include "gfx/texdata.h"

/* Thin helpers over OpenGL 3.3 core / GLES 3.0. All need a current context. */

unsigned cadv_gl_program(cadv_gl_profile profile, const char *vs_body, const char *fs_body);
/* RGBA8 texture, nearest filtering; 'repeat' selects wrap mode. */
unsigned cadv_gl_texture(const cadv_texdata *t, bool repeat);
/* 32^3 RGB8 3D texture used as the palette lookup table. */
unsigned cadv_gl_lut3d(const uint8_t *rgb, int dim);
void cadv_gl_delete_textures(int n, const unsigned *textures);

typedef struct cadv_rt {
    unsigned fbo, color, depth; /* depth is a renderbuffer */
    int w, h;
} cadv_rt;
/* Colour + depth render target; reallocated only when the size changes. */
void cadv_rt_ensure(cadv_rt *rt, int w, int h);
void cadv_rt_free(cadv_rt *rt);

typedef struct cadv_shadow2d {
    unsigned fbo, tex;
    int size;
} cadv_shadow2d;
void cadv_shadow2d_init(cadv_shadow2d *s, int size);
void cadv_shadow2d_free(cadv_shadow2d *s);

typedef struct cadv_shadowcube {
    unsigned fbo, tex;
    int size;
} cadv_shadowcube;
void cadv_shadowcube_init(cadv_shadowcube *s, int size);
/* Binds the framebuffer to render cube face 0..5. */
void cadv_shadowcube_bind_face(const cadv_shadowcube *s, int face);
void cadv_shadowcube_free(cadv_shadowcube *s);

typedef struct cadv_gpu_mesh {
    unsigned vao, vbo, ibo;
    int index_count;
    unsigned capacity_v, capacity_i; /* for dynamic meshes */
} cadv_gpu_mesh;
void cadv_gpu_mesh_upload(cadv_gpu_mesh *g, const cadv_mesh *m, bool dynamic);
void cadv_gpu_mesh_draw(const cadv_gpu_mesh *g);
void cadv_gpu_mesh_free(cadv_gpu_mesh *g);

#endif

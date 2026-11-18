/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_RENDERER_H
#define CADV_GFX_RENDERER_H

#include <stdbool.h>

#include "core/image.h"
#include "gfx/shader_src.h"

/* M0 renderer: clears to the hall's night blue and draws one fir-green
 * triangle. Grows into the low-res forward renderer in M1a. */
typedef struct cadv_renderer {
    cadv_gl_profile profile;
    unsigned program;
    unsigned vao;
    unsigned vbo;
    int u_color;
    bool ready;
} cadv_renderer;

/* Colours used by the M0 scene, also asserted by the smoke test. */
#define CADV_CLEAR_R 16
#define CADV_CLEAR_G 20
#define CADV_CLEAR_B 48
#define CADV_TREE_R 26
#define CADV_TREE_G 140
#define CADV_TREE_B 60

/* Requires a current GL context. */
bool cadv_renderer_init(cadv_renderer *r, cadv_gl_profile profile);
void cadv_renderer_frame(cadv_renderer *r, int width, int height);
/* Reads the current back buffer into img (allocated here, row 0 on top). */
bool cadv_renderer_capture(cadv_renderer *r, int width, int height, cadv_image *img);
void cadv_renderer_shutdown(cadv_renderer *r);

#endif

/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_RENDERER_H
#define CADV_GFX_RENDERER_H

#include <stdbool.h>

#include "core/image.h"
#include "gfx/gl_util.h"
#include "gfx/palette.h"
#include "gfx/scene.h"
#include "gfx/shader_src.h"
#include "gfx/view.h"

/* Low-res forward renderer:
 *   shadow passes (sun + up to 4 cube-mapped point lights)
 *   -> lit scene into a low-res target (meshes, voxels, billboard sprites)
 *   -> post pass (ordered dither + 256-colour palette LUT)
 *   -> UI overlay in low-res pixels
 *   -> nearest-neighbour blit into the window viewport. */

#define CADV_MAX_VIEWS 4

typedef struct cadv_render_view {
    cadv_rect viewport;        /* in window pixels (bottom-left origin) */
    const cadv_camera *camera;
    const cadv_scene *scene;
    const cadv_ui *ui;         /* may be NULL */
} cadv_render_view;

typedef struct cadv_render_stats {
    int draw_calls;
    int triangles;
    int shadow_lights;
} cadv_render_stats;

typedef struct cadv_renderer {
    cadv_gl_profile profile;
    int internal_height;
    unsigned prog_lit, prog_depth, prog_post, prog_ui;
    unsigned tex_white, tex_font, tex_lut, tex_lut_identity;
    cadv_shadow2d sun;
    cadv_shadowcube cubes[CADV_MAX_SHADOW_LIGHTS];
    cadv_rt scene_rt[CADV_MAX_VIEWS];
    cadv_rt post_rt[CADV_MAX_VIEWS];
    cadv_gpu_mesh sprite_mesh;
    cadv_gpu_mesh ui_mesh;
    cadv_mesh scratch;
    unsigned empty_vao;
    cadv_palette palette;
    cadv_render_stats stats;
    bool ready;
} cadv_renderer;

bool cadv_renderer_init(cadv_renderer *r, cadv_gl_profile profile, int internal_height);
/* Replaces the palette used for quantisation (rebuilds the LUT). */
void cadv_renderer_set_palette(cadv_renderer *r, const cadv_palette *p);
/* Renders all views into the default framebuffer of a window_w x window_h window. */
void cadv_renderer_render(cadv_renderer *r, int window_w, int window_h,
                          const cadv_render_view *views, int nviews);
/* Low-res target size used for a viewport (for UI layout before rendering). */
void cadv_renderer_target_size(const cadv_renderer *r, cadv_rect viewport, int *w, int *h);
/* Reads the current back buffer into img (allocated here, row 0 on top). */
bool cadv_renderer_capture(cadv_renderer *r, int width, int height, cadv_image *img);
void cadv_renderer_shutdown(cadv_renderer *r);

#endif

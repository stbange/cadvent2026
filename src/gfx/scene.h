/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_SCENE_H
#define CADV_GFX_SCENE_H

#include <stdbool.h>
#include <stdint.h>

#include "core/mathx.h"
#include "gfx/gl_util.h"
#include "gfx/view.h"

/* What a portal hands to the renderer each frame. Pure data, no GL calls, so
 * game code can build scenes in headless tests too. */

#define CADV_MAX_DRAWS 1024
#define CADV_MAX_SPRITES 512
#define CADV_MAX_LIGHTS 32
#define CADV_MAX_SHADOW_LIGHTS 4
#define CADV_UI_MAX_QUADS 2048

enum {
    CADV_MAT_UNLIT = 1 << 0,
    CADV_MAT_ALPHA_TEST = 1 << 1,
    CADV_MAT_FLAT = 1 << 2,
    CADV_MAT_NO_SHADOW = 1 << 3, /* does not cast shadows */
    CADV_MAT_NO_FOG = 1 << 4
};

typedef struct cadv_material {
    unsigned tex;       /* 0 = white */
    cadv_vec4 tint;     /* multiplies texture and vertex colour */
    float emissive;     /* adds tex*tint*emissive regardless of light */
    uint32_t flags;
} cadv_material;

typedef struct cadv_draw {
    const cadv_gpu_mesh *mesh;
    cadv_mat4 model;
    cadv_material mat;
} cadv_draw;

typedef struct cadv_sprite {
    cadv_vec3 pos;      /* bottom centre */
    float w, h;         /* world size */
    unsigned tex;       /* colour atlas */
    unsigned normal_tex;/* optional normal atlas (0 = flat facing camera) */
    float u0, v0, u1, v1;
    cadv_vec4 tint;
    float emissive;
    bool casts_shadow;
} cadv_sprite;

typedef struct cadv_light {
    cadv_vec3 pos;
    cadv_vec3 color;    /* may exceed 1 for bright lights */
    float radius;       /* influence ends here */
    bool casts_shadow;
} cadv_light;

typedef struct cadv_scene {
    cadv_draw draws[CADV_MAX_DRAWS];
    int ndraws;
    cadv_sprite sprites[CADV_MAX_SPRITES];
    int nsprites;
    cadv_light lights[CADV_MAX_LIGHTS];
    int nlights;
    cadv_vec3 ambient;
    cadv_vec3 sun_dir;       /* direction the light travels */
    cadv_vec3 sun_color;     /* zero = no sun */
    bool sun_shadow;
    cadv_vec3 shadow_center; /* sun shadow box centre and half extent */
    float shadow_extent;
    cadv_vec3 fog_color;
    float fog_near, fog_far; /* far <= near disables fog */
    cadv_vec3 clear_color;
    int light_bands;         /* 0 = smooth lighting */
    float dither;            /* 0..1 ordered dither strength before palette */
    bool use_palette;
} cadv_scene;

void cadv_scene_reset(cadv_scene *s);
cadv_material cadv_material_make(unsigned tex, cadv_vec4 tint, float emissive, uint32_t flags);
/* Return false when the scene is full. */
bool cadv_scene_draw(cadv_scene *s, const cadv_gpu_mesh *mesh, cadv_mat4 model, cadv_material mat);
bool cadv_scene_sprite(cadv_scene *s, const cadv_sprite *sp);
bool cadv_scene_light(cadv_scene *s, cadv_vec3 pos, cadv_vec3 color, float radius, bool shadow);

/* --- UI overlay in low-res pixel coordinates (origin top-left) ------------ */
typedef struct cadv_ui_quad {
    float x, y, w, h;   /* pixels */
    float u0, v0, u1, v1; /* font atlas UVs (solid block for rectangles) */
    cadv_vec4 color;
} cadv_ui_quad;

typedef struct cadv_ui {
    cadv_ui_quad q[CADV_UI_MAX_QUADS];
    int count;
    int width, height; /* target size, set by the renderer before drawing */
} cadv_ui;

void cadv_ui_reset(cadv_ui *ui, int width, int height);
void cadv_ui_rect(cadv_ui *ui, float x, float y, float w, float h, cadv_vec4 color);
/* Draws text with the built-in font at integer 'scale'. Returns width drawn. */
int cadv_ui_text(cadv_ui *ui, float x, float y, int scale, cadv_vec4 color, const char *text);
/* Horizontally centred text around cx. */
void cadv_ui_text_centered(cadv_ui *ui, float cx, float y, int scale, cadv_vec4 color, const char *text);

cadv_vec4 cadv_v4(float x, float y, float z, float w);

#endif

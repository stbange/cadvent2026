/* SPDX-License-Identifier: MIT */
/* GPU side of Day 8: grey station panels and hazard stripes, frosted labs
 * with blinking consoles, the monorail car, the resonance emitter (white,
 * then violet after the cascade), people, soldiers, skitterlings, and the
 * antenna array in a blizzard with the thing under the ice. */
#include "portals/p08_cascade_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portal/help.h"
#include "portals/p08_internal.h"

#define CC(c) (((float)(c) + 0.5f) * P08_CELL)

static unsigned make_tex(cadv_tex_kind kind, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, kind, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

static unsigned sign_tex(const char *text, int *w_out, cadv_rgba bg, cadv_rgba fg)
{
    cadv_texdata t;
    unsigned tex;
    int len = (int)strlen(text);
    int w;
    int i;
    int x;
    int y;
    if (len > 48) {
        len = 48;
    }
    w = 6 * len + 6;
    cadv_texdata_alloc(&t, w, 12);
    for (i = 0; i < w * 12; ++i) {
        t.rgba[i * 4] = bg.r;
        t.rgba[i * 4 + 1] = bg.g;
        t.rgba[i * 4 + 2] = bg.b;
        t.rgba[i * 4 + 3] = 255;
    }
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y)) {
                    cadv_texdata_set(&t, 3 + i * 6 + x, 2 + y, fg.r, fg.g, fg.b, 255);
                }
            }
        }
    }
    tex = cadv_gl_texture(&t, false);
    *w_out = w;
    cadv_texdata_free(&t);
    return tex;
}

static void box(cadv_mesh *m, float x0, float y0, float z0, float x1, float y1, float z1, float uv, cadv_rgba c)
{
    cadv_mesh_box(m, cadv_v3(x0, y0, z0), cadv_v3(x1, y1, z1), uv, c, false);
}

void cadv_p08_gpu_init(void *state)
{
    p08_state *s = (p08_state *)state;
    p08_gpu *g = &s->g;
    cadv_mesh m;
    cadv_mesh props;
    cadv_voxels v;
    cadv_rgba white = cadv_rgba_make(255, 255, 255, 255);
    int i;
    int x;
    int z;
    memset(g, 0, sizeof *g);
    g->tex_panel = make_tex(CADV_TEX_METAL, 81u, cadv_rgba_make(150, 155, 162, 255), cadv_rgba_make(118, 122, 130, 255));
    g->tex_floor = make_tex(CADV_TEX_FLOOR_TILES, 82u, cadv_rgba_make(95, 100, 108, 255), cadv_rgba_make(75, 78, 85, 255));
    g->tex_snow = make_tex(CADV_TEX_NOISE, 83u, cadv_rgba_make(225, 232, 240, 255), cadv_rgba_make(245, 248, 252, 255));
    g->tex_sign[0] = sign_tex(s->mark_text[0], &g->sign_w[0], cadv_rgba_make(230, 200, 40, 255), cadv_rgba_make(20, 20, 20, 255));
    g->tex_sign[1] = sign_tex(s->mark_text[1], &g->sign_w[1], cadv_rgba_make(40, 60, 70, 255), cadv_rgba_make(150, 255, 200, 255));
    g->tex_sign[2] = sign_tex(s->mark_text[2], &g->sign_w[2], cadv_rgba_make(225, 230, 235, 255), cadv_rgba_make(30, 60, 120, 255));
    cadv_mesh_init(&m);
    cadv_mesh_init(&props);
    /* floors: tiles indoors, snow on the array */
    cadv_mesh_quad(&m, cadv_v3(0, 0, (float)P08_H * P08_CELL), cadv_v3(63.0f * P08_CELL, 0, (float)P08_H * P08_CELL), cadv_v3(63.0f * P08_CELL, 0, 0), cadv_v3(0, 0, 0), 60.0f, 22.0f, white);
    cadv_gpu_mesh_upload(&g->floor, &m, false);
    cadv_mesh_clear(&m);
    /* walls with a hazard band at the foot, and the ceiling */
    for (z = 0; z < P08_H; ++z) {
        for (x = 0; x < P08_W;) {
            int x1 = x;
            if (s->cells[z * P08_W + x] != 1) {
                ++x;
                continue;
            }
            while (x1 + 1 < P08_W && s->cells[z * P08_W + x1 + 1] == 1) {
                ++x1;
            }
            box(&m, (float)x * P08_CELL, 0, (float)z * P08_CELL, (float)(x1 + 1) * P08_CELL, x >= 63 ? 6.0f : P08_WALL_H, (float)(z + 1) * P08_CELL, 0.5f, white);
            for (i = x; i <= x1; ++i) {
                box(&props, (float)i * P08_CELL - 0.02f, 0.0f, (float)z * P08_CELL - 0.02f, (float)i * P08_CELL + 1.0f, 0.3f, (float)(z + 1) * P08_CELL + 0.02f, 0.0f,
                    cadv_rgba_make(230, 190, 30, 255));
                box(&props, (float)i * P08_CELL + 1.0f, 0.0f, (float)z * P08_CELL - 0.02f, (float)(i + 1) * P08_CELL + 0.02f, 0.3f, (float)(z + 1) * P08_CELL + 0.02f, 0.0f,
                    cadv_rgba_make(30, 30, 30, 255));
            }
            x = x1 + 1;
        }
    }
    cadv_gpu_mesh_upload(&g->walls, &m, false);
    cadv_mesh_clear(&m);
    box(&m, 0, P08_WALL_H, 0, 63.0f * P08_CELL, P08_WALL_H + 0.2f, (float)P08_H * P08_CELL, 0.5f, cadv_rgba_make(120, 125, 132, 255));
    for (x = 2; x < 62; x += 4) {
        box(&m, (float)x * P08_CELL, P08_WALL_H - 0.08f, 0, (float)x * P08_CELL + 0.5f, P08_WALL_H, (float)P08_H * P08_CELL, 0.0f, cadv_rgba_make(255, 250, 230, 255)); /* light strips */
    }
    cadv_gpu_mesh_upload(&g->ceiling, &m, false);
    cadv_mesh_clear(&m);
    /* consoles and crates, the generator, the antenna tower and its dish */
    for (z = 0; z < P08_H; ++z) {
        for (x = 0; x < P08_W; ++x) {
            if (s->cells[z * P08_W + x] == 2 && !(x >= 70 && x <= 71) && !(x <= 41 && z >= 18)) {
                float x0 = (float)x * P08_CELL + 0.1f;
                float z0 = (float)z * P08_CELL + 0.1f;
                box(&props, x0, 0, z0, x0 + 1.8f, 1.2f, z0 + 1.8f, 0.0f, cadv_rgba_make(90, 95, 105, 255));
                box(&props, x0 + 0.2f, 1.2f, z0 + 0.2f, x0 + 1.6f, 1.3f, z0 + 1.6f, 0.0f, cadv_rgba_make(40, 140, 120, 255));
            }
        }
    }
    box(&props, 40.0f * P08_CELL, 0, 18.0f * P08_CELL, 42.0f * P08_CELL, 1.6f, 21.0f * P08_CELL, 0.0f, cadv_rgba_make(100, 100, 110, 255)); /* the ledge */
    box(&props, 70.0f * P08_CELL + 0.6f, 0, 9.0f * P08_CELL + 0.6f, 72.0f * P08_CELL - 0.6f, 14.0f, 11.0f * P08_CELL - 0.6f, 0.0f, cadv_rgba_make(150, 60, 50, 255));
    for (i = 0; i < 6; ++i) {
        float y = 2.0f + (float)i * 2.0f;
        box(&props, 70.0f * P08_CELL, y, 9.0f * P08_CELL, 72.0f * P08_CELL, y + 0.2f, 11.0f * P08_CELL, 0.0f, cadv_rgba_make(200, 200, 205, 255));
    }
    box(&props, 70.0f * P08_CELL - 2.0f, 14.0f, 9.0f * P08_CELL - 2.0f, 72.0f * P08_CELL + 2.0f, 14.4f, 11.0f * P08_CELL + 2.0f, 0.0f, cadv_rgba_make(220, 225, 230, 255));
    for (i = 0; i < P08_STEPS; ++i) {
        box(&props, (float)(66 + i) * P08_CELL, 0, 17.0f * P08_CELL, (float)(67 + i) * P08_CELL, 0.45f * (float)(i + 1), 19.0f * P08_CELL, 0.0f, cadv_rgba_make(110, 115, 125, 255));
    }
    box(&props, 72.0f * P08_CELL, 0, 16.0f * P08_CELL, 76.0f * P08_CELL, 0.45f * (float)P08_STEPS, 19.0f * P08_CELL, 0.0f, cadv_rgba_make(110, 115, 125, 255));
    cadv_gpu_mesh_upload(&g->props, &props, false);
    cadv_mesh_free(&props);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, white, false);
    cadv_gpu_mesh_upload(&g->cube, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f, white);
    cadv_gpu_mesh_upload(&g->plaque, &m, false);
    cadv_mesh_clear(&m);
    /* the monorail car */
    box(&m, -3.0f, 0.0f, -3.6f, 3.0f, 0.2f, 3.6f, 0.0f, cadv_rgba_make(70, 75, 85, 255));
    box(&m, -3.0f, 2.8f, -3.6f, 3.0f, 3.1f, 3.6f, 0.0f, cadv_rgba_make(200, 200, 205, 255));
    box(&m, -3.0f, 0.2f, -3.6f, 3.0f, 1.0f, -3.3f, 0.0f, cadv_rgba_make(40, 90, 150, 255));
    box(&m, -3.0f, 0.2f, 3.3f, 3.0f, 1.0f, 3.6f, 0.0f, cadv_rgba_make(40, 90, 150, 255));
    box(&m, -3.0f, 0.0f, -0.3f, 3.0f, -0.4f, 0.3f, 0.0f, cadv_rgba_make(50, 50, 55, 255));
    cadv_gpu_mesh_upload(&g->tram, &m, false);
    cadv_mesh_clear(&m);
    box(&m, -0.9f, 0, -0.9f, 0.9f, 0.8f, 0.9f, 1.0f, cadv_rgba_make(160, 120, 70, 255));
    cadv_gpu_mesh_upload(&g->crate, &m, false);
    cadv_mesh_free(&m);
    /* skitterlings: snow-white crab things, a voxel model */
    cadv_voxels_alloc(&v, 9, 4, 9);
    v.pal[1] = cadv_rgba_make(235, 240, 250, 255);
    v.pal[2] = cadv_rgba_make(200, 60, 200, 255);
    for (z = 2; z <= 6; ++z) {
        for (x = 2; x <= 6; ++x) {
            cadv_voxels_set(&v, x, 1, z, 1);
            cadv_voxels_set(&v, x, 2, z, 1);
        }
    }
    for (i = 0; i < 4; ++i) {
        cadv_voxels_set(&v, i % 2 ? 8 : 0, 0, 2 + i * 2 > 8 ? 8 : 2 + i, 1);
        cadv_voxels_set(&v, i % 2 ? 7 : 1, 1, 2 + i, 1);
    }
    cadv_voxels_set(&v, 3, 2, 1, 2);
    cadv_voxels_set(&v, 5, 2, 1, 2);
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.1f, &m);
    cadv_gpu_mesh_upload(&g->skitter, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    /* the Shining Star */
    cadv_voxels_alloc(&v, 15, 15, 3);
    v.pal[1] = cadv_rgba_make(255, 225, 100, 255);
    for (z = 0; z < 15; ++z) {
        for (x = 0; x < 15; ++x) {
            float px = (float)x - 7.0f;
            float py = (float)z - 7.0f;
            float a = cadv_atan2f(py, px) + 0.5f * CADV_PI;
            float rr = cadv_sqrtf(px * px + py * py);
            float k = cadv_cosf(5.0f * a);
            if (rr <= 3.4f + 3.6f * (k > 0.0f ? k * k : 0.0f)) {
                cadv_voxels_set(&v, x, z, 0, 1);
                cadv_voxels_set(&v, x, z, 1, 1);
                cadv_voxels_set(&v, x, z, 2, 1);
            }
        }
    }
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.08f, &m);
    cadv_gpu_mesh_upload(&g->star, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    s->gpu_ready = true;
}

void cadv_p08_gpu_free(void *state)
{
    p08_state *s = (p08_state *)state;
    p08_gpu *g = &s->g;
    cadv_gpu_mesh_free(&g->floor);
    cadv_gpu_mesh_free(&g->walls);
    cadv_gpu_mesh_free(&g->ceiling);
    cadv_gpu_mesh_free(&g->props);
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->tram);
    cadv_gpu_mesh_free(&g->skitter);
    cadv_gpu_mesh_free(&g->crate);
    cadv_gl_delete_textures(1, &g->tex_panel);
    cadv_gl_delete_textures(1, &g->tex_floor);
    cadv_gl_delete_textures(1, &g->tex_snow);
    cadv_gl_delete_textures(3, g->tex_sign);
    s->gpu_ready = false;
}

/* ---- draw ------------------------------------------------------------------------------ */

static void cube(cadv_scene *sc, const p08_gpu *g, cadv_mat4 m, cadv_vec4 c, uint32_t flags)
{
    cadv_scene_draw(sc, &g->cube, m, cadv_material_make(0, c, 0.0f, flags));
}

static cadv_mat4 at(cadv_mat4 base, float x, float y, float z, float sx, float sy, float sz)
{
    return cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(x, y, z)), cadv_m4_scale(cadv_v3(sx, sy, sz))));
}

static cadv_mat4 at_world(float x, float y, float z, float sx, float sy, float sz) { return at(cadv_m4_identity(), x, y, z, sx, sy, sz); }

/* A person: coat, legs and head from boxes, tinted by role. */
static void person(cadv_scene *sc, const p08_gpu *g, cadv_vec3 p, float yaw, cadv_vec4 coat, cadv_vec4 legs, cadv_vec4 head, bool helmet, float walk)
{
    cadv_mat4 base = cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(yaw));
    float sw = cadv_sinf(walk) * 0.4f;
    cube(sc, g, cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.12f, 0.85f, 0)), cadv_m4_mul(cadv_m4_rotate_x(sw), cadv_m4_mul(cadv_m4_translate(cadv_v3(0, -0.42f, 0)), cadv_m4_scale(cadv_v3(0.16f, 0.85f, 0.18f)))))), legs, 0);
    cube(sc, g, cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.12f, 0.85f, 0)), cadv_m4_mul(cadv_m4_rotate_x(-sw), cadv_m4_mul(cadv_m4_translate(cadv_v3(0, -0.42f, 0)), cadv_m4_scale(cadv_v3(0.16f, 0.85f, 0.18f)))))), legs, 0);
    cube(sc, g, at(base, 0, 1.2f, 0, 0.48f, 0.72f, 0.28f), coat, 0);
    cube(sc, g, at(base, 0, 1.68f, 0, 0.26f, 0.28f, 0.26f), head, 0);
    if (helmet) {
        cube(sc, g, at(base, 0, 1.72f, -0.1f, 0.28f, 0.1f, 0.1f), cadv_v4(0.9f, 0.95f, 1.0f, 1), 0); /* the visor */
    }
}

static void draw_world(p08_state *s, cadv_scene *sc, const cadv_camera *cam, float t)
{
    p08_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    bool cascade = s->cascade_t > 0;
    float pulse = 0.6f + 0.4f * cadv_sinf(t * 5.0f);
    int lights = 0;
    int i;
    cadv_scene_draw(sc, &g->floor, cadv_m4_identity(), cadv_material_make(g->tex_floor, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->walls, cadv_m4_identity(), cadv_material_make(g->tex_panel, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->ceiling, cadv_m4_identity(), cadv_material_make(0, white, cascade ? 0.0f : 0.4f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->props, cadv_m4_identity(), cadv_material_make(0, white, 0.0f, 0));
    /* the array's snowfield */
    cube(sc, g, at_world(70.5f * P08_CELL + 1.0f, -0.02f, 11.0f * P08_CELL, 16.0f * P08_CELL, 0.04f, 17.0f * P08_CELL), cadv_v4(0.92f, 0.95f, 1.0f, 1), CADV_MAT_NO_SHADOW);
    /* doors that are still shut */
    {
        static const cadv_vec4 k_door = {0.55f, 0.6f, 0.68f, 1.0f};
        int doors[5] = {s->s_chamber, s->s_breach, s->s_gate, s->s_lab, s->s_scanner};
        for (i = 0; i < 5; ++i) {
            if (s->world.s[doors[i]].enabled) {
                cadv_aabb b = s->world.s[doors[i]].box;
                cube(sc, g, at_world(0.5f * (b.lo.x + b.hi.x), 0.5f * (b.lo.y + b.hi.y), 0.5f * (b.lo.z + b.hi.z), b.hi.x - b.lo.x, b.hi.y - b.lo.y, b.hi.z - b.lo.z),
                     i == 1 ? cadv_v4(0.45f, 0.45f, 0.48f, 1) : k_door, 0);
            }
        }
    }
    /* signs: the test chamber door, the maintenance bot, radiology */
    cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(31.0f * P08_CELL - 0.03f, 2.4f, 22.0f)), cadv_m4_mul(cadv_m4_rotate_y(-0.5f * CADV_PI), cadv_m4_scale(cadv_v3((float)g->sign_w[0] / 22.0f, 0.5f, 1)))),
                    cadv_material_make(g->tex_sign[0], white, 0.2f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(42.5f * P08_CELL, 2.6f, 4.0f * P08_CELL + 1.95f)), cadv_m4_mul(cadv_m4_rotate_y(CADV_PI), cadv_m4_scale(cadv_v3((float)g->sign_w[2] / 22.0f, 0.5f, 1)))),
                    cadv_material_make(g->tex_sign[2], white, 0.3f, CADV_MAT_NO_SHADOW));
    /* the monorail car and the sample cart */
    cadv_scene_draw(sc, &g->tram, cadv_m4_translate(cadv_v3(s->tram_x, 0, 22.0f)), cadv_material_make(0, white, 0.1f, 0));
    cube(sc, g, at_world(s->cart_x, 0.5f, 22.0f, 1.2f, 0.8f, 0.8f), cadv_v4(0.7f, 0.72f, 0.75f, 1), 0);
    cube(sc, g, at_world(s->cart_x, 1.05f, 22.0f, 0.4f, 0.3f, 0.4f), cascade ? cadv_v4(1.4f, 0.5f, 1.6f, 1) : cadv_v4(0.6f, 1.6f, 1.4f, 1), CADV_MAT_UNLIT);
    /* the emitter beam */
    {
        cadv_vec4 beam = cascade ? cadv_v4(1.4f * pulse, 0.4f, 1.8f * pulse, 1) : cadv_v4(0.9f, 1.4f, 1.6f, 1);
        cube(sc, g, at_world(CC(37) + 1.0f, 2.0f, 22.0f, 0.5f, 4.0f, 0.5f), beam, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
        cadv_scene_light(sc, cadv_v3(CC(37), 2.0f, 22.0f), cascade ? cadv_v3(1.6f * pulse, 0.4f, 2.0f * pulse) : cadv_v3(0.7f, 1.2f, 1.4f), cascade ? 18.0f : 10.0f, true);
        ++lights;
    }
    cube(sc, g, at_world(s->crate_x, 0.4f, s->crate_z, 1.8f, 0.8f, 1.8f), cadv_v4(0.65f, 0.48f, 0.3f, 1), 0);
    cube(sc, g, at_world(CC(29), 1.1f, CC(6) - 0.6f, 1.2f, 2.2f, 0.6f), s->suit ? cadv_v4(0.4f, 0.42f, 0.45f, 1) : cadv_v4(0.9f, 0.5f, 0.1f, 1), 0); /* suit locker */
    cube(sc, g, at_world(CC(47), 0.8f, CC(6), 1.6f, 1.6f, 1.6f), s->generator_on ? cadv_v4(0.3f, 1.4f, 0.5f, 1) : cadv_v4(0.5f, 0.3f, 0.2f, 1), s->generator_on ? CADV_MAT_UNLIT : 0);
    cube(sc, g, at_world(CC(61) + 0.95f, 1.5f, CC(9), 0.1f, 0.4f, 0.3f), s->scanner_open ? cadv_v4(0.3f, 1.6f, 0.4f, 1) : cadv_v4(1.6f, 0.3f, 0.2f, 1), CADV_MAT_UNLIT);
    for (i = 0; i < 3; ++i) {
        cadv_vec3 c = s->charger_pos[i];
        float f = s->chargers[i].reserve / 75.0f;
        cube(sc, g, at_world(c.x, 1.2f, c.z, 0.6f, 0.8f, 0.6f), cadv_v4(0.3f + 0.2f * f, 0.6f + 0.8f * f, 1.0f, 1), CADV_MAT_UNLIT);
    }
    {
        static const int k_breakers[3][2] = {{65, 5}, {77, 11}, {74, 17}};
        for (i = 0; i < 3; ++i) {
            bool on = (s->breakers >> i) & 1;
            float y = i == 2 ? 0.45f * (float)P08_STEPS : 0.0f;
            cube(sc, g, at_world(CC(k_breakers[i][0]), y + 0.9f, CC(k_breakers[i][1]), 0.6f, 1.8f, 0.6f), cadv_v4(0.35f, 0.35f, 0.4f, 1), 0);
            cube(sc, g, at_world(CC(k_breakers[i][0]), y + 1.4f, CC(k_breakers[i][1]) - 0.35f, 0.2f, 0.2f, 0.2f), on ? cadv_v4(0.3f, 1.8f, 0.4f, 1) : cadv_v4(1.8f, 0.4f, 0.2f, 1), CADV_MAT_UNLIT);
        }
    }
    /* console lights blink everywhere; after the cascade, alarms */
    for (i = 0; i < P08_W * P08_H && lights < 22; i += 13) {
        int cx = i % P08_W;
        int cz = i / P08_W;
        if (s->cells[cz * P08_W + cx] == 2 && cx < 63) {
            cadv_vec3 p = cadv_v3(CC(cx), 1.6f, CC(cz));
            if (cadv_v3_dist(p, cam->pos) < 26.0f) {
                cadv_vec3 c = cascade ? cadv_v3(1.4f * pulse, 0.2f, 0.3f) : cadv_v3(0.3f, 0.9f, 0.8f);
                cadv_scene_light(sc, p, c, 6.0f, false);
                ++lights;
            }
        }
    }
    /* people */
    for (i = 0; i < P08_NPCS; ++i) {
        const p08_npc *n = &s->npc[i];
        float walk = (float)s->ticks * 0.15f * (n->f.path_len > 0 ? 1.0f : 0.0f);
        if (!n->alive) {
            continue;
        }
        if (i == P08_NPC_C6P) {
            cadv_mat4 b = cadv_m4_mul(cadv_m4_translate(n->f.pos), cadv_m4_rotate_y(n->yaw));
            cube(sc, g, at(b, 0, 0.35f, 0, 0.7f, 0.5f, 0.9f), cadv_v4(0.85f, 0.75f, 0.3f, 1), 0);
            cube(sc, g, at(b, 0, 0.75f, -0.2f, 0.3f, 0.3f, 0.3f), cadv_v4(0.3f, 0.3f, 0.35f, 1), 0);
            cube(sc, g, at(b, 0, 0.8f, -0.36f, 0.2f, 0.06f, 0.02f), cadv_v4(0.4f, 1.6f, 1.0f, 1), CADV_MAT_UNLIT);
            cube(sc, g, at(b, 0, 0.06f, -0.55f, 0.8f, 0.05f, 0.15f), cadv_v4(0.2f, 0.2f, 0.25f, 1), 0); /* the brush */
            continue;
        }
        person(sc, g, n->f.pos, n->yaw, i == P08_NPC_BRANDT ? cadv_v4(0.2f, 0.3f, 0.55f, 1) : cadv_v4(0.92f, 0.92f, 0.95f, 1),
               cadv_v4(0.25f, 0.25f, 0.3f, 1), n->hurt > 0 ? cadv_v4(1.4f, 0.5f, 0.5f, 1) : cadv_v4(0.9f, 0.72f, 0.6f, 1), i == P08_NPC_BRANDT, walk);
    }
    /* enemies */
    for (i = 0; i < P08_ENEMIES; ++i) {
        const p08_enemy *e = &s->e[i];
        if (!e->active || e->hidden_until_scan) {
            continue;
        }
        if (e->kind == P08_E_SKITTER) {
            cadv_scene_draw(sc, &g->skitter, cadv_m4_mul(cadv_m4_translate(cadv_v3(e->pos.x - 0.45f, e->pos.y, e->pos.z - 0.45f)), cadv_m4_rotate_y(e->yaw)),
                            cadv_material_make(0, e->hurt > 0 ? cadv_v4(1.6f, 0.6f, 0.6f, 1) : white, 0.3f, 0));
        } else {
            person(sc, g, e->pos, e->yaw, cadv_v4(0.12f, 0.13f, 0.16f, 1), cadv_v4(0.1f, 0.1f, 0.12f, 1),
                   e->hurt > 0 ? cadv_v4(1.4f, 0.4f, 0.4f, 1) : cadv_v4(0.15f, 0.15f, 0.18f, 1), true, (float)s->ticks * 0.12f);
        }
    }
    /* pickups */
    for (i = 0; i < P08_PICKUPS; ++i) {
        const p08_pickup *p = &s->pk[i];
        cadv_vec4 c;
        if (!p->active) {
            continue;
        }
        c = p->kind == P08_PK_MEDKIT ? cadv_v4(1.0f, 1.0f, 1.0f, 1) : p->kind == P08_PK_CELL ? cadv_v4(0.4f, 1.6f, 1.6f, 1)
            : p->kind == P08_PK_AMMO ? cadv_v4(0.7f, 0.6f, 0.3f, 1) : cadv_v4(0.4f, 0.4f, 0.45f, 1);
        cube(sc, g, cadv_m4_mul(cadv_m4_translate(cadv_v3(p->pos.x, p->pos.y + 0.1f * cadv_sinf(t * 3.0f + (float)i), p->pos.z)),
                                cadv_m4_mul(cadv_m4_rotate_y(t + (float)i), cadv_m4_scale(cadv_v3(0.4f, 0.25f, 0.6f)))),
             c, p->kind == P08_PK_CELL ? CADV_MAT_UNLIT : 0);
    }
    for (i = 0; i < P08_NADES; ++i) {
        if (s->nades[i].active) {
            cube(sc, g, at_world(s->nades[i].pos.x, s->nades[i].pos.y, s->nades[i].pos.z, 0.15f, 0.15f, 0.15f), cadv_v4(0.3f, 0.35f, 0.3f, 1), 0);
        }
    }
    /* the array: falling snow, the tentacle's warning and strike */
    if (s->act == P08_ACT_ARRAY || cam->pos.x > 62.0f * P08_CELL) {
        for (i = 0; i < 60; ++i) {
            float ph = t * (1.0f + (float)(i % 5) * 0.2f) + (float)i * 0.37f;
            float fy = 8.0f - (ph - (float)(int)ph) * 8.0f;
            float fx = cam->pos.x + (float)((i * 37) % 30) - 15.0f + cadv_sinf(ph) * 1.5f;
            float fz = cam->pos.z + (float)((i * 53) % 30) - 15.0f;
            cube(sc, g, at_world(fx, fy, fz, 0.05f, 0.05f, 0.05f), cadv_v4(1.2f, 1.2f, 1.3f, 1), CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
        }
        if (s->slam.active) {
            float k = 1.0f - (float)s->slam.timer / 70.0f;
            cube(sc, g, at_world(s->slam.pos.x, 0.03f, s->slam.pos.z, 6.0f * k + 0.5f, 0.02f, 6.0f * k + 0.5f), cadv_v4(1.6f, 0.3f, 0.2f, 1), CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
            if (s->slam.timer < 20) {
                int seg;
                for (seg = 0; seg < 6; ++seg) {
                    float y = 9.0f - (float)seg * 1.5f - (float)(20 - s->slam.timer) * 0.3f;
                    cube(sc, g, at_world(s->slam.pos.x, cadv_maxf(0.6f, y), s->slam.pos.z, 1.4f - (float)seg * 0.1f, 1.6f, 1.4f - (float)seg * 0.1f), cadv_v4(0.55f, 0.35f, 0.6f, 1), 0);
                }
            }
        }
    }
    if (s->star_active) {
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(s->star_pos), cadv_m4_rotate_y(t * 2.0f)), cadv_material_make(0, white, 1.3f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, s->star_pos, cadv_v3(1.8f, 1.6f, 0.6f), 14.0f, false);
    }
}

static void draw_hud(const p08_state *s, cadv_ui *ui)
{
    float W = (float)ui->width;
    float H = (float)ui->height;
    float cx = W * 0.5f;
    float cy = H * 0.5f;
    cadv_vec4 amber = cadv_v4(1, 0.7f, 0.25f, 1);
    char buf[96];
    int i;
    cadv_ui_rect(ui, cx - 4, cy, 3, 1, amber);
    cadv_ui_rect(ui, cx + 2, cy, 3, 1, amber);
    cadv_ui_rect(ui, cx, cy - 4, 1, 3, amber);
    cadv_ui_rect(ui, cx, cy + 2, 1, 3, amber);
    snprintf(buf, sizeof buf, "HEALTH %d", s->body.health);
    cadv_ui_text(ui, 4, H - 12, 1, s->body.health > 30 ? amber : cadv_v4(1, 0.3f, 0.3f, 1), buf);
    if (s->suit) {
        snprintf(buf, sizeof buf, "SUIT %d", s->body.armor);
        cadv_ui_text(ui, 80, H - 12, 1, cadv_v4(0.5f, 0.85f, 1, 1), buf);
    }
    if (s->owned[s->weapon]) {
        const p08_weapon_def *w = &cadv_p08_weapons()[s->weapon];
        if (s->weapon == P08_W_AXE) {
            snprintf(buf, sizeof buf, "%s", w->name);
        } else if (s->weapon == P08_W_GRENADE) {
            snprintf(buf, sizeof buf, "%s %d", w->name, s->reserve[s->weapon]);
        } else {
            snprintf(buf, sizeof buf, "%s %d/%d", w->name, s->mag[s->weapon], s->reserve[s->weapon]);
        }
        cadv_ui_text(ui, W - 6.0f * (float)strlen(buf) - 4, H - 12, 1, amber, buf);
    }
    {
        int follow = 0;
        for (i = 0; i < P08_NPC_C6P; ++i) {
            follow += s->npc[i].alive && s->npc[i].f.mode == CADV_FOLLOW_ON;
        }
        if (follow > 0) {
            snprintf(buf, sizeof buf, "FOLLOWING: %d", follow);
            cadv_ui_text(ui, 4, H - 22, 1, cadv_v4(0.7f, 1, 0.7f, 1), buf);
        }
    }
    if (s->cells_held > 0) {
        snprintf(buf, sizeof buf, "POWER CELLS %d/2", s->cells_held);
        cadv_ui_text(ui, 4, 4, 1, cadv_v4(0.5f, 1, 1, 1), buf);
    }
    if (s->act == P08_ACT_ARRAY) {
        snprintf(buf, sizeof buf, "BREAKERS %d/3", (s->breakers & 1) + ((s->breakers >> 1) & 1) + ((s->breakers >> 2) & 1));
        cadv_ui_text(ui, 4, 4, 1, cadv_v4(1, 0.8f, 0.4f, 1), buf);
    }
    if (s->radio_timer > 0) {
        char lines[3][64];
        int n = cadv_help_wrap(s->radio, (int)(W / 6.0f) - 2, lines, 3);
        cadv_ui_rect(ui, 0, H - 26 - 9.0f * (float)n, W, 9.0f * (float)n + 2, cadv_v4(0, 0, 0, 0.45f));
        for (i = 0; i < n; ++i) {
            cadv_ui_text_centered(ui, cx, H - 25 - 9.0f * (float)(n - i), 1, cadv_v4(1, 0.9f, 0.75f, 1), lines[i]);
        }
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx, cy - 30, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->zoom) {
        cadv_ui_rect(ui, 0, 0, cx - 50, H, cadv_v4(0, 0, 0, 1));
        cadv_ui_rect(ui, cx + 50, 0, cx - 50, H, cadv_v4(0, 0, 0, 1));
    }
    if (s->flash > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.9f, 0.8f, 1, (float)s->flash / 40.0f));
    }
    if (s->hurt_flash > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.8f, 0.1f, 0.1f, 0.025f * (float)s->hurt_flash));
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, W, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(1, 0.85f, 0.4f, 1), "THE BEAM IS REVERSED");
        cadv_ui_text_centered(ui, cx, cy + 4, 1, cadv_v4(1, 1, 1, 1), "PRESS USE TO RETURN TO THE HALL");
    }
    if (s->lose_timer > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.5f, 0.1f, 0.1f, 0.35f));
    }
}

static void draw_hands(const p08_state *s, cadv_scene *sc, const cadv_camera *cam)
{
    cadv_mat4 view = cadv_m4_mul(cadv_m4_translate(cam->pos), cadv_m4_mul(cadv_m4_rotate_y(cam->yaw), cadv_m4_rotate_x(cam->pitch)));
    float bob = cadv_sinf(s->body.stride * 3.3f) * 0.012f;
    float kick = s->shot_cd > 0.0f && s->shot_cd > cadv_p08_weapons()[s->weapon].rate - 0.08f ? 0.04f : 0.0f;
    uint32_t f = CADV_MAT_NO_SHADOW;
    if (!s->owned[s->weapon] || s->zoom) {
        return;
    }
    switch (s->weapon) {
    case P08_W_AXE:
        cube(sc, &s->g, at(view, 0.25f, -0.25f + bob, -0.45f + kick, 0.04f, 0.04f, 0.4f), cadv_v4(0.35f, 0.25f, 0.15f, 1), f);
        cube(sc, &s->g, at(view, 0.25f, -0.18f + bob, -0.65f + kick, 0.02f, 0.18f, 0.08f), cadv_v4(0.75f, 0.78f, 0.82f, 1), f);
        break;
    case P08_W_GRENADE:
        cube(sc, &s->g, at(view, 0.22f, -0.2f + bob, -0.4f, 0.08f, 0.1f, 0.08f), cadv_v4(0.3f, 0.35f, 0.3f, 1), f);
        break;
    default: {
        float len = s->weapon == P08_W_PISTOL ? 0.2f : 0.5f;
        cube(sc, &s->g, at(view, 0.2f, -0.2f + bob, -0.35f + kick - len * 0.5f, 0.06f, 0.08f, len), cadv_v4(0.2f, 0.2f, 0.22f, 1), f);
        cube(sc, &s->g, at(view, 0.2f, -0.27f + bob, -0.3f + kick, 0.05f, 0.12f, 0.06f), cadv_v4(0.25f, 0.25f, 0.28f, 1), f);
        if (s->weapon == P08_W_CROSSBOW) {
            cube(sc, &s->g, at(view, 0.2f, -0.18f + bob, -0.6f + kick, 0.4f, 0.03f, 0.04f), cadv_v4(0.3f, 0.3f, 0.32f, 1), f);
        }
        break;
    }
    }
}

void cadv_p08_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p08_state *s = (p08_state *)state;
    float t = ((float)s->ticks + alpha) * P08_DT;
    bool outside = s->body.pos.x > 62.5f * P08_CELL;
    bool cascade = s->cascade_t > 0;
    cam->pos = cadv_p08_eye(s);
    cam->yaw = s->body.yaw;
    cam->pitch = s->body.pitch;
    cam->fovy = s->zoom ? 0.4f : 1.25f;
    cam->znear = 0.05f;
    cam->zfar = 140.0f;
    if (outside) {
        sc->ambient = cadv_v3(0.4f, 0.42f, 0.5f);
        sc->sun_color = cadv_v3(0.6f, 0.65f, 0.75f);
        sc->clear_color = cadv_v3(0.55f, 0.58f, 0.66f);
        sc->fog_color = cadv_v3(0.7f, 0.73f, 0.8f);
        sc->fog_near = 10.0f;
        sc->fog_far = 55.0f; /* the blizzard */
    } else {
        float pulse = 0.5f + 0.5f * cadv_sinf(t * 5.0f);
        sc->ambient = cascade ? cadv_v3(0.18f + 0.08f * pulse, 0.12f, 0.24f + 0.1f * pulse) : cadv_v3(0.38f, 0.4f, 0.44f);
        sc->sun_color = cadv_v3(0, 0, 0);
        sc->clear_color = cadv_v3(0.05f, 0.05f, 0.07f);
        sc->fog_color = cascade ? cadv_v3(0.12f, 0.06f, 0.16f) : cadv_v3(0.2f, 0.22f, 0.25f);
        sc->fog_near = 25.0f;
        sc->fog_far = 90.0f;
    }
    sc->sun_dir = cadv_v3_norm(cadv_v3(0.3f, -0.8f, 0.4f));
    sc->sun_shadow = outside;
    sc->shadow_center = cadv_v3(cam->pos.x, 0, cam->pos.z);
    sc->shadow_extent = 30.0f;
    sc->light_bands = 6;
    sc->dither = 0.5f;
    sc->use_palette = true;
    draw_world(s, sc, cam, t);
    if (!outside) {
        /* the suit's helmet lamp */
        cadv_vec3 f = cadv_player_forward(&s->body);
        cadv_scene_light(sc, cadv_v3_add(cam->pos, cadv_v3_scale(f, 2.0f)), cascade ? cadv_v3(0.9f, 0.85f, 0.8f) : cadv_v3(0.5f, 0.5f, 0.48f), 12.0f, false);
    }
    draw_hands(s, sc, cam);
    if (ui) {
        draw_hud(s, ui);
    }
}

/* SPDX-License-Identifier: MIT */
/* GPU side of Day 10: Nivalis at night in the rain. Wet streets under
 * towers with lit window strips, neon lamps (the dynamic-light showcase),
 * holographic Halcyon ads that go dark relay by relay, the hover-car's
 * headlights throwing shadows, facades crumbling in chunks, tracers and
 * laser beams, the VTOL coming down on the plaza, and the HUD: the cell,
 * the overclock sliders, the alert, the crowd, the hacking ring and the
 * radial order ring. */
#include "portals/p10_neon_syndic_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/view.h"
#include "gfx/voxel.h"
#include "portal/help.h"
#include "portals/p10_internal.h"

#define CC(c) (((float)(c) + 0.5f) * P10_CELL)

static const char *const k_ads[4] = {"CHEER! HALCYON", "SMILE MORE", "NIVALIS LOVES YOU", "MOOD FOR ALL"};

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
    if (len > 40) {
        len = 40;
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

static void upload(cadv_gpu_mesh *g, cadv_mesh *m)
{
    cadv_gpu_mesh_upload(g, m, false);
    cadv_mesh_clear(m);
}

static float flat_d(cadv_vec3 a, cadv_vec3 b) { return cadv_sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

static uint8_t cell_at(const p10_state *s, int x, int z) { return x < 0 || z < 0 || x >= P10_W || z >= P10_H ? 1 : s->cells[z * P10_W + x]; }

static void build_facade(p10_state *s, int k)
{
    const cadv_facade *f = &s->fac[k];
    cadv_mesh m;
    int x;
    int y;
    cadv_mesh_init(&m);
    for (y = 0; y < f->h; ++y) {
        for (x = 0; x < f->w; ++x) {
            if (cadv_facade_alive(f, x, y)) {
                cadv_aabb b = s->world.s[f->solid[y * f->w + x]].box;
                int tone = 120 + ((x * 7 + y * 13) % 5) * 12;
                box(&m, b.lo.x + 0.02f, b.lo.y + 0.02f, b.lo.z + 0.02f, b.hi.x - 0.02f, b.hi.y - 0.02f, b.hi.z - 0.02f, 0.5f,
                    cadv_rgba_make((uint8_t)(tone * 3 / 4), (uint8_t)(tone * 2 / 3), (uint8_t)tone, 255));
            }
        }
    }
    cadv_gpu_mesh_free(&s->g.facade[k]);
    cadv_gpu_mesh_upload(&s->g.facade[k], &m, false);
    cadv_mesh_free(&m);
    s->g.facade_ver[k] = f->ver + 1u;
}

void cadv_p10_gpu_init(void *state)
{
    p10_state *s = (p10_state *)state;
    p10_gpu *g = &s->g;
    cadv_mesh m;
    cadv_mesh win;
    cadv_voxels v;
    cadv_rgba white = cadv_rgba_make(255, 255, 255, 255);
    char buf[80];
    int x;
    int z;
    int k;
    memset(g, 0, sizeof *g);
    g->tex_street = make_tex(CADV_TEX_NOISE, 101u, cadv_rgba_make(60, 56, 76, 255), cadv_rgba_make(84, 78, 104, 255));
    g->tex_wall = make_tex(CADV_TEX_METAL, 102u, cadv_rgba_make(60, 50, 80, 255), cadv_rgba_make(44, 38, 62, 255));
    snprintf(buf, sizeof buf, "%s - UPGRADE NOW!", s->mark_text[0]);
    g->tex_sign[0] = sign_tex(buf, &g->sign_w[0], cadv_rgba_make(20, 10, 30, 255), cadv_rgba_make(80, 255, 230, 255));
    g->tex_sign[1] = sign_tex(s->mark_text[1], &g->sign_w[1], cadv_rgba_make(10, 10, 20, 255), cadv_rgba_make(255, 200, 80, 255));
    g->tex_sign[2] = sign_tex(s->mark_text[2], &g->sign_w[2], cadv_rgba_make(50, 46, 60, 255), cadv_rgba_make(255, 90, 200, 255));
    for (k = 0; k < 4; ++k) {
        g->tex_ad[k] = sign_tex(k_ads[k], &g->ad_w[k], cadv_rgba_make(255, 70, 190, 255), cadv_rgba_make(255, 255, 255, 255));
    }
    cadv_mesh_init(&m);
    cadv_mesh_init(&win);
    cadv_mesh_quad(&m, cadv_v3(0, 0, (float)P10_H * P10_CELL), cadv_v3((float)P10_W * P10_CELL, 0, (float)P10_H * P10_CELL), cadv_v3((float)P10_W * P10_CELL, 0, 0), cadv_v3(0, 0, 0), 48.0f,
                   32.0f, white);
    upload(&g->street, &m);
    /* buildings: one box per run of equal height; lit window strips on street faces */
    for (z = 0; z < P10_H; ++z) {
        for (x = 0; x < P10_W;) {
            int x1 = x;
            uint8_t h;
            if (cell_at(s, x, z) != 1) {
                ++x;
                continue;
            }
            h = s->height[z * P10_W + x];
            while (x1 + 1 < P10_W && cell_at(s, x1 + 1, z) == 1 && s->height[z * P10_W + x1 + 1] == h) {
                ++x1;
            }
            box(&m, (float)x * P10_CELL, 0, (float)z * P10_CELL, (float)(x1 + 1) * P10_CELL, (float)h, (float)(z + 1) * P10_CELL, 0.25f, white);
            for (k = x; k <= x1; ++k) {
                int fl;
                for (fl = 1; fl < h / 3; ++fl) {
                    uint32_t r = (uint32_t)(k * 92821 + z * 68917 + fl * 1237) * 2654435761u;
                    cadv_rgba c = (r >> 28) % 3 == 0 ? cadv_rgba_make(255, 210, 120, 255) : cadv_rgba_make(90, 230, 255, 255);
                    float y0 = (float)fl * 3.0f + 0.8f;
                    if ((r >> 20) % 3 != 0) {
                        continue;
                    }
                    if (cell_at(s, k, z + 1) != 1) {
                        box(&win, (float)k * P10_CELL + 0.3f, y0, (float)(z + 1) * P10_CELL, (float)k * P10_CELL + 1.7f, y0 + 1.2f, (float)(z + 1) * P10_CELL + 0.05f, 0.0f, c);
                    }
                    if (cell_at(s, k, z - 1) != 1) {
                        box(&win, (float)k * P10_CELL + 0.3f, y0, (float)z * P10_CELL - 0.05f, (float)k * P10_CELL + 1.7f, y0 + 1.2f, (float)z * P10_CELL, 0.0f, c);
                    }
                }
            }
            x = x1 + 1;
        }
    }
    upload(&g->blocks, &m);
    upload(&g->windows, &win);
    cadv_mesh_free(&win);
    /* props: market stalls with awnings, factory machines, relay housings, planters */
    for (z = 0; z < P10_H; ++z) {
        for (x = 0; x < P10_W; ++x) {
            if (cell_at(s, x, z) == 2) {
                float x0 = (float)x * P10_CELL;
                float z0 = (float)z * P10_CELL;
                if (z > 30 && x < 34) {
                    box(&m, x0 + 0.2f, 0, z0 + 0.2f, x0 + 1.8f, 1.1f, z0 + 1.8f, 0.5f, cadv_rgba_make(120, 90, 70, 255));
                    box(&m, x0 - 0.2f, 2.2f, z0 - 0.2f, x0 + 2.2f, 2.35f, z0 + 2.2f, 0.0f, (x + z) % 2 ? cadv_rgba_make(230, 60, 140, 255) : cadv_rgba_make(60, 200, 230, 255));
                    box(&m, x0 + 0.1f, 1.1f, z0 + 0.1f, x0 + 0.2f, 2.2f, z0 + 0.2f, 0.0f, cadv_rgba_make(80, 80, 90, 255));
                    box(&m, x0 + 1.8f, 1.1f, z0 + 1.8f, x0 + 1.9f, 2.2f, z0 + 1.9f, 0.0f, cadv_rgba_make(80, 80, 90, 255));
                } else if (z < 22) {
                    box(&m, x0 + 0.1f, 0, z0 + 0.1f, x0 + 1.9f, 1.3f, z0 + 1.9f, 0.5f, cadv_rgba_make(90, 100, 110, 255));
                    box(&m, x0 + 0.5f, 1.3f, z0 + 0.5f, x0 + 1.5f, 2.6f, z0 + 1.5f, 0.5f, cadv_rgba_make(70, 76, 84, 255));
                } else {
                    box(&m, x0 + 0.1f, 0, z0 + 0.1f, x0 + 1.9f, 1.3f, z0 + 1.9f, 0.5f, cadv_rgba_make(70, 70, 80, 255));
                }
            }
        }
    }
    upload(&g->props, &m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, white, false);
    upload(&g->cube, &m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f, white);
    upload(&g->plaque, &m);
    /* the hover-car and the VTOL */
    box(&m, -1.1f, 0.5f, -2.4f, 1.1f, 1.2f, 2.4f, 0.0f, cadv_rgba_make(40, 40, 52, 255));
    box(&m, -0.9f, 1.2f, -1.0f, 0.9f, 1.75f, 1.2f, 0.0f, cadv_rgba_make(30, 60, 80, 255));
    box(&m, -1.0f, 0.75f, -2.45f, -0.6f, 0.95f, -2.4f, 0.0f, cadv_rgba_make(255, 250, 220, 255));
    box(&m, 0.6f, 0.75f, -2.45f, 1.0f, 0.95f, -2.4f, 0.0f, cadv_rgba_make(255, 250, 220, 255));
    box(&m, -1.0f, 0.75f, 2.4f, 1.0f, 0.9f, 2.45f, 0.0f, cadv_rgba_make(255, 40, 60, 255));
    box(&m, -1.0f, 0.3f, -2.0f, 1.0f, 0.4f, 2.0f, 0.0f, cadv_rgba_make(80, 200, 255, 255));
    upload(&g->car, &m);
    box(&m, -2.0f, 0.0f, -5.0f, 2.0f, 2.6f, 5.0f, 0.0f, cadv_rgba_make(60, 64, 72, 255));
    box(&m, -6.0f, 1.8f, -1.5f, 6.0f, 2.2f, 1.5f, 0.0f, cadv_rgba_make(50, 54, 60, 255));
    box(&m, -6.6f, 1.4f, -1.0f, -5.4f, 2.6f, 1.0f, 0.0f, cadv_rgba_make(80, 200, 255, 255));
    box(&m, 5.4f, 1.4f, -1.0f, 6.6f, 2.6f, 1.0f, 0.0f, cadv_rgba_make(80, 200, 255, 255));
    box(&m, -1.4f, 1.6f, -5.1f, 1.4f, 2.4f, -4.9f, 0.0f, cadv_rgba_make(255, 220, 140, 255));
    upload(&g->vtol, &m);
    cadv_mesh_free(&m);
    /* the Shining Star */
    cadv_voxels_alloc(&v, 15, 15, 3);
    v.pal[1] = cadv_rgba_make(255, 225, 100, 255);
    for (z = 0; z < 15; ++z) {
        for (x = 0; x < 15; ++x) {
            float px = (float)x - 7.0f;
            float py = (float)z - 7.0f;
            float a = cadv_atan2f(py, px) + 0.5f * CADV_PI;
            float rr = cadv_sqrtf(px * px + py * py);
            float kk = cadv_cosf(5.0f * a);
            if (rr <= 3.4f + 3.6f * (kk > 0.0f ? kk * kk : 0.0f)) {
                cadv_voxels_set(&v, x, z, 0, 1);
                cadv_voxels_set(&v, x, z, 1, 1);
                cadv_voxels_set(&v, x, z, 2, 1);
            }
        }
    }
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.2f, &m);
    cadv_gpu_mesh_upload(&g->star, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    for (k = 0; k < s->facades; ++k) {
        build_facade(s, k);
    }
    s->gpu_ready = true;
}

void cadv_p10_gpu_free(void *state)
{
    p10_state *s = (p10_state *)state;
    p10_gpu *g = &s->g;
    int k;
    cadv_gpu_mesh_free(&g->street);
    cadv_gpu_mesh_free(&g->blocks);
    cadv_gpu_mesh_free(&g->roofs);
    cadv_gpu_mesh_free(&g->windows);
    cadv_gpu_mesh_free(&g->props);
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->car);
    cadv_gpu_mesh_free(&g->vtol);
    for (k = 0; k < P10_FACADES; ++k) {
        cadv_gpu_mesh_free(&g->facade[k]);
        g->facade_ver[k] = 0;
    }
    cadv_gl_delete_textures(1, &g->tex_street);
    cadv_gl_delete_textures(1, &g->tex_wall);
    cadv_gl_delete_textures(3, g->tex_sign);
    cadv_gl_delete_textures(4, g->tex_ad);
    s->gpu_ready = false;
}

/* ---- the world ------------------------------------------------------------------------ */

static void cube(cadv_scene *sc, const p10_gpu *g, cadv_mat4 m, cadv_vec4 c, float glow, uint32_t flags)
{
    cadv_scene_draw(sc, &g->cube, m, cadv_material_make(0, c, glow, flags));
}

static cadv_mat4 at(cadv_mat4 base, float x, float y, float z, float sx, float sy, float sz)
{
    return cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(x, y, z)), cadv_m4_scale(cadv_v3(sx, sy, sz))));
}

static void person(cadv_scene *sc, const p10_gpu *g, cadv_vec3 p, float yaw, cadv_vec4 coat, cadv_vec4 head, float size, float walk, bool visor)
{
    cadv_mat4 base = cadv_m4_mul(cadv_m4_translate(p), cadv_m4_mul(cadv_m4_rotate_y(yaw), cadv_m4_scale(cadv_v3(size, size, size))));
    float sw = cadv_sinf(walk) * 0.35f;
    cadv_vec4 legs = cadv_v4(coat.x * 0.4f, coat.y * 0.4f, coat.z * 0.4f, 1);
    cube(sc, g, cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.12f, 0.85f, 0)), cadv_m4_mul(cadv_m4_rotate_x(sw), cadv_m4_mul(cadv_m4_translate(cadv_v3(0, -0.42f, 0)), cadv_m4_scale(cadv_v3(0.16f, 0.85f, 0.18f)))))), legs, 0.0f, 0);
    cube(sc, g, cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.12f, 0.85f, 0)), cadv_m4_mul(cadv_m4_rotate_x(-sw), cadv_m4_mul(cadv_m4_translate(cadv_v3(0, -0.42f, 0)), cadv_m4_scale(cadv_v3(0.16f, 0.85f, 0.18f)))))), legs, 0.0f, 0);
    cube(sc, g, at(base, 0, 1.25f, 0, 0.5f, 0.8f, 0.3f), coat, 0.0f, 0);
    cube(sc, g, at(base, 0, 1.75f, 0, 0.26f, 0.28f, 0.26f), head, 0.0f, 0);
    if (visor) {
        cube(sc, g, at(base, 0, 1.78f, -0.12f, 0.28f, 0.07f, 0.06f), cadv_v4(0.3f, 1.0f, 1.0f, 1), 1.5f, CADV_MAT_UNLIT);
    }
}

static void draw_world(p10_state *s, cadv_scene *sc, const cadv_camera *cam, float t)
{
    p10_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    int cut = (s->relay_done[0] ? 1 : 0) + (s->relay_done[1] ? 1 : 0) + (s->relay_done[2] ? 1 : 0);
    int lights = 0;
    int i;
    int k;
    cadv_scene_draw(sc, &g->street, cadv_m4_identity(), cadv_material_make(g->tex_street, white, 0.0f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->blocks, cadv_m4_identity(), cadv_material_make(g->tex_wall, cadv_v4(0.85f, 0.8f, 1.0f, 1), 0.0f, 0));
    cadv_scene_draw(sc, &g->windows, cadv_m4_identity(), cadv_material_make(0, white, 1.2f - 0.25f * (float)cut, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->props, cadv_m4_identity(), cadv_material_make(0, white, 0.0f, 0));
    for (k = 0; k < s->facades; ++k) {
        if (g->facade_ver[k] != s->fac[k].ver + 1u) {
            build_facade(s, k);
        }
        cadv_scene_draw(sc, &g->facade[k], cadv_m4_identity(), cadv_material_make(g->tex_wall, white, 0.0f, 0));
    }
    /* neon lamps: the nearest ones light the street */
    {
        int order[P10_LAMPS];
        int n = s->lamps;
        for (i = 0; i < n; ++i) {
            order[i] = i;
        }
        for (i = 1; i < n; ++i) {
            int j = i;
            while (j > 0 && flat_d(s->lamp[order[j]], cam->pos) < flat_d(s->lamp[order[j - 1]], cam->pos)) {
                int tmp = order[j];
                order[j] = order[j - 1];
                order[j - 1] = tmp;
                --j;
            }
        }
        for (i = 0; i < n; ++i) {
            cadv_vec3 p = s->lamp[order[i]];
            cadv_vec3 c = s->lamp_col[order[i]];
            float flick = (order[i] * 7 + s->ticks / 5) % 37 == 0 ? 0.3f : 1.0f;
            cube(sc, g, at(cadv_m4_identity(), p.x, p.y, p.z, 0.25f, 1.6f, 0.25f), cadv_v4(c.x, c.y, c.z, 1), 2.0f * flick, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
            if (i < 14) {
                cadv_scene_light(sc, p, cadv_v3(c.x * flick * 1.4f, c.y * flick * 1.4f, c.z * flick * 1.4f), 18.0f, false);
                ++lights;
            }
        }
    }
    /* the friends' marks: the neon ad, the codename tag over the agent, the graffiti */
    cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(37.0f, 5.0f, 64.05f)), cadv_m4_scale(cadv_v3((float)g->sign_w[0] / 9.0f, 1.4f, 1))),
                    cadv_material_make(g->tex_sign[0], white, 1.6f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(63.95f, 1.2f, 107.0f)), cadv_m4_mul(cadv_m4_rotate_y(-CADV_PI * 0.5f), cadv_m4_scale(cadv_v3((float)g->sign_w[2] / 14.0f, 0.9f, 1)))),
                    cadv_material_make(g->tex_sign[2], white, 0.4f, CADV_MAT_NO_SHADOW));
    /* holo ads along the avenue, fading as the network goes dark */
    for (k = 0; k < 4 && cut < 3; ++k) {
        float x = 80.0f + (float)k * 26.0f;
        float a = (1.0f - 0.33f * (float)cut) * (0.8f + 0.2f * cadv_sinf(t * 3.0f + (float)k));
        if ((s->ticks / 7 + k * 13) % 41 == 0) {
            continue; /* glitch */
        }
        cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(x, 12.0f + (float)(k % 2) * 4.0f, 63.9f)), cadv_m4_scale(cadv_v3((float)g->ad_w[k] / 6.0f, 2.4f, 1))),
                        cadv_material_make(g->tex_ad[k], cadv_v4(a, a, a, 1), 1.4f * a, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
    }
    /* relays */
    for (k = 0; k < 3; ++k) {
        cadv_vec3 r = s->relay[k];
        bool live = !s->relay_done[k];
        cube(sc, g, at(cadv_m4_identity(), r.x, 1.9f, r.z, 0.9f, 1.2f, 0.9f), cadv_v4(0.25f, 0.25f, 0.3f, 1), 0.0f, 0);
        cube(sc, g, at(cadv_m4_identity(), r.x, 2.7f + 0.1f * cadv_sinf(t * 4.0f), r.z, 0.5f, 0.2f, 0.5f), live ? cadv_v4(1.0f, 0.3f, 0.8f, 1) : cadv_v4(0.1f, 0.1f, 0.1f, 1),
             live ? 2.0f : 0.0f, CADV_MAT_UNLIT);
        if (live && lights < 20) {
            cadv_scene_light(sc, cadv_v3(r.x, 3.0f, r.z), cadv_v3(1.0f, 0.3f, 0.8f), 7.0f, false);
            ++lights;
        }
    }
    if (s->casing_open && !s->star_taken) {
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(s->star_pos), cadv_m4_mul(cadv_m4_rotate_y(t * 1.5f), cadv_m4_translate(cadv_v3(-1.5f, -1.5f, -0.3f)))),
                        cadv_material_make(0, cadv_v4(1.3f, 1.2f, 0.7f, 1), 1.5f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, s->star_pos, cadv_v3(1.4f, 1.2f, 0.6f), 12.0f, false);
    }
    /* people */
    for (i = 0; i < P10_AGENTS; ++i) {
        const p10_agent *a = &s->ag[i];
        if (a->alive && !s->car.driven) {
            person(sc, g, a->f.pos, a->yaw, a->hurt > 0 ? cadv_v4(0.9f, 0.3f, 0.3f, 1) : cadv_v4(0.15f, 0.15f, 0.2f, 1), cadv_v4(0.8f, 0.7f, 0.6f, 1), 1.0f, (float)s->ticks * 0.15f, true);
        }
    }
    for (i = 0; i < P10_CIVS; ++i) {
        const p10_civ *c = &s->civ[i];
        static const float coats[5][3] = {{0.7f, 0.3f, 0.5f}, {0.3f, 0.5f, 0.7f}, {0.6f, 0.6f, 0.3f}, {0.4f, 0.7f, 0.5f}, {0.5f, 0.4f, 0.7f}};
        if (!c->active || flat_d(c->f.pos, cam->pos) > 70.0f) {
            continue;
        }
        person(sc, g, c->f.pos, c->yaw, cadv_v4(coats[i % 5][0], coats[i % 5][1], coats[i % 5][2], 1), cadv_v4(0.85f, 0.7f, 0.6f, 1), 0.95f, (float)(s->ticks + i * 10) * 0.12f, false);
        if (c->state == P10_CIV_FOLLOW) {
            cube(sc, g, at(cadv_m4_identity(), c->f.pos.x, 2.25f, c->f.pos.z, 0.15f, 0.15f, 0.15f), cadv_v4(0.3f, 1.0f, 0.9f, 1), 2.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
        }
    }
    for (i = 0; i < P10_ENEMIES; ++i) {
        const p10_enemy *e = &s->e[i];
        cadv_vec4 coat;
        if (!e->active || flat_d(e->f.pos, cam->pos) > 80.0f) {
            continue;
        }
        if (e->kind == P10_E_TURRET) {
            cadv_mat4 b = cadv_m4_mul(cadv_m4_translate(e->f.pos), cadv_m4_rotate_y(e->yaw));
            cube(sc, g, at(b, 0, 0.7f, 0, 0.8f, 1.4f, 0.8f), cadv_v4(0.7f, 0.65f, 0.4f, 1), 0.0f, 0);
            cube(sc, g, at(b, 0, 1.6f, -0.6f, 0.2f, 0.2f, 1.2f), cadv_v4(0.2f, 0.2f, 0.2f, 1), 0.0f, 0);
            cube(sc, g, at(b, 0, 1.6f, 0.0f, 0.5f, 0.3f, 0.5f), e->hurt > 290 ? cadv_v4(1, 1, 1, 1) : cadv_v4(1.0f, 0.2f, 0.2f, 1), 1.5f, CADV_MAT_UNLIT);
            continue;
        }
        coat = e->ally ? cadv_v4(0.3f, 0.8f, 0.8f, 1) : e->kind == P10_E_ENFORCER ? cadv_v4(0.85f, 0.75f, 0.3f, 1) : cadv_v4(0.9f, 0.9f, 0.85f, 1);
        if (e->hurt > 290) {
            coat = cadv_v4(1.2f, 0.4f, 0.4f, 1);
        }
        person(sc, g, e->f.pos, e->yaw, coat, cadv_v4(0.8f, 0.7f, 0.5f, 1), e->kind == P10_E_ENFORCER ? 1.25f : 1.0f, (float)(s->ticks + i * 9) * 0.15f, true);
    }
    /* the car with its headlights */
    {
        const p10_car *c = &s->car;
        cadv_vec3 f = cadv_v3(-cadv_sinf(c->yaw), 0, -cadv_cosf(c->yaw));
        float hover = 0.15f * cadv_sinf(t * 2.0f);
        if (!c->driven) {
            cadv_scene_draw(sc, &g->car, cadv_m4_mul(cadv_m4_translate(cadv_v3(c->pos.x, hover, c->pos.z)), cadv_m4_rotate_y(c->yaw)), cadv_material_make(0, white, 0.2f, 0));
        }
        if (s->act <= P10_ACT_INDUSTRIAL || c->driven) {
            cadv_scene_light(sc, cadv_v3(c->pos.x + f.x * 7.0f, 1.2f, c->pos.z + f.z * 7.0f), cadv_v3(1.3f, 1.25f, 1.1f), 16.0f, true);
        }
    }
    /* the VTOL comes down during the hold */
    if (s->act >= P10_ACT_PLAZA) {
        float k01 = s->vtol_down ? 0.0f : (float)s->hold_t / (float)P10_HOLD;
        cadv_vec3 p = cadv_v3(s->pad.x - 60.0f * k01, 40.0f * k01, s->pad.z - 20.0f * k01);
        cadv_scene_draw(sc, &g->vtol, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(0.3f * k01)), cadv_material_make(0, white, 0.2f, 0));
        cadv_scene_light(sc, cadv_v3(p.x, p.y - 1.0f, p.z), cadv_v3(1.0f, 1.0f, 0.9f), 20.0f, true);
        cube(sc, g, at(cadv_m4_identity(), s->pad.x, 0.03f, s->pad.z, 6.0f, 0.05f, 6.0f), cadv_v4(0.9f, 0.8f, 0.2f, 1), 0.6f + 0.4f * cadv_sinf(t * 5.0f), CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
    }
    /* tracers, laser beams, gauss slugs */
    for (i = 0; i < P10_TRACERS; ++i) {
        const p10_tracer *tr = &s->tracers[i];
        cadv_vec3 d;
        float len;
        if (tr->ttl <= 0) {
            continue;
        }
        d = cadv_v3_sub(tr->b, tr->a);
        len = cadv_v3_len(d);
        if (len < 0.01f) {
            continue;
        }
        {
            cadv_vec3 mid = cadv_v3_add(tr->a, cadv_v3_scale(d, 0.5f));
            float yaw = cadv_atan2f(-d.x, -d.z);
            float pitch = cadv_atan2f(d.y, cadv_sqrtf(d.x * d.x + d.z * d.z));
            float w = tr->kind == 1 ? 0.06f : 0.025f;
            cadv_mat4 m = cadv_m4_mul(cadv_m4_translate(mid), cadv_m4_mul(cadv_m4_rotate_y(yaw), cadv_m4_mul(cadv_m4_rotate_x(pitch), cadv_m4_scale(cadv_v3(w, w, len)))));
            cube(sc, g, m, tr->kind == 1 ? cadv_v4(1.5f, 0.2f, 0.3f, 1) : cadv_v4(1.4f, 1.2f, 0.6f, 1), 2.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
        }
    }
    for (i = 0; i < P10_SHOTS; ++i) {
        if (s->shots[i].active) {
            cube(sc, g, at(cadv_m4_identity(), s->shots[i].pos.x, s->shots[i].pos.y, s->shots[i].pos.z, 0.25f, 0.25f, 0.25f), cadv_v4(0.6f, 0.8f, 1.5f, 1), 2.0f, CADV_MAT_UNLIT);
            if (lights < 22) {
                cadv_scene_light(sc, s->shots[i].pos, cadv_v3(0.5f, 0.7f, 1.3f), 8.0f, false);
                ++lights;
            }
        }
    }
    /* rain: streaks around the camera */
    for (i = 0; i < 90; ++i) {
        uint32_t r = (uint32_t)(i * 2654435761u);
        float rx = cam->pos.x + (float)((int)(r % 2400u) - 1200) / 100.0f;
        float rz = cam->pos.z + (float)((int)((r >> 12) % 2400u) - 1200) / 100.0f;
        float ph = (float)((s->ticks * 7 + (int)(r >> 20)) % 120) / 120.0f;
        float ry = 9.0f - ph * 9.0f;
        cube(sc, g, at(cadv_m4_identity(), rx, ry, rz, 0.015f, 0.5f, 0.015f), cadv_v4(0.6f, 0.7f, 0.9f, 1), 0.6f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
    }
}

/* ---- HUD -------------------------------------------------------------------------------------- */

static bool to_screen(const cadv_camera *cam, const cadv_ui *ui, cadv_vec3 p, float *sx, float *sy)
{
    cadv_vec3 f = cadv_v3_from_yaw_pitch(cam->yaw, cam->pitch);
    cadv_mat4 vp = cadv_m4_mul(cadv_camera_proj(cam, (float)ui->width / (float)ui->height), cadv_camera_view(cam));
    cadv_vec3 n;
    if (cadv_v3_dot(cadv_v3_sub(p, cam->pos), f) < 0.5f) {
        return false;
    }
    n = cadv_m4_mul_point(vp, p);
    *sx = (n.x * 0.5f + 0.5f) * (float)ui->width;
    *sy = (0.5f - n.y * 0.5f) * (float)ui->height;
    return true;
}

static void bar(cadv_ui *ui, float x, float y, float w, float frac, cadv_vec4 c)
{
    cadv_ui_rect(ui, x - 1, y - 1, w + 2, 5, cadv_v4(0, 0, 0, 0.6f));
    cadv_ui_rect(ui, x, y, w * cadv_clampf(frac, 0.0f, 1.0f), 3, c);
}

static void draw_hud(p10_state *s, const cadv_camera *cam, cadv_ui *ui)
{
    float W = (float)ui->width;
    float H = (float)ui->height;
    float cx = W * 0.5f;
    float cy = H * 0.5f;
    char buf[96];
    int i;
    static const char *const objective[P10_ACT_DONE + 1] = {"CUT RELAY 1 IN THE MARKET", "TAKE THE CAR - RELAY 2 IN THE FACTORY", "RELAY 3 - HALCYON TOWER",
                                                            "TAKE WHAT GLOWS IN THE ANTENNA", "HOLD THE PLAZA PAD FOR THE VTOL", "EXTRACTED"};
    static const char *const sliders[3] = {"SPD", "AIM", "AWR"};
    /* crosshair */
    cadv_ui_rect(ui, cx - 5, cy, 3, 1, cadv_v4(0.3f, 1, 0.9f, 0.9f));
    cadv_ui_rect(ui, cx + 3, cy, 3, 1, cadv_v4(0.3f, 1, 0.9f, 0.9f));
    cadv_ui_rect(ui, cx, cy - 5, 1, 3, cadv_v4(0.3f, 1, 0.9f, 0.9f));
    cadv_ui_rect(ui, cx, cy + 3, 1, 3, cadv_v4(0.3f, 1, 0.9f, 0.9f));
    /* objective and alert */
    cadv_ui_text(ui, 4, 4, 1, cadv_v4(1, 0.5f, 0.9f, 1), objective[s->act]);
    snprintf(buf, sizeof buf, "ALERT %s", s->alert >= 80.0f ? "ALARM" : s->alert >= 40.0f ? "WARY" : "CALM");
    cadv_ui_text(ui, 4, 13, 1, s->alert >= 80.0f ? cadv_v4(1, 0.3f, 0.3f, 1) : s->alert >= 40.0f ? cadv_v4(1, 0.8f, 0.3f, 1) : cadv_v4(0.5f, 0.9f, 1, 1), buf);
    bar(ui, 64, 15, 40, s->alert / 100.0f, cadv_v4(1, 0.35f, 0.3f, 1));
    snprintf(buf, sizeof buf, "CROWD %d", s->followers);
    cadv_ui_text(ui, 4, 22, 1, cadv_v4(0.3f, 1, 0.9f, 1), buf);
    /* the cell on the right */
    for (i = 0; i < P10_AGENTS; ++i) {
        const p10_agent *a = &s->ag[i];
        const char *o = a->alive ? cadv_order_name((cadv_order)a->order) : "DOWN";
        snprintf(buf, sizeof buf, "%-.12s", s->names[i]);
        cadv_ui_text(ui, W - 76, 4 + (float)i * 18, 1, a->alive ? cadv_v4(0.8f, 0.9f, 1, 1) : cadv_v4(0.5f, 0.3f, 0.3f, 1), buf);
        bar(ui, W - 76, 13 + (float)i * 18, 30, a->alive ? (float)a->hp / 100.0f : 0.0f, cadv_v4(0.3f, 0.9f, 0.5f, 1));
        cadv_ui_text(ui, W - 4 - (float)strlen(o) * 6, 12 + (float)i * 18, 1, cadv_v4(0.6f, 0.7f, 0.8f, 1), o);
    }
    /* health, weapon, sliders */
    bar(ui, 6, H - 9, 60, (float)s->hp / 100.0f, cadv_v4(0.9f, 0.3f, 0.3f, 1));
    cadv_ui_text(ui, 6, H - 19, 1, cadv_v4(0.9f, 0.9f, 1, 1), "HEALTH");
    snprintf(buf, sizeof buf, "%s%s", s->weapon == P10_W_MINIGUN ? "MINIGUN" : s->weapon == P10_W_LASER ? "LASER" : "GAUSS", "");
    if (s->weapon == P10_W_GAUSS) {
        snprintf(buf, sizeof buf, "GAUSS %d", s->gauss_ammo);
    }
    cadv_ui_text(ui, W - 6 - (float)strlen(buf) * 6, H - 11, 1, cadv_v4(0.9f, 0.9f, 1, 1), buf);
    for (i = 0; i < 3; ++i) {
        bool on = s->oc.on[i];
        cadv_ui_text(ui, 76 + (float)i * 26, H - 19, 1, cadv_overclock_crashed(&s->oc) ? cadv_v4(0.6f, 0.3f, 0.3f, 1) : on ? cadv_v4(0.3f, 1, 0.9f, 1) : cadv_v4(0.45f, 0.45f, 0.55f, 1), sliders[i]);
    }
    bar(ui, 76, H - 9, 74, s->oc.stamina / s->oc.max, cadv_overclock_crashed(&s->oc) ? cadv_v4(0.6f, 0.3f, 0.3f, 1) : cadv_v4(0.3f, 0.9f, 1, 1));
    if (s->car.driven) {
        snprintf(buf, sizeof buf, "%d KM/H", (int)(cadv_absf(s->car.speed) * 3.6f));
        cadv_ui_text_centered(ui, cx, H - 30, 2, cadv_v4(0.3f, 1, 0.9f, 1), buf);
    }
    if (s->act == P10_ACT_PLAZA && !s->vtol_down) {
        snprintf(buf, sizeof buf, "VTOL IN %d", s->hold_t / 60 + 1);
        cadv_ui_text_centered(ui, cx, 22, 2, cadv_v4(1, 0.85f, 0.4f, 1), buf);
    }
    /* awareness: hostiles marked through the walls */
    if (s->oc.on[CADV_OC_AWARE]) {
        for (i = 0; i < P10_ENEMIES; ++i) {
            float sx;
            float sy;
            if (cadv_p10_hostile(s, i) && flat_d(s->e[i].f.pos, cam->pos) < 60.0f &&
                to_screen(cam, ui, cadv_v3(s->e[i].f.pos.x, 1.2f, s->e[i].f.pos.z), &sx, &sy)) {
                cadv_ui_rect(ui, sx - 2, sy - 2, 4, 4, cadv_v4(1, 0.3f, 0.3f, 0.9f));
            }
        }
    }
    /* the hacking ring: dots round a circle, the gap lit, the marker sweeping */
    if (s->hacking >= 0) {
        int d;
        cadv_ui_rect(ui, cx - 36, cy - 36, 72, 72, cadv_v4(0, 0, 0, 0.5f));
        for (d = 0; d < 32; ++d) {
            float a = (float)d / 32.0f * 2.0f * CADV_PI;
            bool gap = cadv_absf(cadv_wrap_angle(a - s->hack.window)) <= s->hack.width;
            cadv_ui_rect(ui, cx + cadv_sinf(a) * 28.0f - 1, cy - cadv_cosf(a) * 28.0f - 1, 2, 2, gap ? cadv_v4(0.3f, 1, 0.5f, 1) : cadv_v4(0.5f, 0.3f, 0.6f, 1));
        }
        cadv_ui_rect(ui, cx + cadv_sinf(s->hack.angle) * 28.0f - 2, cy - cadv_cosf(s->hack.angle) * 28.0f - 2, 4, 4, cadv_v4(1, 1, 1, 1));
        snprintf(buf, sizeof buf, "%d/%d", s->hack.hits, s->hack.need);
        cadv_ui_text_centered(ui, cx, cy - 3, 1, cadv_v4(1, 1, 1, 1), buf);
    }
    /* the radial order ring */
    if (s->ring.open) {
        static const char *const labels[4] = {"FOLLOW", "ATTACK", "HOLD", "COVER"};
        static const float pos[4][2] = {{0, -26}, {30, 0}, {0, 26}, {-30, 0}};
        cadv_ui_rect(ui, cx - 44, cy - 34, 88, 68, cadv_v4(0, 0, 0, 0.55f));
        for (i = 0; i < 4; ++i) {
            cadv_ui_text_centered(ui, cx + pos[i][0], cy + pos[i][1] - 3, 1, s->ring.sel == i ? cadv_v4(0.3f, 1, 0.9f, 1) : cadv_v4(0.7f, 0.7f, 0.8f, 1), labels[i]);
        }
    }
    if (s->message_timer > 0) {
        float a = cadv_minf(1.0f, (float)s->message_timer / 40.0f);
        char lines[3][64];
        int n = cadv_help_wrap(s->message, (int)(W / 6.0f) - 2, lines, 3);
        for (i = 0; i < n; ++i) {
            cadv_ui_text_centered(ui, cx, cy + 40 + 9 * (float)i, 1, cadv_v4(1, 0.9f, 1, a), lines[i]);
        }
    }
    if (s->radio_timer > 0) {
        char lines[4][64];
        int n = cadv_help_wrap(s->radio, (int)(W / 6.0f) - 4, lines, 4);
        cadv_ui_rect(ui, 0, 60, W, 4 + 9 * (float)n, cadv_v4(0.05f, 0, 0.1f, 0.55f));
        for (i = 0; i < n; ++i) {
            cadv_ui_text(ui, 8, 62 + 9 * (float)i, 1, cadv_v4(0.5f, 1, 0.9f, 1), lines[i]);
        }
    }
    if (s->hurt_flash > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.8f, 0.1f, 0.2f, 0.03f * (float)s->hurt_flash));
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, W, 36, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(1, 0.5f, 0.9f, 1), "NIVALIS SLEEPS");
    }
    if (s->lose_timer > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.4f, 0.0f, 0.1f, 0.4f));
    }
}

void cadv_p10_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p10_state *s = (p10_state *)state;
    float t = ((float)s->ticks + alpha) * P10_DT;
    cam->pos = cadv_p10_eye(s);
    if (s->car.driven) {
        cam->pos = cadv_v3(s->car.pos.x, 1.6f, s->car.pos.z);
    }
    cam->yaw = s->body.yaw;
    cam->pitch = s->body.pitch;
    cam->fovy = 1.15f + (cadv_overclock_level(&s->oc, CADV_OC_SPEED) > 0.0f ? 0.12f : 0.0f);
    cam->znear = 0.1f;
    cam->zfar = 160.0f;
    sc->ambient = cadv_v3(0.34f, 0.28f, 0.46f);
    sc->sun_color = cadv_v3(0.3f, 0.26f, 0.45f);
    sc->clear_color = cadv_v3(0.06f, 0.03f, 0.1f);
    sc->fog_color = cadv_v3(0.2f, 0.1f, 0.26f);
    sc->sun_dir = cadv_v3_norm(cadv_v3(0.3f, -0.8f, -0.4f));
    sc->sun_shadow = false;
    sc->fog_near = 35.0f;
    sc->fog_far = 140.0f;
    sc->light_bands = 5;
    sc->dither = 0.6f;
    sc->use_palette = true;
    if (cadv_overclock_crashed(&s->oc)) {
        cam->pitch += 0.02f * cadv_sinf(t * 7.0f); /* the crash sways the view */
    }
    draw_world(s, sc, cam, t);
    /* your gun in hand */
    if (!s->car.driven && s->lose_timer == 0) {
        cadv_mat4 view = cadv_m4_mul(cadv_m4_translate(cam->pos), cadv_m4_mul(cadv_m4_rotate_y(cam->yaw), cadv_m4_rotate_x(cam->pitch)));
        cadv_vec4 c = s->weapon == P10_W_MINIGUN ? cadv_v4(0.3f, 0.3f, 0.34f, 1) : s->weapon == P10_W_LASER ? cadv_v4(0.6f, 0.2f, 0.3f, 1) : cadv_v4(0.3f, 0.4f, 0.6f, 1);
        float kick = s->cd > 0.5f ? (s->cd - 0.5f) * 0.1f : 0.0f;
        cube(sc, &s->g, at(view, 0.2f, -0.2f, -0.55f + kick, 0.06f, 0.07f, 0.32f), c, 0.0f, CADV_MAT_NO_SHADOW);
        if (s->beacon) {
            cube(sc, &s->g, at(view, -0.25f, -0.22f, -0.5f, 0.12f, 0.12f, 0.12f), cadv_v4(0.3f, 1.0f, 0.9f, 1), 2.0f + cadv_sinf(t * 12.0f), CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
            cadv_scene_light(sc, cam->pos, cadv_v3(0.2f, 0.8f, 0.7f), 8.0f, false);
        }
    }
    if (ui) {
        draw_hud(s, cam, ui);
    }
}

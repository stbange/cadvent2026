/* SPDX-License-Identifier: MIT */
/* GPU side of Day 7: the half-timbered market of Glühstadt under snow,
 * stalls and string lights, the giant tree, low-poly characters for both
 * teams, confetti, smoke clouds, the glitter bomb, the weapon in hand and
 * the HUD with the buy menu and the scoreboard. */
#include "portals/p07_tinsel_strike_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portals/p07_internal.h"

static unsigned make_tex(cadv_tex_kind kind, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, kind, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

/* Spray paint: bright letters, a little ragged, on transparent-looking plaster. */
static unsigned tag_tex(const char *text, int *w_out)
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
        t.rgba[i * 4] = 205;
        t.rgba[i * 4 + 1] = 195;
        t.rgba[i * 4 + 2] = 175;
        t.rgba[i * 4 + 3] = 255;
    }
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y)) {
                    cadv_texdata_set(&t, 3 + i * 6 + x, 2 + y + (i % 2), 220, 30, 120, 255);
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

static void build_static(p07_state *s)
{
    p07_gpu *g = &s->g;
    cadv_mesh m;
    cadv_mesh roofs;
    cadv_mesh crates;
    cadv_mesh lights;
    float W = (float)P07_W * P07_CELL;
    float H = (float)P07_H * P07_CELL;
    int x;
    int z;
    int k;
    cadv_mesh_init(&m);
    cadv_mesh_init(&roofs);
    cadv_mesh_init(&crates);
    cadv_mesh_init(&lights);
    cadv_mesh_quad(&m, cadv_v3(0, 0, H), cadv_v3(W, 0, H), cadv_v3(W, 0, 0), cadv_v3(0, 0, 0), 30.0f, 24.0f, cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->ground, &m, false);
    cadv_mesh_clear(&m);
    /* houses: runs of building cells, each run a slightly different tint and height */
    for (z = 0; z < P07_H; ++z) {
        for (x = 0; x < P07_W;) {
            int x1 = x;
            float h;
            cadv_rgba tint;
            if (s->cells[z * P07_W + x] != 1) {
                ++x;
                continue;
            }
            while (x1 + 1 < P07_W && s->cells[z * P07_W + x1 + 1] == 1 && x1 - x < 3) {
                ++x1;
            }
            h = P07_HOUSE_H + (float)((x * 7 + z * 3) % 3) * 0.6f;
            k = (x * 5 + z * 11) % 4;
            tint = k == 0 ? cadv_rgba_make(235, 225, 200, 255) : k == 1 ? cadv_rgba_make(225, 200, 180, 255)
                   : k == 2 ? cadv_rgba_make(210, 215, 200, 255) : cadv_rgba_make(240, 210, 170, 255);
            box(&m, (float)x * P07_CELL, 0, (float)z * P07_CELL, (float)(x1 + 1) * P07_CELL, h, (float)(z + 1) * P07_CELL, 0.5f, tint);
            {
                /* half-timbering: dark beams on the facades, a floor band and posts */
                cadv_rgba beam = cadv_rgba_make(70, 45, 35, 255);
                float bx0 = (float)x * P07_CELL - 0.04f;
                float bx1 = (float)(x1 + 1) * P07_CELL + 0.04f;
                float bz0 = (float)z * P07_CELL - 0.04f;
                float bz1 = (float)(z + 1) * P07_CELL + 0.04f;
                int p;
                box(&roofs, bx0, 2.6f, bz0, bx1, 2.85f, bz1, 0.0f, beam);
                box(&roofs, bx0, h - 0.3f, bz0, bx1, h, bz1, 0.0f, beam);
                for (p = x; p <= x1 + 1; ++p) {
                    float px = (float)p * P07_CELL;
                    box(&roofs, px - 0.12f, 0.0f, bz0, px + 0.12f, h, bz1, 0.0f, beam);
                }
                box(&roofs, bx0, 0.0f, bz0, bx0 + 0.24f, h, bz1, 0.0f, beam);
                /* a lit window in the upper floor */
                if ((x * 3 + z) % 4 != 0) {
                    float wx = ((float)x + 0.5f) * P07_CELL;
                    box(&roofs, wx - 0.35f, 3.6f, bz0 - 0.01f, wx + 0.35f, 4.5f, bz1 + 0.01f, 0.0f, cadv_rgba_make(255, 210, 120, 255));
                }
            }
            /* snow on the roofs, a chimney now and then */
            box(&roofs, (float)x * P07_CELL - 0.15f, h, (float)z * P07_CELL - 0.15f, (float)(x1 + 1) * P07_CELL + 0.15f, h + 0.35f,
                (float)(z + 1) * P07_CELL + 0.15f, 0.0f, cadv_rgba_make(240, 245, 252, 255));
            if ((x + z) % 5 == 0) {
                box(&roofs, (float)x * P07_CELL + 0.6f, h + 0.35f, (float)z * P07_CELL + 0.6f, (float)x * P07_CELL + 1.3f, h + 1.6f,
                    (float)z * P07_CELL + 1.3f, 0.0f, cadv_rgba_make(120, 70, 60, 255));
            }
            x = x1 + 1;
        }
    }
    cadv_gpu_mesh_upload(&g->houses, &m, false);
    cadv_gpu_mesh_upload(&g->roofs, &roofs, false);
    cadv_mesh_clear(&m);
    /* stalls: wooden counters with striped canopies */
    for (z = 0; z < P07_H; ++z) {
        for (x = 0; x < P07_W; ++x) {
            if (s->cells[z * P07_W + x] == 2) {
                float x0 = (float)x * P07_CELL + 0.05f;
                float z0 = (float)z * P07_CELL + 0.05f;
                bool red = (x + z) % 2 == 0;
                box(&crates, x0, 0, z0, x0 + P07_CELL - 0.1f, P07_CRATE_H, z0 + P07_CELL - 0.1f, 1.0f, cadv_rgba_make(170, 120, 80, 255));
                box(&crates, x0, 2.3f, z0, x0 + P07_CELL - 0.1f, 2.45f, z0 + P07_CELL - 0.1f, 0.0f,
                    red ? cadv_rgba_make(200, 50, 45, 255) : cadv_rgba_make(240, 240, 235, 255));
                box(&crates, x0 + 0.05f, P07_CRATE_H, z0 + 0.05f, x0 + 0.15f, 2.3f, z0 + 0.15f, 0.0f, cadv_rgba_make(120, 80, 50, 255));
                box(&crates, x0 + P07_CELL - 0.25f, P07_CRATE_H, z0 + 0.05f, x0 + P07_CELL - 0.15f, 2.3f, z0 + 0.15f, 0.0f, cadv_rgba_make(120, 80, 50, 255));
            }
        }
    }
    cadv_gpu_mesh_upload(&g->crates, &crates, false);
    /* string lights zig-zagging across the streets */
    for (k = 0; k < (int)(P07_W * P07_H); k += 7) {
        int cx = k % P07_W;
        int cz = k / P07_W;
        if (s->cells[cz * P07_W + cx] == 0 && (cx + cz) % 3 == 0) {
            int b;
            for (b = 0; b < 6; ++b) {
                float px = (float)cx * P07_CELL + (float)b * 0.33f;
                float py = 4.6f - 0.25f * cadv_sinf((float)b * 0.6f);
                float pz = (float)cz * P07_CELL + 1.0f;
                cadv_rgba c = b % 3 == 0 ? cadv_rgba_make(255, 220, 120, 255) : b % 3 == 1 ? cadv_rgba_make(255, 120, 90, 255) : cadv_rgba_make(150, 220, 255, 255);
                box(&lights, px - 0.06f, py - 0.06f, pz - 0.06f, px + 0.06f, py + 0.06f, pz + 0.06f, 0.0f, c);
            }
        }
    }
    cadv_gpu_mesh_upload(&g->lights, &lights, false);
    /* the giant tree: trunk and stacked tiers, with baubles */
    cadv_mesh_clear(&m);
    {
        float cx = (float)19 * P07_CELL + 2.0f;
        float cz = (float)15 * P07_CELL + 2.0f;
        int t;
        cadv_mesh_prism(&m, cadv_v3(cx, 0, cz), 0.8f, 3.0f, 8, 0.0f, cadv_rgba_make(110, 75, 50, 255));
        for (t = 0; t < 6; ++t) {
            float r = 3.4f - (float)t * 0.5f;
            cadv_mesh_prism(&m, cadv_v3(cx, 2.0f + (float)t * 1.3f, cz), r, 1.6f, 8, 0.0f,
                            t % 2 ? cadv_rgba_make(40, 100, 60, 255) : cadv_rgba_make(50, 115, 70, 255));
        }
        for (t = 0; t < 24; ++t) {
            float a = (float)t * 1.7f;
            float y = 2.4f + (float)(t % 6) * 1.3f;
            float r = 3.4f - (float)(t % 6) * 0.5f + 0.1f;
            cadv_rgba c = t % 3 == 0 ? cadv_rgba_make(230, 40, 40, 255) : t % 3 == 1 ? cadv_rgba_make(250, 210, 60, 255) : cadv_rgba_make(80, 140, 255, 255);
            box(&m, cx + cadv_cosf(a) * r - 0.15f, y, cz + cadv_sinf(a) * r - 0.15f, cx + cadv_cosf(a) * r + 0.15f, y + 0.3f, cz + cadv_sinf(a) * r + 0.15f, 0.0f, c);
        }
    }
    cadv_gpu_mesh_upload(&g->tree, &m, false);
    cadv_mesh_clear(&m);
    /* the bomb sites: painted letters on the cobbles */
    for (k = 0; k < 2; ++k) {
        float cx = k == 0 ? 33.0f * P07_CELL - 0.5f : 7.0f * P07_CELL - 0.5f;
        float cz = 8.0f * P07_CELL;
        cadv_rgba c = cadv_rgba_make(250, 200, 60, 255);
        box(&m, cx - 1.6f, 0.01f, cz - 2.0f, cx - 1.1f, 0.03f, cz + 2.0f, 0.0f, c);
        box(&m, cx + 1.1f, 0.01f, cz - 2.0f, cx + 1.6f, 0.03f, cz + 2.0f, 0.0f, c);
        box(&m, cx - 1.6f, 0.01f, cz - 2.0f, cx + 1.6f, 0.03f, cz - 1.5f, 0.0f, c);
        box(&m, cx - 1.6f, 0.01f, cz - 0.25f, cx + 1.6f, 0.03f, cz + 0.25f, 0.0f, c);
        if (k == 1) {
            box(&m, cx - 1.6f, 0.01f, cz + 1.5f, cx + 1.6f, 0.03f, cz + 2.0f, 0.0f, c); /* the B closes at the bottom */
        }
    }
    cadv_gpu_mesh_upload(&g->sites, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, cadv_rgba_make(255, 255, 255, 255), false);
    cadv_gpu_mesh_upload(&g->cube, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f, cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->plaque, &m, false);
    cadv_mesh_clear(&m);
    /* the glitter bomb: a wrapped present with a fuse and a lamp */
    box(&m, -0.3f, 0, -0.22f, 0.3f, 0.35f, 0.22f, 0.0f, cadv_rgba_make(160, 60, 200, 255));
    box(&m, -0.32f, 0, -0.04f, 0.32f, 0.37f, 0.04f, 0.0f, cadv_rgba_make(250, 220, 80, 255));
    box(&m, -0.04f, 0, -0.24f, 0.04f, 0.37f, 0.24f, 0.0f, cadv_rgba_make(250, 220, 80, 255));
    box(&m, 0.15f, 0.35f, -0.05f, 0.22f, 0.5f, 0.02f, 0.0f, cadv_rgba_make(40, 40, 40, 255));
    cadv_gpu_mesh_upload(&g->bomb, &m, false);
    cadv_mesh_free(&m);
    cadv_mesh_free(&roofs);
    cadv_mesh_free(&crates);
    cadv_mesh_free(&lights);
}

void cadv_p07_gpu_init(void *state)
{
    p07_state *s = (p07_state *)state;
    p07_gpu *g = &s->g;
    cadv_voxels v;
    cadv_mesh m;
    int x;
    int z;
    memset(g, 0, sizeof *g);
    g->tex_cobble = make_tex(CADV_TEX_STONE_BRICKS, 71u, cadv_rgba_make(150, 150, 155, 255), cadv_rgba_make(205, 210, 220, 255));
    g->tex_timber = make_tex(CADV_TEX_PLASTER, 72u, cadv_rgba_make(235, 228, 210, 255), cadv_rgba_make(200, 190, 170, 255));
    g->tex_wood = make_tex(CADV_TEX_PLANKS, 73u, cadv_rgba_make(150, 105, 70, 255), cadv_rgba_make(110, 75, 50, 255));
    g->tex_mark = tag_tex(s->mark_text[0], &g->mark_w);
    build_static(s);
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
    cadv_voxels_mesh(&v, 0.1f, &m);
    cadv_gpu_mesh_upload(&g->star, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    s->gpu_ready = true;
}

void cadv_p07_gpu_free(void *state)
{
    p07_state *s = (p07_state *)state;
    p07_gpu *g = &s->g;
    cadv_gpu_mesh_free(&g->ground);
    cadv_gpu_mesh_free(&g->houses);
    cadv_gpu_mesh_free(&g->roofs);
    cadv_gpu_mesh_free(&g->crates);
    cadv_gpu_mesh_free(&g->tree);
    cadv_gpu_mesh_free(&g->trunk);
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->steps);
    cadv_gpu_mesh_free(&g->bomb);
    cadv_gpu_mesh_free(&g->lights);
    cadv_gpu_mesh_free(&g->sites);
    cadv_gl_delete_textures(1, &g->tex_cobble);
    cadv_gl_delete_textures(1, &g->tex_timber);
    cadv_gl_delete_textures(1, &g->tex_wood);
    cadv_gl_delete_textures(1, &g->tex_mark);
    g->steps_built = 0;
    s->gpu_ready = false;
}

/* ---- draw ------------------------------------------------------------------------------- */

static void cube(cadv_scene *sc, const p07_gpu *g, cadv_mat4 m, cadv_vec4 c, uint32_t flags)
{
    cadv_scene_draw(sc, &g->cube, m, cadv_material_make(0, c, 0.0f, flags));
}

static cadv_mat4 at(cadv_mat4 base, float x, float y, float z, float sx, float sy, float sz)
{
    return cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(x, y, z)), cadv_m4_scale(cadv_v3(sx, sy, sz))));
}

/* A low-poly figure: Humbug pranksters in green with masks and bobble hats,
 * Nutcracker guards in red with white trousers and tall black hats. */
static void draw_agent(const p07_state *s, cadv_scene *sc, int i)
{
    const p07_gpu *g = &s->g;
    const p07_agent *a = &s->a[i];
    int side = cadv_p07_side_of(s, a->team);
    float speed = cadv_sqrtf(a->body.vel.x * a->body.vel.x + a->body.vel.z * a->body.vel.z);
    float swing = cadv_sinf(a->body.stride * 3.3f) * 0.45f * cadv_minf(1.0f, speed / 3.0f);
    float crouch = a->body.crouching ? 0.6f : 1.0f;
    cadv_mat4 base = cadv_m4_mul(cadv_m4_translate(a->body.pos), cadv_m4_rotate_y(a->body.yaw));
    cadv_vec4 coat = side == P07_HUMBUG ? cadv_v4(0.25f, 0.5f, 0.3f, 1) : cadv_v4(0.75f, 0.12f, 0.12f, 1);
    cadv_vec4 legs = side == P07_HUMBUG ? cadv_v4(0.2f, 0.18f, 0.2f, 1) : cadv_v4(0.92f, 0.92f, 0.9f, 1);
    cadv_vec4 skin = cadv_v4(0.92f, 0.75f, 0.62f, 1);
    cadv_vec4 hat = side == P07_HUMBUG ? cadv_v4(0.85f, 0.2f, 0.2f, 1) : cadv_v4(0.08f, 0.08f, 0.1f, 1);
    float hy = 0.85f * crouch;
    cube(sc, g, cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.13f, hy, 0)), cadv_m4_mul(cadv_m4_rotate_x(swing), cadv_m4_mul(cadv_m4_translate(cadv_v3(0, -hy * 0.5f, 0)), cadv_m4_scale(cadv_v3(0.17f, hy, 0.2f)))))), legs, 0);
    cube(sc, g, cadv_m4_mul(base, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.13f, hy, 0)), cadv_m4_mul(cadv_m4_rotate_x(-swing), cadv_m4_mul(cadv_m4_translate(cadv_v3(0, -hy * 0.5f, 0)), cadv_m4_scale(cadv_v3(0.17f, hy, 0.2f)))))), legs, 0);
    cube(sc, g, at(base, 0, hy + 0.33f, 0, 0.5f, 0.66f, 0.3f), coat, 0);
    cube(sc, g, at(base, 0, hy + 0.82f, 0, 0.3f, 0.32f, 0.3f), skin, 0);
    if (side == P07_HUMBUG) {
        cube(sc, g, at(base, 0, hy + 0.85f, -0.13f, 0.32f, 0.1f, 0.06f), cadv_v4(0.1f, 0.1f, 0.1f, 1), 0); /* the mask */
        cube(sc, g, at(base, 0, hy + 1.03f, 0, 0.32f, 0.12f, 0.32f), hat, 0);
        cube(sc, g, at(base, 0, hy + 1.15f, 0, 0.12f, 0.12f, 0.12f), cadv_v4(0.95f, 0.95f, 0.95f, 1), 0);
    } else {
        cube(sc, g, at(base, 0, hy + 1.12f, 0, 0.3f, 0.32f, 0.3f), hat, 0);
        cube(sc, g, at(base, 0, hy + 0.75f, -0.15f, 0.26f, 0.06f, 0.04f), cadv_v4(0.95f, 0.95f, 0.95f, 1), 0); /* the beard */
    }
    /* arms forward with the weapon */
    cube(sc, g, at(base, 0.18f, hy + 0.45f, -0.25f, 0.1f, 0.1f, 0.5f), coat, 0);
    cube(sc, g, at(base, -0.12f, hy + 0.45f, -0.28f, 0.1f, 0.1f, 0.45f), coat, 0);
    cube(sc, g, at(base, 0.05f, hy + 0.5f, -0.55f, 0.07f, 0.1f, a->slot == P07_SLOT_PRIMARY ? 0.7f : 0.3f), cadv_v4(0.15f, 0.15f, 0.17f, 1), 0);
    if (i == s->carrier) {
        cube(sc, g, at(base, 0, hy + 0.4f, 0.22f, 0.4f, 0.3f, 0.15f), cadv_v4(0.6f, 0.25f, 0.8f, 1), 0); /* the bomb in a backpack */
    }
}

static void draw_confetti(const p07_state *s, cadv_scene *sc)
{
    int k;
    float t = (float)(60 - s->confetti_t) / 60.0f;
    for (k = 0; k < 30; ++k) {
        float a = (float)k * 2.399f;
        float r = t * (1.5f + (float)(k % 5) * 0.4f);
        float y = 1.2f + t * 2.0f - t * t * 2.6f + (float)(k % 4) * 0.2f;
        cadv_vec4 c = k % 4 == 0 ? cadv_v4(1, 0.3f, 0.3f, 1) : k % 4 == 1 ? cadv_v4(1, 0.9f, 0.3f, 1) : k % 4 == 2 ? cadv_v4(0.3f, 0.8f, 1, 1) : cadv_v4(0.5f, 1, 0.5f, 1);
        cube(sc, &s->g,
             cadv_m4_mul(cadv_m4_translate(cadv_v3(s->confetti_pos.x + cadv_cosf(a) * r, s->confetti_pos.y + y, s->confetti_pos.z + cadv_sinf(a) * r)),
                         cadv_m4_mul(cadv_m4_rotate_y(a + t * 5.0f), cadv_m4_scale(cadv_v3(0.08f, 0.02f, 0.06f)))),
             c, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
    }
}

static int view_agent(const p07_state *s) { return s->a[0].alive || s->phase == P07_PH_CELEBRATE ? 0 : s->spectate; }

static void draw_view_weapon(const p07_state *s, cadv_scene *sc, const cadv_camera *cam)
{
    const p07_agent *a = &s->a[view_agent(s)];
    cadv_mat4 view = cadv_m4_mul(cadv_m4_translate(cam->pos), cadv_m4_mul(cadv_m4_rotate_y(cam->yaw), cadv_m4_rotate_x(cam->pitch)));
    float kick = a->shot_cd > 0.0f && a->since_shot < 0.08f ? 0.04f : 0.0f;
    float bob = cadv_sinf(a->body.stride * 3.3f) * 0.012f;
    cadv_vec4 metal = cadv_v4(0.18f, 0.18f, 0.2f, 1);
    cadv_vec4 wood = cadv_v4(0.45f, 0.3f, 0.18f, 1);
    uint32_t f = CADV_MAT_NO_SHADOW;
    if (a->scoped || s->phase == P07_PH_CELEBRATE) {
        return;
    }
    switch (a->slot) {
    case P07_SLOT_KNIFE: /* a candy cane, striped */
        cube(sc, &s->g, at(view, 0.22f, -0.2f + bob, -0.45f, 0.03f, 0.03f, 0.35f), cadv_v4(0.95f, 0.95f, 0.95f, 1), f);
        cube(sc, &s->g, at(view, 0.22f, -0.2f + bob, -0.5f, 0.035f, 0.035f, 0.06f), cadv_v4(0.9f, 0.1f, 0.1f, 1), f);
        cube(sc, &s->g, at(view, 0.22f, -0.2f + bob, -0.38f, 0.035f, 0.035f, 0.06f), cadv_v4(0.9f, 0.1f, 0.1f, 1), f);
        break;
    case P07_SLOT_FLASH:
    case P07_SLOT_SMOKE:
        cube(sc, &s->g, at(view, 0.2f, -0.18f + bob, -0.4f, 0.08f, 0.12f, 0.08f), a->slot == P07_SLOT_FLASH ? cadv_v4(0.8f, 0.8f, 0.85f, 1) : cadv_v4(0.4f, 0.45f, 0.4f, 1), f);
        break;
    default: {
        bool primary = a->slot == P07_SLOT_PRIMARY;
        float len = primary ? 0.55f : 0.22f;
        cube(sc, &s->g, at(view, 0.2f, -0.19f + bob, -0.35f + kick - len * 0.5f, 0.06f, 0.08f, len), metal, f);
        cube(sc, &s->g, at(view, 0.2f, -0.26f + bob, -0.3f + kick, 0.05f, 0.12f, 0.06f), wood, f);
        if (primary) {
            cube(sc, &s->g, at(view, 0.2f, -0.22f + bob, -0.12f + kick, 0.06f, 0.08f, 0.2f), wood, f);
        }
        if (a->since_shot < 0.05f) {
            cube(sc, &s->g, at(view, 0.2f, -0.19f + bob, -0.38f - len, 0.08f, 0.08f, 0.08f), cadv_v4(2.0f, 1.6f, 0.6f, 1), CADV_MAT_UNLIT | f);
        }
        break;
    }
    }
}

static void draw_world(p07_state *s, cadv_scene *sc, const cadv_camera *cam, float t)
{
    p07_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    int lights = 0;
    int i;
    int self = view_agent(s);
    cadv_scene_draw(sc, &g->ground, cadv_m4_identity(), cadv_material_make(g->tex_cobble, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->houses, cadv_m4_identity(), cadv_material_make(g->tex_timber, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->roofs, cadv_m4_identity(), cadv_material_make(0, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->crates, cadv_m4_identity(), cadv_material_make(g->tex_wood, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->tree, cadv_m4_identity(), cadv_material_make(0, white, 0.05f, 0));
    cadv_scene_draw(sc, &g->sites, cadv_m4_identity(), cadv_material_make(0, white, 0.3f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->lights, cadv_m4_identity(), cadv_material_make(0, cadv_v4(1.6f, 1.5f, 1.4f, 1), 1.5f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
    /* the spray tag on the square's north wall */
    cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(33.0f, 1.6f, 20.02f)), cadv_m4_mul(cadv_m4_rotate_y(CADV_PI), cadv_m4_scale(cadv_v3((float)g->mark_w / 22.0f, 0.55f, 1)))),
                    cadv_material_make(g->tex_mark, white, 0.2f, CADV_MAT_NO_SHADOW));
    /* warm stall lamps and the tree's glow; one lamp post throws shadows */
    cadv_scene_light(sc, cadv_v3(40.0f, 6.0f, 32.0f), cadv_v3(1.4f, 1.1f, 0.7f), 16.0f, true);
    ++lights;
    for (i = 0; i < P07_W * P07_H && lights < 20; i += 37) {
        int cx = i % P07_W;
        int cz = i / P07_W;
        if (s->cells[cz * P07_W + cx] == 2) {
            cadv_vec3 p = cadv_v3((float)cx * P07_CELL + 1.0f, 2.0f, (float)cz * P07_CELL + 1.0f);
            if (cadv_v3_dist(p, cam->pos) < 40.0f) {
                cadv_scene_light(sc, p, cadv_v3(1.0f, 0.7f, 0.35f), 7.0f, false);
                ++lights;
            }
        }
    }
    for (i = 0; i < P07_AGENTS; ++i) {
        if (s->a[i].alive && i != self && (s->phase != P07_PH_CELEBRATE || i == 0 || s->a[i].team == 0)) {
            draw_agent(s, sc, i);
        }
    }
    if (s->confetti_t > 0) {
        draw_confetti(s, sc);
    }
    if (s->planted || s->bomb_dropped) {
        cadv_scene_draw(sc, &g->bomb, cadv_m4_translate(s->bomb_pos), cadv_material_make(0, white, 0.2f, 0));
        if (s->planted && (s->bomb_timer / (s->bomb_timer > 600 ? 60 : 15)) % 2 == 0 && lights < 30) {
            cadv_scene_light(sc, cadv_v3(s->bomb_pos.x, 0.6f, s->bomb_pos.z), cadv_v3(1.6f, 0.3f, 1.4f), 6.0f, false);
            ++lights;
        }
    }
    for (i = 0; i < P07_SMOKES; ++i) {
        const p07_smoke *k = &s->smokes[i];
        if (k->ticks > 0) {
            int b;
            float r = k->ticks > 60 ? 4.0f : 4.0f * (float)k->ticks / 60.0f;
            for (b = 0; b < 9; ++b) {
                float a = (float)b * 0.7f + t * 0.1f;
                float sz = r * (0.9f + 0.2f * cadv_sinf((float)b));
                cube(sc, g, cadv_m4_mul(cadv_m4_translate(cadv_v3(k->pos.x + cadv_cosf(a) * r * 0.4f, k->pos.y - 0.6f + (float)(b % 3) * 1.0f, k->pos.z + cadv_sinf(a) * r * 0.4f)),
                                        cadv_m4_mul(cadv_m4_rotate_y(a), cadv_m4_scale(cadv_v3(sz, sz * 0.7f, sz)))),
                     cadv_v4(0.72f, 0.74f, 0.76f, 1), CADV_MAT_NO_SHADOW);
            }
        }
    }
    for (i = 0; i < P07_NADES; ++i) {
        if (s->nades[i].active) {
            cube(sc, g, at(cadv_m4_identity(), s->nades[i].pos.x, s->nades[i].pos.y, s->nades[i].pos.z, 0.12f, 0.15f, 0.12f), cadv_v4(0.5f, 0.55f, 0.5f, 1), 0);
        }
    }
    for (i = 0; i < 16; ++i) {
        const p07_tracer *tr = &s->tracers[i];
        if (tr->ttl > 0) {
            cadv_vec3 d = cadv_v3_sub(tr->b, tr->a);
            float len = cadv_v3_len(d);
            cadv_mat4 m = cadv_m4_mul(cadv_m4_translate(cadv_v3_scale(cadv_v3_add(tr->a, tr->b), 0.5f)),
                                      cadv_m4_mul(cadv_m4_rotate_y(cadv_atan2f(d.x, d.z)),
                                                  cadv_m4_mul(cadv_m4_rotate_x(-cadv_atan2f(d.y, cadv_sqrtf(d.x * d.x + d.z * d.z))),
                                                              cadv_m4_scale(cadv_v3(0.02f, 0.02f, len > 0.01f ? len : 0.01f)))));
            cube(sc, g, m, cadv_v4(1.8f, 1.6f, 1.0f, 1), CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW);
        }
    }
    /* the celebration: steps up the tree and the star on top */
    if (s->steps) {
        for (i = 0; i < P07_STEPS; ++i) {
            cadv_vec3 p = s->step_pos[i];
            cube(sc, g, at(cadv_m4_identity(), p.x, p.y - 0.15f, p.z, 2.0f, 0.3f, 2.0f), cadv_v4(0.75f, 0.55f, 0.35f, 1), 0);
            cube(sc, g, at(cadv_m4_identity(), p.x, p.y + 0.01f, p.z, 2.0f, 0.04f, 2.0f), cadv_v4(0.95f, 0.97f, 1.0f, 1), CADV_MAT_NO_SHADOW);
        }
        cube(sc, g, at(cadv_m4_identity(), s->star_pos.x, s->star_pos.y - 1.15f, s->star_pos.z, 3.4f, 0.3f, 3.4f), cadv_v4(0.75f, 0.55f, 0.35f, 1), 0);
    }
    if (!s->star_taken) {
        cadv_vec3 p = s->steps ? s->star_pos : cadv_v3(40.0f, 10.3f, 32.0f);
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(cadv_v3(p.x, p.y + 0.15f * cadv_sinf(t * 2.0f), p.z)), cadv_m4_rotate_y(t * 1.2f)),
                        cadv_material_make(0, white, 1.2f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, p, cadv_v3(1.6f, 1.4f, 0.5f), 10.0f, false);
    }
}

/* ---- HUD ------------------------------------------------------------------------------------ */

static void draw_buy(const p07_state *s, cadv_ui *ui)
{
    const p07_agent *a = &s->a[0];
    float x = 20.0f;
    float y = 30.0f;
    char buf[64];
    int i;
    cadv_ui_rect(ui, x - 6, y - 16, 170, 14 + 11.0f * (float)P07_BUY_ITEMS + 16, cadv_v4(0.05f, 0.04f, 0.03f, 0.88f));
    snprintf(buf, sizeof buf, "BUY MENU  $%d", a->money);
    cadv_ui_text(ui, x, y - 12, 1, cadv_v4(1, 0.85f, 0.4f, 1), buf);
    for (i = 0; i < P07_BUY_ITEMS; ++i) {
        bool sel = i == s->buy_cursor;
        bool afford = a->money >= cadv_p07_buy_price(i) && (i != P07_BUY_KIT || cadv_p07_side_of(s, 0) == P07_GUARD);
        if (sel) {
            cadv_ui_rect(ui, x - 3, y + (float)i * 11.0f - 1, 162, 10, cadv_v4(0.35f, 0.2f, 0.1f, 1));
        }
        snprintf(buf, sizeof buf, "%-14s $%d", cadv_p07_buy_name(i), cadv_p07_buy_price(i));
        cadv_ui_text(ui, x, y + (float)i * 11.0f, 1, afford ? cadv_v4(1, 1, 1, 1) : cadv_v4(0.5f, 0.45f, 0.4f, 1), buf);
    }
    cadv_ui_text(ui, x, y + 11.0f * (float)P07_BUY_ITEMS + 2, 1, cadv_v4(0.7f, 0.7f, 0.6f, 1), "Q/WHEEL MOVE  FIRE BUY  6 CLOSE");
}

static void draw_scoreboard(const p07_state *s, cadv_ui *ui)
{
    float cx = (float)ui->width * 0.5f;
    float y = 24.0f;
    char buf[64];
    int team;
    cadv_ui_rect(ui, cx - 120, y - 6, 240, 140, cadv_v4(0.03f, 0.03f, 0.05f, 0.85f));
    for (team = 0; team < 2; ++team) {
        int side = cadv_p07_side_of(s, team);
        int i;
        snprintf(buf, sizeof buf, "%s  %d", side == P07_HUMBUG ? "HUMBUG SYNDICATE" : "NUTCRACKER GUARD", s->score[team]);
        cadv_ui_text(ui, cx - 112, y, 1, side == P07_HUMBUG ? cadv_v4(0.5f, 1, 0.6f, 1) : cadv_v4(1, 0.5f, 0.45f, 1), buf);
        y += 10.0f;
        for (i = 0; i < P07_AGENTS; ++i) {
            const p07_agent *a = &s->a[i];
            if (a->team != team) {
                continue;
            }
            snprintf(buf, sizeof buf, "%-12.12s %2d %2d  $%d", s->names[i], a->kills, a->deaths, a->money);
            cadv_ui_text(ui, cx - 104, y, 1, a->alive ? cadv_v4(1, 1, 1, 1) : cadv_v4(0.5f, 0.5f, 0.5f, 1), buf);
            y += 9.0f;
        }
        y += 6.0f;
    }
}

static void draw_hud(const p07_state *s, cadv_ui *ui)
{
    int v = view_agent(s);
    const p07_agent *a = &s->a[v];
    float W = (float)ui->width;
    float H = (float)ui->height;
    float cx = W * 0.5f;
    float cy = H * 0.5f;
    cadv_vec4 warm = cadv_v4(1, 0.88f, 0.55f, 1);
    char buf[96];
    int i;
    /* score and the clock */
    {
        int t = s->phase == P07_PH_LIVE ? (s->planted ? s->bomb_timer : s->timer) : s->phase == P07_PH_FREEZE ? s->timer : 0;
        int sec = t / 60;
        cadv_ui_rect(ui, cx - 40, 2, 80, 11, cadv_v4(0, 0, 0, 0.6f));
        snprintf(buf, sizeof buf, "%d  %d:%02d  %d", s->score[0], sec / 60, sec % 60, s->score[1]);
        cadv_ui_text_centered(ui, cx, 4, 1, s->planted ? cadv_v4(1, 0.35f, 0.9f, 1) : cadv_v4(1, 1, 1, 1), buf);
        snprintf(buf, sizeof buf, "ROUND %d  %s", s->round, s->player_side == P07_HUMBUG ? "HUMBUG" : "GUARD");
        cadv_ui_text_centered(ui, cx, 15, 1, cadv_v4(0.75f, 0.75f, 0.7f, 1), buf);
    }
    /* team alive pips */
    for (i = 0; i < P07_AGENTS; ++i) {
        float x = i < P07_TEAM ? cx - 48.0f - (float)i * 6.0f : cx + 42.0f + (float)(i - P07_TEAM) * 6.0f;
        cadv_ui_rect(ui, x, 4, 4, 7, s->a[i].alive ? (s->a[i].team == 0 ? cadv_v4(0.4f, 0.8f, 1, 1) : cadv_v4(1, 0.4f, 0.3f, 1)) : cadv_v4(0.25f, 0.25f, 0.25f, 1));
    }
    if (s->phase == P07_PH_CELEBRATE || s->phase == P07_PH_OVER) {
        if (s->message_timer > 0) {
            cadv_ui_text_centered(ui, cx, cy - 30, 1, cadv_v4(1, 0.9f, 0.4f, 1), s->message);
        }
        if (s->phase == P07_PH_OVER) {
            draw_scoreboard(s, ui);
        }
        return;
    }
    /* crosshair grows with the spread */
    if (!a->scoped) {
        float speed = cadv_sqrtf(a->body.vel.x * a->body.vel.x + a->body.vel.z * a->body.vel.z);
        float gap = 3.0f + speed * 1.2f + (a->since_shot < 0.3f ? (float)cadv_mini(a->spray, 8) : 0.0f) + (a->body.on_ground ? 0.0f : 6.0f);
        cadv_vec4 c = cadv_v4(0.5f, 1, 0.5f, 1);
        cadv_ui_rect(ui, cx - gap - 4, cy, 4, 1, c);
        cadv_ui_rect(ui, cx + gap + 1, cy, 4, 1, c);
        cadv_ui_rect(ui, cx, cy - gap - 4, 1, 4, c);
        cadv_ui_rect(ui, cx, cy + gap + 1, 1, 4, c);
    } else {
        /* the scope ring */
        cadv_ui_rect(ui, 0, 0, cx - 60, H, cadv_v4(0, 0, 0, 1));
        cadv_ui_rect(ui, cx + 60, 0, cx - 60, H, cadv_v4(0, 0, 0, 1));
        cadv_ui_rect(ui, cx - 60, cy, 120, 1, cadv_v4(0, 0, 0, 1));
        cadv_ui_rect(ui, cx, 0, 1, H, cadv_v4(0, 0, 0, 1));
    }
    /* health, armour, money, ammo */
    cadv_ui_rect(ui, 0, H - 14, W, 14, cadv_v4(0, 0, 0, 0.5f));
    snprintf(buf, sizeof buf, "+%d", a->alive ? a->body.health : 0);
    cadv_ui_text(ui, 4, H - 11, 1, a->body.health > 30 ? warm : cadv_v4(1, 0.3f, 0.3f, 1), buf);
    snprintf(buf, sizeof buf, "ARM %d", a->body.armor);
    cadv_ui_text(ui, 40, H - 11, 1, cadv_v4(0.6f, 0.8f, 1, 1), buf);
    snprintf(buf, sizeof buf, "$%d", a->money);
    cadv_ui_text(ui, 92, H - 11, 1, cadv_v4(0.5f, 1, 0.5f, 1), buf);
    if (a->kit) {
        cadv_ui_text(ui, 140, H - 11, 1, cadv_v4(0.9f, 0.9f, 0.5f, 1), "KIT");
    }
    if (v == s->carrier) {
        cadv_ui_text(ui, 164, H - 11, 1, cadv_v4(1, 0.4f, 1, 1), "BOMB");
    }
    {
        const char *wn = a->slot == P07_SLOT_KNIFE ? "CANDY CANE" : a->slot == P07_SLOT_FLASH ? "FLASHBANG" : a->slot == P07_SLOT_SMOKE ? "SMOKE"
                         : cadv_p07_weapons()[a->slot == P07_SLOT_PRIMARY ? a->primary : a->pistol].name;
        if (a->slot == P07_SLOT_PRIMARY || a->slot == P07_SLOT_PISTOL) {
            int ms = a->slot == P07_SLOT_PRIMARY ? 2 : 1;
            snprintf(buf, sizeof buf, "%s %d/%d", wn, a->mag[ms], a->reserve[ms]);
        } else {
            snprintf(buf, sizeof buf, "%s", wn);
        }
        cadv_ui_text(ui, W - 6.0f * (float)strlen(buf) - 4, H - 11, 1, warm, buf);
    }
    /* planting / defusing progress */
    if (a->plant_t > 0.0f || a->defuse_t > 0.0f) {
        float need = a->plant_t > 0.0f ? (float)P07_PLANT * P07_DT : (float)(a->kit ? P07_DEFUSE_KIT : P07_DEFUSE) * P07_DT;
        float f = (a->plant_t > 0.0f ? a->plant_t : a->defuse_t) / need;
        cadv_ui_rect(ui, cx - 40, cy + 20, 80, 5, cadv_v4(0.1f, 0.1f, 0.1f, 0.8f));
        cadv_ui_rect(ui, cx - 40, cy + 20, 80.0f * f, 5, cadv_v4(1, 0.4f, 1, 1));
        cadv_ui_text_centered(ui, cx, cy + 27, 1, cadv_v4(1, 1, 1, 1), a->plant_t > 0.0f ? "PLANTING" : "DEFUSING");
    }
    /* kill feed and radio */
    for (i = 0; i < P07_FEED; ++i) {
        if (s->feed_t[i] > 0) {
            cadv_ui_text(ui, W - 6.0f * (float)strlen(s->feed[i]) - 4, 26 + (float)i * 9.0f, 1, cadv_v4(1, 1, 1, 0.9f), s->feed[i]);
        }
    }
    if (s->radio_timer > 0) {
        cadv_ui_text(ui, 4, H - 26, 1, cadv_v4(0.7f, 0.9f, 1, 1), s->radio);
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx, cy - 34, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->phase == P07_PH_FREEZE && !s->buy_open && s->a[0].alive) {
        cadv_ui_text_centered(ui, cx, cy + 40, 1, cadv_v4(0.9f, 0.9f, 0.7f, 1), "BUY TIME - PRESS 6 FOR THE BUY MENU");
    }
    if (!s->a[0].alive) {
        snprintf(buf, sizeof buf, "SPECTATING %s", s->names[v]);
        cadv_ui_text_centered(ui, cx, H - 38, 1, cadv_v4(0.8f, 0.8f, 0.8f, 1), buf);
    }
    if (s->buy_open) {
        draw_buy(s, ui);
    }
    if (s->map_held || s->phase == P07_PH_AFTER) {
        draw_scoreboard(s, ui);
    }
    /* flashed: the screen goes white */
    if (a->blind > 0.0f) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(1, 1, 1, cadv_minf(1.0f, a->blind / 1.5f)));
    }
    if (s->hurt_flash > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.8f, 0.1f, 0.1f, 0.025f * (float)s->hurt_flash));
    }
}

void cadv_p07_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p07_state *s = (p07_state *)state;
    int v = view_agent(s);
    float t = ((float)s->ticks + alpha) * P07_DT;
    (void)alpha;
    cam->pos = cadv_p07_eye(s, v);
    cam->yaw = s->a[v].body.yaw;
    cam->pitch = s->a[v].body.pitch;
    cam->fovy = s->a[v].scoped ? 0.35f : 1.25f;
    cam->znear = 0.05f;
    cam->zfar = 150.0f;
    /* a winter evening: blue dusk, warm stalls */
    sc->ambient = cadv_v3(0.32f, 0.33f, 0.42f);
    sc->sun_dir = cadv_v3_norm(cadv_v3(0.4f, -0.6f, 0.5f));
    sc->sun_color = cadv_v3(0.55f, 0.55f, 0.7f);
    sc->sun_shadow = true;
    sc->shadow_center = cadv_v3(cam->pos.x, 0, cam->pos.z);
    sc->shadow_extent = 30.0f;
    sc->clear_color = cadv_v3(0.15f, 0.17f, 0.3f);
    sc->fog_color = cadv_v3(0.22f, 0.24f, 0.36f);
    sc->fog_near = 40.0f;
    sc->fog_far = 120.0f;
    sc->light_bands = 6;
    sc->dither = 0.5f;
    sc->use_palette = true;
    draw_world(s, sc, cam, t);
    draw_view_weapon(s, sc, cam);
    if (ui) {
        draw_hud(s, ui);
    }
}

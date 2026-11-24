/* SPDX-License-Identifier: MIT */
/* GPU side of Day 9: Snowglobe Basin under an aurora, flat-shaded hills in
 * long fog, chunky armour in tribal colours with jet flames, discs as bright
 * lights that sweep across the snow, hill-top bases whose generators glow
 * (and go dark), flags on poles, the halfpipe sign, and the HUD: energy,
 * health, speed, score, flag markers and the kill feed. */
#include "portals/p09_skifrost_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/view.h"
#include "gfx/voxel.h"
#include "portal/help.h"
#include "portals/p09_internal.h"

static const cadv_vec4 k_team_tint[2] = {{0.62f, 0.8f, 1.0f, 1.0f}, {1.0f, 0.55f, 0.22f, 1.0f}};

static unsigned make_tex(cadv_tex_kind kind, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, kind, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

/* A painted sign: dark wood, pale letters. */
static unsigned sign_tex(const char *text, int *w_out)
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
        t.rgba[i * 4] = 70;
        t.rgba[i * 4 + 1] = 48;
        t.rgba[i * 4 + 2] = 34;
        t.rgba[i * 4 + 3] = 255;
    }
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y)) {
                    cadv_texdata_set(&t, 3 + i * 6 + x, 2 + y, 240, 236, 220, 255);
                }
            }
        }
    }
    tex = cadv_gl_texture(&t, false);
    *w_out = w;
    cadv_texdata_free(&t);
    return tex;
}

static cadv_rgba ground_colour(float steep)
{
    if (steep > 0.5f) {
        return cadv_rgba_make(100, 104, 120, 255); /* ice-scoured rock */
    }
    if (steep > 0.28f) {
        return cadv_rgba_make(185, 192, 208, 255);
    }
    return cadv_rgba_make(232, 236, 245, 255);
}

static void face(cadv_mesh *m, cadv_vec3 a, cadv_vec3 b, cadv_vec3 c)
{
    cadv_vec3 n = cadv_v3_norm(cadv_v3_cross(cadv_v3_sub(b, a), cadv_v3_sub(c, a)));
    cadv_rgba col = ground_colour(1.0f - n.y);
    uint32_t base = cadv_mesh_vertex(m, a, n, a.x / 8.0f, a.z / 8.0f, col);
    cadv_mesh_vertex(m, b, n, b.x / 8.0f, b.z / 8.0f, col);
    cadv_mesh_vertex(m, c, n, c.x / 8.0f, c.z / 8.0f, col);
    cadv_mesh_tri(m, base, base + 1, base + 2);
}

static void build_chunk(const p09_state *s, int cx, int cz, cadv_mesh *m)
{
    const cadv_terrain *t = &s->terrain;
    int x;
    int z;
    int x0 = cx * CADV_TERRAIN_CHUNK;
    int z0 = cz * CADV_TERRAIN_CHUNK;
    cadv_mesh_clear(m);
    for (z = 0; z < CADV_TERRAIN_CHUNK; ++z) {
        for (x = 0; x < CADV_TERRAIN_CHUNK; ++x) {
            int gx = x0 + x;
            int gz = z0 + z;
            cadv_vec3 a = cadv_v3((float)gx * t->cell, cadv_terrain_sample(t, gx, gz), (float)gz * t->cell);
            cadv_vec3 b = cadv_v3((float)(gx + 1) * t->cell, cadv_terrain_sample(t, gx + 1, gz), (float)gz * t->cell);
            cadv_vec3 c = cadv_v3((float)gx * t->cell, cadv_terrain_sample(t, gx, gz + 1), (float)(gz + 1) * t->cell);
            cadv_vec3 d = cadv_v3((float)(gx + 1) * t->cell, cadv_terrain_sample(t, gx + 1, gz + 1), (float)(gz + 1) * t->cell);
            face(m, a, c, b);
            face(m, b, c, d);
        }
    }
}

static void refresh_terrain(p09_state *s)
{
    int c = cadv_terrain_chunks_per_side(&s->terrain);
    int i;
    cadv_mesh m;
    bool any = false;
    for (i = 0; i < c * c; ++i) {
        if (s->g.chunk_seen[i] != s->terrain.chunk_ver[i]) {
            if (!any) {
                cadv_mesh_init(&m);
                any = true;
            }
            build_chunk(s, i % c, i / c, &m);
            cadv_gpu_mesh_free(&s->g.chunk[i]);
            cadv_gpu_mesh_upload(&s->g.chunk[i], &m, false);
            s->g.chunk_seen[i] = s->terrain.chunk_ver[i];
        }
    }
    if (any) {
        cadv_mesh_free(&m);
    }
}

static void box(cadv_mesh *m, float x0, float y0, float z0, float x1, float y1, float z1, cadv_rgba c)
{
    cadv_mesh_box(m, cadv_v3(x0, y0, z0), cadv_v3(x1, y1, z1), 0.0f, c, false);
}

static void upload(cadv_gpu_mesh *g, cadv_mesh *m)
{
    cadv_gpu_mesh_upload(g, m, false);
    cadv_mesh_clear(m);
}

void cadv_p09_gpu_init(void *state)
{
    p09_state *s = (p09_state *)state;
    p09_gpu *g = &s->g;
    cadv_mesh m;
    cadv_voxels v;
    cadv_rgba white = cadv_rgba_make(255, 255, 255, 255);
    cadv_rgba dark = cadv_rgba_make(55, 58, 66, 255);
    cadv_rgba steel = cadv_rgba_make(150, 156, 168, 255);
    char sign[72];
    int x;
    int z;
    int k;
    memset(g, 0, sizeof *g);
    g->tex_snow = make_tex(CADV_TEX_NOISE, 91u, cadv_rgba_make(232, 236, 246, 255), cadv_rgba_make(250, 252, 255, 255));
    g->tex_rock = make_tex(CADV_TEX_NOISE, 92u, cadv_rgba_make(110, 112, 125, 255), cadv_rgba_make(150, 152, 165, 255));
    snprintf(sign, sizeof sign, "THE %s", s->mark_text[3]);
    g->tex_mark = sign_tex(sign, &g->mark_w);
    cadv_mesh_init(&m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, white, false);
    upload(&g->cube, &m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f, white);
    upload(&g->plaque, &m);
    /* armour: wider and taller with the class; tinted by tribe */
    for (k = 0; k < CADV_ARMOURS; ++k) {
        float w = 0.38f + 0.1f * (float)k;
        float hgt = 1.25f + 0.12f * (float)k;
        box(&m, -w, 0.75f, -0.25f - 0.04f * (float)k, w, hgt + 0.2f, 0.25f + 0.04f * (float)k, white);     /* torso */
        box(&m, -w - 0.18f, hgt - 0.1f, -0.3f, w + 0.18f, hgt + 0.25f, 0.3f, white);                         /* shoulders */
        box(&m, -w + 0.05f, 0.0f, -0.18f, -0.04f, 0.8f, 0.18f, dark);                                         /* legs */
        box(&m, 0.04f, 0.0f, -0.18f, w - 0.05f, 0.8f, 0.18f, dark);
        box(&m, w + 0.02f, hgt - 0.6f, -0.65f, w + 0.18f, hgt - 0.4f, 0.0f, dark);                            /* the launcher */
        box(&m, -0.3f, 0.0f, -0.6f, 0.3f, 0.06f, 0.5f, steel);                                                /* skis */
        upload(&g->armour[k], &m);
    }
    box(&m, -0.2f, 0.0f, -0.2f, 0.2f, 0.36f, 0.22f, white);
    box(&m, -0.16f, 0.12f, -0.23f, 0.16f, 0.24f, -0.2f, cadv_rgba_make(255, 220, 120, 255)); /* visor */
    upload(&g->head, &m);
    box(&m, -0.3f, 0.0f, 0.0f, 0.3f, 0.7f, 0.3f, dark);
    box(&m, -0.24f, -0.12f, 0.06f, -0.08f, 0.0f, 0.22f, steel);
    box(&m, 0.08f, -0.12f, 0.06f, 0.24f, 0.0f, 0.22f, steel);
    upload(&g->pack, &m);
    /* flag cloth and pole */
    box(&m, -0.05f, 0.0f, -0.05f, 0.05f, 3.2f, 0.05f, steel);
    upload(&g->pole, &m);
    cadv_mesh_quad(&m, cadv_v3(0, 2.0f, 0), cadv_v3(1.6f, 2.1f, 0), cadv_v3(1.6f, 3.1f, 0), cadv_v3(0, 3.1f, 0), 1.0f, 1.0f, white);
    cadv_mesh_quad(&m, cadv_v3(0, 3.1f, 0), cadv_v3(1.6f, 3.1f, 0), cadv_v3(1.6f, 2.1f, 0), cadv_v3(0, 2.0f, 0), 1.0f, 1.0f, white);
    upload(&g->flag, &m);
    /* base pieces, around their own origins */
    box(&m, -1.5f, -1.0f, -1.5f, 1.5f, 0.0f, 1.5f, steel);       /* the stand (top at 0) */
    box(&m, -1.2f, -0.05f, -1.2f, 1.2f, 0.02f, 1.2f, white);
    upload(&g->base, &m);
    box(&m, -1.5f, -1.6f, -1.5f, 1.5f, 1.6f, 1.5f, dark);        /* generator housing */
    box(&m, -1.55f, -0.4f, -1.55f, 1.55f, 0.4f, 1.55f, white);   /* the glowing band (tinted) */
    upload(&g->gen, &m);
    box(&m, -0.5f, -6.6f, -0.5f, 0.5f, -0.6f, 0.5f, steel);      /* pillar (pos is the head) */
    box(&m, -0.9f, -0.6f, -0.9f, 0.9f, 0.4f, 0.9f, dark);
    upload(&g->turret, &m);
    box(&m, -0.12f, -0.12f, -1.9f, 0.12f, 0.12f, -0.6f, dark);   /* the barrel, turned to the target */
    upload(&g->ring, &m);
    box(&m, -0.8f, 0.0f, -0.8f, 0.8f, 2.2f, 0.8f, steel);        /* inventory station */
    box(&m, -0.6f, 1.2f, -0.82f, 0.6f, 1.9f, 0.82f, white);
    upload(&g->station, &m);
    /* aurora curtains over the globe */
    for (k = 0; k < 3; ++k) {
        float z0 = 120.0f + 120.0f * (float)k;
        int i;
        for (i = 0; i < 24; ++i) {
            float x0 = -60.0f + (float)i * 632.0f / 24.0f;
            float x1 = x0 + 632.0f / 24.0f;
            float za = z0 + 30.0f * cadv_sinf(x0 * 0.02f + (float)k);
            float zb = z0 + 30.0f * cadv_sinf(x1 * 0.02f + (float)k);
            float y0 = 170.0f + 14.0f * (float)k;
            uint32_t a = cadv_mesh_vertex(&m, cadv_v3(x0, y0, za), cadv_v3(0, 0, 1), 0, 0, cadv_rgba_make(40, 255, 150, 255));
            uint32_t b = cadv_mesh_vertex(&m, cadv_v3(x1, y0, zb), cadv_v3(0, 0, 1), 1, 0, cadv_rgba_make(40, 255, 150, 255));
            uint32_t c = cadv_mesh_vertex(&m, cadv_v3(x1, y0 + 50.0f, zb + 10.0f), cadv_v3(0, 0, 1), 1, 1, cadv_rgba_make(140, 60, 255, 255));
            uint32_t d = cadv_mesh_vertex(&m, cadv_v3(x0, y0 + 50.0f, za + 10.0f), cadv_v3(0, 0, 1), 0, 1, cadv_rgba_make(140, 60, 255, 255));
            cadv_mesh_tri(&m, a, b, c);
            cadv_mesh_tri(&m, a, c, d);
            cadv_mesh_tri(&m, a, c, b);
            cadv_mesh_tri(&m, a, d, c);
        }
    }
    upload(&g->aurora, &m);
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
    refresh_terrain(s);
    s->gpu_ready = true;
}

void cadv_p09_gpu_free(void *state)
{
    p09_state *s = (p09_state *)state;
    p09_gpu *g = &s->g;
    int i;
    for (i = 0; i < CADV_TERRAIN_MAX_CHUNKS; ++i) {
        cadv_gpu_mesh_free(&g->chunk[i]);
        g->chunk_seen[i] = 0;
    }
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->aurora);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->flag);
    cadv_gpu_mesh_free(&g->pole);
    for (i = 0; i < CADV_ARMOURS; ++i) {
        cadv_gpu_mesh_free(&g->armour[i]);
    }
    cadv_gpu_mesh_free(&g->head);
    cadv_gpu_mesh_free(&g->pack);
    cadv_gpu_mesh_free(&g->base);
    cadv_gpu_mesh_free(&g->gen);
    cadv_gpu_mesh_free(&g->turret);
    cadv_gpu_mesh_free(&g->station);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->ring);
    cadv_gl_delete_textures(1, &g->tex_snow);
    cadv_gl_delete_textures(1, &g->tex_rock);
    cadv_gl_delete_textures(1, &g->tex_mark);
    s->gpu_ready = false;
}

/* ---- the world ------------------------------------------------------------------- */

static void draw_flag(p09_state *s, cadv_scene *sc, int team, float t)
{
    const cadv_flag *f = &s->flag[team];
    cadv_vec3 p = f->pos;
    float yaw = t * 0.8f + (float)team;
    if (f->state == CADV_FLAG_CARRIED) {
        const p09_agent *c = &s->a[f->carrier];
        if (f->carrier == 0) {
            return; /* on your own back: out of view */
        }
        p = cadv_v3(c->k.pos.x + cadv_sinf(c->k.yaw) * 0.4f, c->k.pos.y + 0.6f, c->k.pos.z + cadv_cosf(c->k.yaw) * 0.4f);
        yaw = c->k.yaw + CADV_PI * 0.5f;
    }
    cadv_scene_draw(sc, &s->g.pole, cadv_m4_translate(p), cadv_material_make(0, cadv_v4(1, 1, 1, 1), 0.0f, 0));
    cadv_scene_draw(sc, &s->g.flag, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(yaw + 0.2f * cadv_sinf(t * 3.0f))),
                    cadv_material_make(0, k_team_tint[team], 0.5f, CADV_MAT_NO_SHADOW));
}

static void draw_agent(p09_state *s, cadv_scene *sc, int i, float t, int *lights)
{
    const p09_agent *a = &s->a[i];
    cadv_mat4 at = cadv_m4_mul(cadv_m4_translate(a->k.pos), cadv_m4_rotate_y(a->k.yaw));
    cadv_vec4 tint = k_team_tint[a->team];
    float hgt = 1.25f + 0.12f * (float)a->k.armour;
    if (a->invuln > 0.0f && s->ticks % 8 < 3) {
        tint = cadv_v4(1.4f, 1.4f, 1.4f, 1);
    }
    cadv_scene_draw(sc, &s->g.armour[a->k.armour], at, cadv_material_make(0, tint, 0.05f, 0));
    cadv_scene_draw(sc, &s->g.head, cadv_m4_mul(at, cadv_m4_translate(cadv_v3(0, hgt + 0.22f, 0))), cadv_material_make(0, tint, 0.05f, 0));
    cadv_scene_draw(sc, &s->g.pack, cadv_m4_mul(at, cadv_m4_translate(cadv_v3(0, hgt - 0.55f, 0.25f))), cadv_material_make(0, cadv_v4(1, 1, 1, 1), 0.0f, 0));
    if (a->k.jetting) {
        float fl = 0.5f + 0.3f * cadv_sinf(t * 40.0f + (float)i);
        cadv_mat4 m = cadv_m4_mul(at, cadv_m4_mul(cadv_m4_translate(cadv_v3(0, hgt - 0.95f, 0.4f)), cadv_m4_scale(cadv_v3(0.35f, fl, 0.25f))));
        cadv_scene_draw(sc, &s->g.cube, m, cadv_material_make(0, cadv_v4(1.0f, 0.75f, 0.3f, 1), 2.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
        if (*lights < 20) {
            cadv_scene_light(sc, cadv_v3(a->k.pos.x, a->k.pos.y + 0.3f, a->k.pos.z), cadv_v3(1.0f, 0.6f, 0.25f), 8.0f, false);
            ++*lights;
        }
    }
    (void)t;
}

static void draw_base(p09_state *s, cadv_scene *sc, int b, float t, int *lights)
{
    const p09_base *pb = &s->base[b];
    bool on = cadv_generator_powered(&pb->power);
    cadv_vec4 glow = on ? cadv_v4(k_team_tint[b].x * 1.5f, k_team_tint[b].y * 1.5f, k_team_tint[b].z * 1.5f, 1) : cadv_v4(0.15f, 0.15f, 0.17f, 1);
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float turret_yaw = t * 0.4f;
    float sg = b == 0 ? 1.0f : -1.0f;
    cadv_scene_draw(sc, &s->g.base, cadv_m4_translate(pb->stand), cadv_material_make(0, white, 0.0f, 0));
    cadv_scene_draw(sc, &s->g.gen, cadv_m4_translate(pb->gen), cadv_material_make(0, glow, on ? 1.0f : 0.0f, 0));
    cadv_scene_draw(sc, &s->g.cube, cadv_m4_mul(cadv_m4_translate(cadv_v3(pb->gen.x, pb->gen.y - 0.6f, pb->gen.z + sg * 3.5f)), cadv_m4_scale(cadv_v3(8.0f, 2.0f, 0.8f))),
                    cadv_material_make(s->g.tex_rock, cadv_v4(0.8f, 0.8f, 0.85f, 1), 0.0f, 0));
    if (pb->turret_target >= 0 && on) {
        turret_yaw = cadv_atan2f(-(s->a[pb->turret_target].k.pos.x - pb->turret.pos.x), -(s->a[pb->turret_target].k.pos.z - pb->turret.pos.z));
    }
    cadv_scene_draw(sc, &s->g.turret, cadv_m4_translate(pb->turret.pos), cadv_material_make(0, white, 0.0f, 0));
    cadv_scene_draw(sc, &s->g.ring, cadv_m4_mul(cadv_m4_translate(pb->turret.pos), cadv_m4_rotate_y(turret_yaw)), cadv_material_make(0, glow, on ? 0.6f : 0.0f, 0));
    cadv_scene_draw(sc, &s->g.station, cadv_m4_translate(pb->station), cadv_material_make(0, glow, on ? 0.6f : 0.0f, 0));
    if (on && *lights < 20) {
        cadv_scene_light(sc, cadv_v3(pb->gen.x, pb->gen.y + 2.0f, pb->gen.z), cadv_v3(k_team_tint[b].x, k_team_tint[b].y, k_team_tint[b].z), 16.0f, false);
        ++*lights;
    }
}

static void draw_world(p09_state *s, cadv_scene *sc, float t)
{
    p09_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    int chunks = cadv_terrain_chunks_per_side(&s->terrain);
    int lights = 0;
    int i;
    for (i = 0; i < chunks * chunks; ++i) {
        cadv_scene_draw(sc, &g->chunk[i], cadv_m4_identity(), cadv_material_make(g->tex_snow, white, 0.0f, 0));
    }
    cadv_scene_draw(sc, &g->aurora, cadv_m4_translate(cadv_v3(6.0f * cadv_sinf(t * 0.15f), 0, 0)),
                    cadv_material_make(0, white, 1.1f + 0.2f * cadv_sinf(t * 0.6f), CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW | CADV_MAT_NO_FOG));
    for (i = 0; i < 2; ++i) {
        draw_base(s, sc, i, t, &lights);
        draw_flag(s, sc, i, t);
    }
    /* the halfpipe sign on two posts at its north end */
    {
        float y = cadv_terrain_height(&s->terrain, 112.0f, 330.0f);
        cadv_mat4 m = cadv_m4_mul(cadv_m4_translate(cadv_v3(130.0f, y + 2.2f, 334.0f)), cadv_m4_scale(cadv_v3((float)g->mark_w / 8.0f, 1.6f, 1)));
        cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(130.0f, y + 2.2f, 334.0f)), cadv_m4_mul(cadv_m4_rotate_y(CADV_PI), cadv_m4_scale(cadv_v3((float)g->mark_w / 8.0f, 1.6f, 1)))),
                        cadv_material_make(g->tex_mark, white, 0.3f, CADV_MAT_NO_SHADOW));
        cadv_scene_draw(sc, &g->plaque, m, cadv_material_make(g->tex_mark, white, 0.3f, CADV_MAT_NO_SHADOW));
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(cadv_v3(130.0f - (float)g->mark_w / 16.0f, y + 1.5f, 334.0f)), cadv_m4_scale(cadv_v3(0.2f, 3.0f, 0.2f))),
                        cadv_material_make(0, cadv_v4(0.4f, 0.3f, 0.2f, 1), 0.0f, 0));
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(cadv_v3(130.0f + (float)g->mark_w / 16.0f, y + 1.5f, 334.0f)), cadv_m4_scale(cadv_v3(0.2f, 3.0f, 0.2f))),
                        cadv_material_make(0, cadv_v4(0.4f, 0.3f, 0.2f, 1), 0.0f, 0));
    }
    for (i = 1; i < P09_AGENTS; ++i) {
        if (s->a[i].alive) {
            draw_agent(s, sc, i, t, &lights);
        }
    }
    /* discs: bright lights skimming the snow */
    for (i = 0; i < P09_PROJ; ++i) {
        const p09_proj *p = &s->proj[i];
        if (!p->active) {
            continue;
        }
        if (p->kind == P09_PJ_GRENADE) {
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(p->pos), cadv_m4_scale(cadv_v3(0.25f, 0.25f, 0.25f))), cadv_material_make(0, cadv_v4(0.3f, 0.35f, 0.3f, 1), 0.0f, 0));
        } else {
            cadv_vec4 c = p->team == 0 ? cadv_v4(0.7f, 0.9f, 1.4f, 1) : cadv_v4(1.4f, 0.8f, 0.4f, 1);
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(p->pos), cadv_m4_mul(cadv_m4_rotate_y(t * 20.0f), cadv_m4_scale(cadv_v3(0.55f, 0.12f, 0.55f)))),
                            cadv_material_make(0, c, 2.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            if (lights < 24) {
                cadv_scene_light(sc, p->pos, cadv_v3(c.x, c.y, c.z), 12.0f, false);
                ++lights;
            }
        }
    }
    if (s->boom_t > 0) {
        float k = (float)s->boom_t / 12.0f;
        float sz = 7.0f * (1.0f - k) + 1.0f;
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(s->boom_pos), cadv_m4_scale(cadv_v3(sz, sz, sz))),
                        cadv_material_make(0, cadv_v4(1.5f * k, 1.1f * k, 0.6f * k, 1), 2.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, s->boom_pos, cadv_v3(1.6f * k, 1.1f * k, 0.6f * k), 20.0f, false);
    }
    if (s->phase == P09_PH_TRAIN) {
        if (s->train == P09_TR_JET) {
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(s->beacon), cadv_m4_mul(cadv_m4_rotate_y(t), cadv_m4_scale(cadv_v3(1.2f, 1.2f, 1.2f)))),
                            cadv_material_make(0, cadv_v4(0.5f, 1.5f, 0.8f, 1), 2.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(cadv_v3(s->beacon.x, s->beacon.y - 12.0f, s->beacon.z)), cadv_m4_scale(cadv_v3(0.15f, 24.0f, 0.15f))),
                            cadv_material_make(0, cadv_v4(0.4f, 1.2f, 0.7f, 1), 1.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            cadv_scene_light(sc, s->beacon, cadv_v3(0.4f, 1.2f, 0.6f), 18.0f, false);
        } else if (s->train == P09_TR_DISC) {
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(s->drone), cadv_m4_mul(cadv_m4_rotate_y(t * 3.0f), cadv_m4_scale(cadv_v3(1.4f, 0.5f, 1.4f)))),
                            cadv_material_make(0, cadv_v4(1.4f, 0.4f, 0.4f, 1), 1.0f, CADV_MAT_NO_SHADOW));
        }
    }
    if (s->phase == P09_PH_LAP && !s->star_taken) {
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(s->star_pos), cadv_m4_mul(cadv_m4_rotate_y(t * 1.5f), cadv_m4_translate(cadv_v3(-1.5f, -1.5f, -0.3f)))),
                        cadv_material_make(0, cadv_v4(1.3f, 1.2f, 0.7f, 1), 1.5f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, s->star_pos, cadv_v3(1.4f, 1.2f, 0.6f), 22.0f, false);
    }
}

static float flat_dist(cadv_vec3 a, cadv_vec3 b) { return cadv_sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

/* ---- HUD -------------------------------------------------------------------------------- */

/* world point to UI pixels; false when behind the camera */
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

static void bar(cadv_ui *ui, float x, float y, float w, float frac, cadv_vec4 c, const char *label)
{
    cadv_ui_rect(ui, x - 1, y - 1, w + 2, 7, cadv_v4(0, 0, 0, 0.6f));
    cadv_ui_rect(ui, x, y, w * cadv_clampf(frac, 0.0f, 1.0f), 5, c);
    cadv_ui_text(ui, x, y - 9, 1, cadv_v4(0.9f, 0.95f, 1, 1), label);
}

static void draw_hud(p09_state *s, const cadv_camera *cam, cadv_ui *ui)
{
    const p09_agent *me = &s->a[0];
    float W = (float)ui->width;
    float H = (float)ui->height;
    float cx = W * 0.5f;
    float cy = H * 0.5f;
    char buf[96];
    int i;
    int t;
    /* crosshair */
    if (me->alive) {
        cadv_vec4 ch = s->hit_mark > 0 ? cadv_v4(1, 0.4f, 0.3f, 1) : cadv_v4(0.9f, 1, 0.9f, 0.9f);
        cadv_ui_rect(ui, cx - 6, cy, 4, 1, ch);
        cadv_ui_rect(ui, cx + 3, cy, 4, 1, ch);
        cadv_ui_rect(ui, cx, cy - 6, 1, 4, ch);
        cadv_ui_rect(ui, cx, cy + 3, 1, 4, ch);
    }
    /* score and clock */
    cadv_ui_rect(ui, cx - 52, 3, 104, 14, cadv_v4(0, 0, 0, 0.5f));
    snprintf(buf, sizeof buf, "WOLVES %d-%d OWLS", s->score[0], s->score[1]);
    cadv_ui_text_centered(ui, cx, 6, 1, cadv_v4(1, 1, 1, 1), buf);
    if (s->phase == P09_PH_MATCH) {
        int sec = s->timer > 0 ? s->timer / 60 : 0;
        snprintf(buf, sizeof buf, s->sudden ? "SUDDEN DEATH" : "%d:%02d", sec / 60, sec % 60);
        cadv_ui_text_centered(ui, cx, 19, 1, cadv_v4(0.8f, 0.9f, 1, 1), buf);
    } else if (s->phase == P09_PH_COUNT) {
        snprintf(buf, sizeof buf, "MATCH STARTS IN %d", s->timer / 60 + 1);
        cadv_ui_text_centered(ui, cx, cy - 30, 2, cadv_v4(1, 0.9f, 0.5f, 1), buf);
    }
    /* flag states */
    for (t = 0; t < 2; ++t) {
        static const char *const st[3] = {"HOME", "TAKEN", "DROPPED"};
        cadv_vec4 c = k_team_tint[t];
        snprintf(buf, sizeof buf, "%s %s", t == 0 ? "OURS" : "THEIRS", st[s->flag[t].state]);
        cadv_ui_text(ui, 6, 6 + (float)t * 10, 1, c, buf);
    }
    /* markers over the flags (and the star), even through hills */
    for (t = 0; t < 3; ++t) {
        cadv_vec3 p;
        float sx;
        float sy;
        cadv_vec4 c;
        if (t < 2) {
            if (s->flag[t].state == CADV_FLAG_CARRIED && s->flag[t].carrier == 0) {
                continue;
            }
            p = cadv_v3_add(s->flag[t].pos, cadv_v3(0, 4.0f, 0));
            c = k_team_tint[t];
        } else {
            if (s->phase != P09_PH_LAP || s->star_carried) {
                continue;
            }
            p = s->star_pos;
            c = cadv_v4(1, 0.9f, 0.4f, 1);
        }
        if (s->phase != P09_PH_TRAIN && to_screen(cam, ui, p, &sx, &sy)) {
            float d = cadv_v3_dist(p, cam->pos);
            cadv_ui_rect(ui, sx - 3, sy - 3, 6, 6, c);
            snprintf(buf, sizeof buf, "%dM", (int)d);
            cadv_ui_text_centered(ui, sx, sy + 5, 1, c, buf);
        }
    }
    /* kill feed, under the clock on the right */
    for (i = 0; i < 3; ++i) {
        if (s->feed_t[i] > 0) {
            float a = cadv_minf(1.0f, (float)s->feed_t[i] / 60.0f);
            int w = (int)strlen(s->feed[i]) * 6;
            cadv_ui_text(ui, W - 4 - (float)w, 31 + (float)i * 9, 1, cadv_v4(1, 0.95f, 0.85f, a), s->feed[i]);
        }
    }
    /* energy, health, speed, loadout */
    if (me->alive) {
        const cadv_armour_def *ad = cadv_armour_get(me->k.armour);
        bar(ui, 6, H - 9, 60, me->k.hp / ad->hp, cadv_v4(0.9f, 0.3f, 0.25f, 1), "HEALTH");
        bar(ui, W - 66, H - 9, 60, me->k.energy / ad->energy_max, me->k.jet_lock > 0.0f ? cadv_v4(0.5f, 0.5f, 0.5f, 1) : cadv_v4(0.35f, 0.75f, 1.0f, 1), "ENERGY");
        snprintf(buf, sizeof buf, "%d KM/H%s", (int)(cadv_skier_speed(&me->k) * 3.6f), me->k.skiing ? " SKI" : "");
        cadv_ui_text_centered(ui, cx, H - 34, 2, cadv_skier_speed(&me->k) > P09_FAST ? cadv_v4(1, 0.8f, 0.3f, 1) : cadv_v4(0.9f, 0.95f, 1, 1), buf);
        snprintf(buf, sizeof buf, "%s %s", ad->name, me->weapon == P09_DISC ? "DISC" : me->weapon == P09_CHAIN ? "CHAINGUN" : "GRENADES");
        cadv_ui_rect(ui, cx - (float)strlen(buf) * 3.0f - 3, H - 13, (float)strlen(buf) * 6.0f + 6, 11, cadv_v4(0, 0, 0, 0.5f));
        cadv_ui_text_centered(ui, cx, H - 11, 1, cadv_v4(0.85f, 0.9f, 1, 1), buf);
        if (s->flag[1].carrier == 0) {
            cadv_ui_text_centered(ui, cx, cy + 22, 1, k_team_tint[1], "FLAG! SKI HOME!");
        }
        if (s->phase == P09_PH_MATCH && flat_dist(me->k.pos, s->base[0].station) < 3.5f) {
            cadv_ui_text_centered(ui, cx, cy + 34, 1, cadv_v4(0.8f, 1, 0.8f, 1), "E: CHANGE ARMOUR");
        }
    } else if (s->phase == P09_PH_MATCH) {
        snprintf(buf, sizeof buf, "RESPAWN IN %d", me->respawn / 60 + 1);
        cadv_ui_text_centered(ui, cx, cy - 4, 2, cadv_v4(1, 0.7f, 0.6f, 1), buf);
    }
    if (s->phase == P09_PH_TRAIN) {
        static const char *const step[3] = {"1/3  SKI", "2/3  JET", "3/3  DISC"};
        cadv_ui_rect(ui, cx - 60, 30, 120, 12, cadv_v4(0, 0, 0, 0.5f));
        cadv_ui_text_centered(ui, cx, 32, 1, cadv_v4(1, 0.9f, 0.5f, 1), step[s->train < 3 ? s->train : 2]);
        cadv_ui_text_centered(ui, cx, 45, 1, cadv_v4(0.7f, 0.75f, 0.8f, 1), "E: SKIP TRAINING");
    }
    if (s->message_timer > 0) {
        float a = cadv_minf(1.0f, (float)s->message_timer / 40.0f);
        char lines[3][64];
        int n = cadv_help_wrap(s->message, (int)(W / 6.0f) - 2, lines, 3);
        cadv_ui_rect(ui, 0, cy - 60, W, 4 + 9 * (float)n, cadv_v4(0, 0, 0, 0.45f * a));
        for (i = 0; i < n; ++i) {
            cadv_ui_text_centered(ui, cx, cy - 58 + 9 * (float)i, 1, cadv_v4(1, 0.95f, 0.8f, a), lines[i]);
        }
    }
    if (s->map_held) {
        /* the scoreboard */
        cadv_ui_rect(ui, cx - 100, cy - 48, 200, 92, cadv_v4(0, 0, 0, 0.75f));
        for (i = 0; i < P09_AGENTS; ++i) {
            const p09_agent *a = &s->a[i];
            snprintf(buf, sizeof buf, "%-12.12s %c K%2d D%2d C%d", s->names[i], cadv_armour_get(a->k.armour)->name[0], a->kills, a->deaths, a->caps);
            cadv_ui_text(ui, cx - 96, cy - 44 + (float)i * 11, 1, k_team_tint[a->team], buf);
        }
    }
    if (s->hurt_flash > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.8f, 0.2f, 0.1f, 0.03f * (float)s->hurt_flash));
    }
    if (s->end_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, W, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, s->won ? cadv_v4(1, 0.85f, 0.4f, 1) : cadv_v4(1, 0.5f, 0.4f, 1),
                              s->won ? "CHAMPIONS!" : "THE OWLS WIN");
    }
}

void cadv_p09_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p09_state *s = (p09_state *)state;
    const p09_agent *me = &s->a[0];
    float t = ((float)s->ticks + alpha) * P09_DT;
    refresh_terrain(s);
    cam->pos = cadv_skier_eye(&me->k);
    if (!me->alive) {
        cam->pos.y += 3.0f; /* floating over where you fell */
    }
    cam->yaw = me->k.yaw;
    cam->pitch = me->k.pitch;
    /* speed widens the view a little */
    cam->fovy = 1.15f + cadv_minf(0.25f, cadv_skier_speed(&me->k) / 200.0f);
    cam->znear = 0.15f;
    cam->zfar = 420.0f;
    sc->ambient = cadv_v3(0.26f, 0.3f, 0.4f);
    sc->sun_color = cadv_v3(0.75f, 0.78f, 0.95f);
    sc->clear_color = cadv_v3(0.05f, 0.08f, 0.16f);
    sc->fog_color = cadv_v3(0.16f, 0.2f, 0.3f);
    sc->sun_dir = cadv_v3_norm(cadv_v3(0.4f, -0.6f, -0.5f));
    sc->sun_shadow = true;
    sc->shadow_center = cadv_v3(cam->pos.x, 0, cam->pos.z);
    sc->shadow_extent = 60.0f;
    sc->fog_near = 140.0f;
    sc->fog_far = 400.0f;
    sc->light_bands = 5;
    sc->dither = 0.5f;
    sc->use_palette = true;
    draw_world(s, sc, t);
    /* your own launcher in the corner */
    if (me->alive) {
        cadv_mat4 view = cadv_m4_mul(cadv_m4_translate(cam->pos), cadv_m4_mul(cadv_m4_rotate_y(cam->yaw), cadv_m4_rotate_x(cam->pitch)));
        float kick = me->cd > 0.9f ? (me->cd - 0.9f) * 0.4f : 0.0f;
        cadv_vec4 c = me->weapon == P09_DISC ? cadv_v4(0.5f, 0.55f, 0.7f, 1) : me->weapon == P09_CHAIN ? cadv_v4(0.45f, 0.45f, 0.45f, 1) : cadv_v4(0.4f, 0.5f, 0.35f, 1);
        cadv_scene_draw(sc, &s->g.cube, cadv_m4_mul(view, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.26f, -0.3f, -0.7f + kick)), cadv_m4_scale(cadv_v3(0.09f, 0.09f, 0.5f)))),
                        cadv_material_make(0, c, 0.1f, CADV_MAT_NO_SHADOW));
        if (me->weapon == P09_DISC && me->cd <= 0.0f) {
            cadv_scene_draw(sc, &s->g.cube, cadv_m4_mul(view, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.26f, -0.24f, -0.9f)), cadv_m4_scale(cadv_v3(0.12f, 0.025f, 0.12f)))),
                            cadv_material_make(0, cadv_v4(0.7f, 0.9f, 1.4f, 1), 1.5f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
        }
        if (s->star_carried && !s->star_taken) {
            cadv_scene_draw(sc, &s->g.star, cadv_m4_mul(view, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.5f, -0.35f, -1.0f)), cadv_m4_scale(cadv_v3(0.12f, 0.12f, 0.12f)))),
                            cadv_material_make(0, cadv_v4(1.3f, 1.2f, 0.7f, 1), 1.5f, CADV_MAT_NO_SHADOW));
        }
    }
    if (ui) {
        draw_hud(s, cam, ui);
    }
}

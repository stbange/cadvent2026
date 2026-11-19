/* SPDX-License-Identifier: MIT */
/* GPU side of Day 6: a flat-shaded Winterhold valley in fog, walkers built
 * from blocky parts with a walk cycle, Santa Claude's red-and-white crawler,
 * laser and PPC arcs lighting the snow, missile trails, steam vents, and the
 * cockpit: struts, heat gauge, throttle lever, damage silhouette, weapon
 * groups, radar and the mech lab. */
#include "portals/p06_iron_yule_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portal/help.h"
#include "portals/p06_internal.h"

static unsigned make_tex(cadv_tex_kind kind, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, kind, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

/* Stencil lettering sprayed on a wreck: pale paint on dark armour. */
static unsigned stencil_tex(const char *text, int *w_out)
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
        t.rgba[i * 4] = 40;
        t.rgba[i * 4 + 1] = 40;
        t.rgba[i * 4 + 2] = 45;
        t.rgba[i * 4 + 3] = 255;
    }
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y) && (x + y + i) % 5 != 0) {
                    cadv_texdata_set(&t, 3 + i * 6 + x, 2 + y, 230, 60, 50, 255);
                }
            }
        }
    }
    tex = cadv_gl_texture(&t, false);
    *w_out = w;
    cadv_texdata_free(&t);
    return tex;
}

static cadv_rgba ground_colour(float h, float steep)
{
    if (h < P06_SEA + 0.3f) {
        return cadv_rgba_make(150, 160, 170, 255); /* frozen shore */
    }
    if (steep > 0.45f) {
        return cadv_rgba_make(95, 95, 105, 255); /* bare rock on the cliffs */
    }
    if (steep > 0.25f) {
        return cadv_rgba_make(170, 175, 185, 255);
    }
    return cadv_rgba_make(225, 230, 238, 255);
}

/* Flat shading: every triangle carries its own face normal (the era's look). */
static void face(cadv_mesh *m, cadv_vec3 a, cadv_vec3 b, cadv_vec3 c)
{
    cadv_vec3 n = cadv_v3_norm(cadv_v3_cross(cadv_v3_sub(b, a), cadv_v3_sub(c, a)));
    float cy = (a.y + b.y + c.y) / 3.0f;
    cadv_rgba col = ground_colour(cy, 1.0f - n.y);
    uint32_t base = cadv_mesh_vertex(m, a, n, a.x / 8.0f, a.z / 8.0f, col);
    cadv_mesh_vertex(m, b, n, b.x / 8.0f, b.z / 8.0f, col);
    cadv_mesh_vertex(m, c, n, c.x / 8.0f, c.z / 8.0f, col);
    cadv_mesh_tri(m, base, base + 1, base + 2);
}

static void build_chunk(const p06_state *s, int cx, int cz, cadv_mesh *m)
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

static void refresh_terrain(p06_state *s)
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

void cadv_p06_gpu_init(void *state)
{
    p06_state *s = (p06_state *)state;
    p06_gpu *g = &s->g;
    cadv_mesh m;
    cadv_voxels v;
    cadv_rgba white = cadv_rgba_make(255, 255, 255, 255);
    cadv_rgba red = cadv_rgba_make(200, 40, 35, 255);
    cadv_rgba dark = cadv_rgba_make(60, 60, 68, 255);
    cadv_rgba steel = cadv_rgba_make(150, 155, 165, 255);
    float size = cadv_terrain_size(&s->terrain);
    int x;
    int z;
    memset(g, 0, sizeof *g);
    g->tex_snow = make_tex(CADV_TEX_NOISE, 61u, cadv_rgba_make(235, 238, 245, 255), cadv_rgba_make(250, 252, 255, 255));
    g->tex_water = make_tex(CADV_TEX_ICE, 62u, cadv_rgba_make(90, 120, 150, 255), cadv_rgba_make(150, 180, 205, 255));
    g->tex_mark = stencil_tex(s->mark_text[1], &g->mark_w);
    cadv_mesh_init(&m);
    cadv_mesh_quad(&m, cadv_v3(0, P06_SEA, size), cadv_v3(size, P06_SEA, size), cadv_v3(size, P06_SEA, 0), cadv_v3(0, P06_SEA, 0), 40.0f, 40.0f, white);
    upload(&g->water, &m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, white, false);
    upload(&g->cube, &m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f, white);
    upload(&g->plaque, &m);
    /* walker parts, unit sized around their pivots; tinted per machine */
    box(&m, -1.4f, -0.6f, -1.0f, 1.4f, 0.6f, 1.0f, white); /* hip */
    upload(&g->walker_part[0], &m);
    box(&m, -1.8f, 0.0f, -1.5f, 1.8f, 2.6f, 1.6f, white); /* torso */
    box(&m, -1.0f, 1.2f, -1.7f, 1.0f, 2.2f, -1.5f, cadv_rgba_make(255, 220, 140, 255)); /* canopy glow */
    upload(&g->walker_part[1], &m);
    box(&m, -0.45f, -2.4f, -0.5f, 0.45f, 0.3f, 0.5f, white); /* arm */
    box(&m, -0.25f, -2.6f, -1.6f, 0.25f, -1.9f, -0.4f, dark);   /* barrel */
    upload(&g->walker_part[2], &m);
    box(&m, -0.5f, -4.4f, -0.6f, 0.5f, 0.0f, 0.6f, white); /* leg */
    box(&m, -0.7f, -4.6f, -1.0f, 0.7f, -4.2f, 0.9f, dark);  /* foot */
    upload(&g->walker_part[3], &m);
    box(&m, -0.6f, 0.0f, -0.7f, 0.6f, 0.8f, 0.6f, white); /* head */
    upload(&g->walker_part[4], &m);
    box(&m, -0.35f, -0.5f, 0.0f, 0.35f, 1.0f, 0.7f, dark); /* jump jet pod */
    upload(&g->walker_part[5], &m);
    /* Sleighbell: a red-and-white supply crawler with an antenna "beard" */
    box(&m, -3.0f, 0.0f, -6.0f, 3.0f, 1.2f, 6.0f, dark);   /* tracks */
    box(&m, -2.6f, 1.2f, -5.5f, 2.6f, 3.6f, 4.0f, red);    /* body */
    box(&m, -2.7f, 3.0f, -5.6f, 2.7f, 3.4f, 4.1f, white);  /* white trim */
    box(&m, -2.2f, 1.2f, -7.4f, 2.2f, 3.4f, -5.5f, white); /* cab */
    box(&m, -1.8f, 2.4f, -7.5f, 1.8f, 3.1f, -7.3f, cadv_rgba_make(255, 230, 150, 255));
    for (x = 0; x < 7; ++x) {
        float ax = -1.5f + (float)x * 0.5f;
        box(&m, ax - 0.06f, 0.6f - (float)(x % 2) * 0.4f, -7.6f, ax + 0.06f, 2.0f, -7.45f, white); /* the beard */
    }
    box(&m, -2.0f, 3.6f, 0.0f, 2.0f, 4.0f, 4.0f, red); /* cargo bay lid */
    upload(&g->crawler, &m);
    /* pump tower, gun turret, hover tank, canister, vent mouth, cockpit strut */
    cadv_mesh_prism(&m, cadv_v3(0, 0, 0), 3.5f, 10.0f, 8, 0.0f, steel);
    box(&m, -4.5f, 0.0f, -1.0f, 4.5f, 2.0f, 1.0f, dark);
    cadv_mesh_prism(&m, cadv_v3(0, 10.0f, 0), 1.2f, 3.0f, 6, 0.0f, red);
    upload(&g->pump, &m);
    box(&m, -2.0f, 0.0f, -2.0f, 2.0f, 2.0f, 2.0f, steel);
    box(&m, -1.4f, 2.0f, -1.4f, 1.4f, 3.2f, 1.4f, dark);
    box(&m, -0.25f, 2.4f, -4.2f, 0.25f, 2.9f, -1.0f, dark);
    upload(&g->turret, &m);
    box(&m, -2.2f, 0.4f, -3.0f, 2.2f, 1.6f, 3.0f, steel);
    box(&m, -1.4f, 1.6f, -1.6f, 1.4f, 2.6f, 1.2f, dark);
    box(&m, -1.2f, 2.0f, -3.4f, -0.6f, 2.4f, -1.0f, dark);
    box(&m, 0.6f, 2.0f, -3.4f, 1.2f, 2.4f, -1.0f, dark);
    upload(&g->hover, &m);
    cadv_mesh_prism(&m, cadv_v3(0, 0, 0), 0.8f, 2.2f, 8, 0.0f, cadv_rgba_make(80, 200, 255, 255));
    box(&m, -0.9f, 2.2f, -0.2f, 0.9f, 2.4f, 0.2f, white);
    upload(&g->canister, &m);
    cadv_mesh_prism(&m, cadv_v3(0, -0.5f, 0), 2.2f, 1.0f, 8, 0.0f, dark);
    upload(&g->vent, &m);
    box(&m, -0.05f, -0.05f, -0.05f, 0.05f, 0.05f, 0.05f, dark);
    upload(&g->cockpit, &m);
    /* a wreck: a toppled torso and a broken leg, dark and scorched */
    box(&m, -1.8f, 0.0f, -1.5f, 1.8f, 2.6f, 1.6f, cadv_rgba_make(55, 55, 60, 255));
    box(&m, 2.0f, 0.0f, -0.5f, 6.4f, 1.0f, 0.5f, cadv_rgba_make(45, 45, 50, 255));
    box(&m, -2.6f, 0.0f, 1.0f, -1.6f, 0.8f, 4.0f, cadv_rgba_make(70, 60, 55, 255));
    upload(&g->wreck, &m);
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
            float k = cadv_cosf(5.0f * a);
            if (rr <= 3.4f + 3.6f * (k > 0.0f ? k * k : 0.0f)) {
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

void cadv_p06_gpu_free(void *state)
{
    p06_state *s = (p06_state *)state;
    p06_gpu *g = &s->g;
    int i;
    for (i = 0; i < CADV_TERRAIN_MAX_CHUNKS; ++i) {
        cadv_gpu_mesh_free(&g->chunk[i]);
        g->chunk_seen[i] = 0;
    }
    cadv_gpu_mesh_free(&g->water);
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->crawler);
    cadv_gpu_mesh_free(&g->pump);
    cadv_gpu_mesh_free(&g->turret);
    cadv_gpu_mesh_free(&g->hover);
    cadv_gpu_mesh_free(&g->canister);
    cadv_gpu_mesh_free(&g->vent);
    cadv_gpu_mesh_free(&g->cockpit);
    cadv_gpu_mesh_free(&g->wreck);
    for (i = 0; i < 6; ++i) {
        cadv_gpu_mesh_free(&g->walker_part[i]);
    }
    cadv_gl_delete_textures(1, &g->tex_snow);
    cadv_gl_delete_textures(1, &g->tex_water);
    cadv_gl_delete_textures(1, &g->tex_mark);
    s->gpu_ready = false;
}

/* ---- draw ------------------------------------------------------------------------------ */

static cadv_mat4 segment(cadv_vec3 a, cadv_vec3 b, float thick)
{
    cadv_vec3 d = cadv_v3_sub(b, a);
    float len = cadv_v3_len(d);
    float yaw = cadv_atan2f(d.x, d.z);
    float pitch = -cadv_atan2f(d.y, cadv_sqrtf(d.x * d.x + d.z * d.z));
    return cadv_m4_mul(cadv_m4_translate(cadv_v3_scale(cadv_v3_add(a, b), 0.5f)),
                       cadv_m4_mul(cadv_m4_rotate_y(yaw), cadv_m4_mul(cadv_m4_rotate_x(pitch), cadv_m4_scale(cadv_v3(thick, thick, len > 0.01f ? len : 0.01f)))));
}

static void part(cadv_scene *sc, const cadv_gpu_mesh *mesh, cadv_mat4 m, cadv_vec4 tint)
{
    cadv_scene_draw(sc, mesh, m, cadv_material_make(0, tint, 0.05f, 0));
}

/* A walker from parts: legs swing with the stride, the torso turns on the hips. */
static void draw_walker(const p06_state *s, cadv_scene *sc, const cadv_walker *w, float height, cadv_vec4 tint, bool jets_big)
{
    const p06_gpu *g = &s->g;
    float k = height / 9.0f; /* parts are sized for a 9 m machine */
    float swing = cadv_sinf(w->stride) * 0.5f * cadv_minf(1.0f, cadv_absf(w->speed) / 4.0f);
    cadv_mat4 root = cadv_m4_mul(cadv_m4_translate(w->pos), cadv_m4_scale(cadv_v3(k, k, k)));
    cadv_mat4 legs = cadv_m4_mul(root, cadv_m4_rotate_y(w->legs_yaw));
    cadv_mat4 hip = cadv_m4_mul(legs, cadv_m4_translate(cadv_v3(0, 4.8f, 0)));
    cadv_mat4 torso = cadv_m4_mul(cadv_m4_mul(root, cadv_m4_translate(cadv_v3(0, 5.2f, 0))), cadv_m4_rotate_y(w->twist + w->legs_yaw));
    cadv_vec4 broken = cadv_v4(0.25f, 0.22f, 0.2f, 1);
    part(sc, &g->walker_part[0], hip, tint);
    part(sc, &g->walker_part[3], cadv_m4_mul(cadv_m4_mul(hip, cadv_m4_translate(cadv_v3(-1.0f, -0.2f, 0))), cadv_m4_rotate_x(swing)),
         cadv_walker_has(w, CADV_WZ_LL) ? tint : broken);
    part(sc, &g->walker_part[3], cadv_m4_mul(cadv_m4_mul(hip, cadv_m4_translate(cadv_v3(1.0f, -0.2f, 0))), cadv_m4_rotate_x(-swing)),
         cadv_walker_has(w, CADV_WZ_RL) ? tint : broken);
    part(sc, &g->walker_part[1], torso, tint);
    if (cadv_walker_has(w, CADV_WZ_LA)) {
        part(sc, &g->walker_part[2], cadv_m4_mul(torso, cadv_m4_mul(cadv_m4_translate(cadv_v3(-2.3f, 2.2f, 0)), cadv_m4_rotate_x(w->pitch))), tint);
    }
    if (cadv_walker_has(w, CADV_WZ_RA)) {
        part(sc, &g->walker_part[2], cadv_m4_mul(torso, cadv_m4_mul(cadv_m4_translate(cadv_v3(2.3f, 2.2f, 0)), cadv_m4_rotate_x(w->pitch))), tint);
    }
    part(sc, &g->walker_part[4], cadv_m4_mul(torso, cadv_m4_translate(cadv_v3(0, 2.6f, -0.2f))), tint);
    {
        float js = jets_big ? 1.6f : 1.0f; /* Foen-X's oversized jump jets */
        cadv_mat4 jl = cadv_m4_mul(torso, cadv_m4_mul(cadv_m4_translate(cadv_v3(-1.2f, 1.0f, 1.6f)), cadv_m4_scale(cadv_v3(js, js, js))));
        cadv_mat4 jr = cadv_m4_mul(torso, cadv_m4_mul(cadv_m4_translate(cadv_v3(1.2f, 1.0f, 1.6f)), cadv_m4_scale(cadv_v3(js, js, js))));
        part(sc, &g->walker_part[5], jl, tint);
        part(sc, &g->walker_part[5], jr, tint);
    }
    if (w->jetting || jets_big) {
        cadv_vec3 p = cadv_v3(w->pos.x, w->pos.y + 5.0f * k, w->pos.z);
        float f = w->jetting ? 1.6f : 0.35f; /* hot air, always a little */
        cadv_scene_light(sc, p, cadv_v3(f, 0.6f * f, 0.25f * f), 10.0f, false);
    }
}

static cadv_vec4 team_tint(const p06_state *s, const p06_unit *u)
{
    float h = u->hurt > 0 ? 1.6f : 1.0f;
    if (u->kind == P06_K_KRAMPUS) {
        return cadv_v4(0.55f * h, 0.15f * h, 0.12f * h, 1); /* the ace's blood red */
    }
    if (u->team == P06_TEAM_GRAIL) {
        return cadv_v4(0.75f * h, 0.62f * h, 0.3f * h, 1); /* corporate ochre */
    }
    (void)s;
    return cadv_v4(0.7f * h, 0.78f * h, 0.9f * h, 1);
}

static void draw_world(p06_state *s, cadv_scene *sc, const cadv_camera *cam, float t)
{
    p06_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    int chunks = cadv_terrain_chunks_per_side(&s->terrain);
    int lights = 0;
    int i;
    for (i = 0; i < chunks * chunks; ++i) {
        cadv_scene_draw(sc, &g->chunk[i], cadv_m4_identity(), cadv_material_make(g->tex_snow, white, 0.0f, 0));
    }
    cadv_scene_draw(sc, &g->water, cadv_m4_identity(), cadv_material_make(g->tex_water, white, 0.1f, CADV_MAT_NO_SHADOW));
    if (s->mission == P06_M3) {
        for (i = 0; i < P06_VENTS; ++i) {
            cadv_vec3 v = s->vents[i];
            int k;
            cadv_scene_draw(sc, &g->vent, cadv_m4_translate(v), cadv_material_make(0, white, 0.0f, 0));
            for (k = 0; k < 5; ++k) {
                /* steam puffs rising and drifting */
                float ph = (t * 0.6f + (float)k * 0.2f + (float)i * 0.37f);
                float f = ph - (float)(int)ph;
                float sz = 1.0f + f * 3.0f;
                cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(cadv_v3(v.x + f * 2.0f, v.y + 1.0f + f * 14.0f, v.z)), cadv_m4_scale(cadv_v3(sz, sz, sz))),
                                cadv_material_make(0, cadv_v4(0.9f, 0.92f, 0.95f, 1), 0.4f, CADV_MAT_NO_SHADOW));
            }
            if (lights < 6) {
                cadv_scene_light(sc, cadv_v3(v.x, v.y + 3.0f, v.z), cadv_v3(0.5f, 0.6f, 0.7f), 14.0f, false);
                ++lights;
            }
        }
    }
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        const p06_unit *u = &s->u[i];
        cadv_vec3 p;
        cadv_vec4 tint;
        if (!u->active) {
            continue;
        }
        p = cadv_p06_unit_pos(s, i);
        if ((p.x - cam->pos.x) * (p.x - cam->pos.x) + (p.z - cam->pos.z) * (p.z - cam->pos.z) > 250.0f * 250.0f) {
            continue; /* lost in the fog banks */
        }
        tint = team_tint(s, u);
        if (u->kind == P06_K_WRECK || u->dead) {
            cadv_scene_draw(sc, &g->wreck, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(u->yaw)), cadv_material_make(0, cadv_v4(0.6f, 0.6f, 0.6f, 1), 0.0f, 0));
            if (u->kind == P06_K_WRECK) {
                cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(p.x, p.y + 0.9f, p.z)),
                                                            cadv_m4_mul(cadv_m4_rotate_y(u->yaw), cadv_m4_mul(cadv_m4_translate(cadv_v3(0, 0, 1.65f)), cadv_m4_scale(cadv_v3((float)g->mark_w / 24.0f, 0.5f, 1))))),
                                cadv_material_make(g->tex_mark, white, 0.3f, CADV_MAT_NO_SHADOW));
            } else if (lights < 20 && s->ticks % 20 < 14) {
                cadv_scene_light(sc, cadv_v3(p.x, p.y + 2.0f, p.z), cadv_v3(1.2f, 0.5f, 0.2f), 8.0f, false); /* burning */
                ++lights;
            }
            continue;
        }
        switch (u->kind) {
        case P06_K_JACKAL:
        case P06_K_KRAMPUS:
        case P06_K_WINGMAN:
            draw_walker(s, sc, &u->w, cadv_p06_walker_def(u->kind)->height, tint, u->kind == P06_K_WINGMAN && i == s->wingmen[1]);
            break;
        case P06_K_CRAWLER:
            cadv_scene_draw(sc, &g->crawler, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(u->yaw)), cadv_material_make(0, white, 0.1f, 0));
            if (lights < 20) {
                cadv_scene_light(sc, cadv_v3(p.x - cadv_sinf(u->yaw) * 9.0f, p.y + 3.0f, p.z - cadv_cosf(u->yaw) * 9.0f), cadv_v3(1.4f, 1.3f, 1.0f), 22.0f, false);
                ++lights;
            }
            break;
        case P06_K_PUMP:
            cadv_scene_draw(sc, &g->pump, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(t * 0.3f)), cadv_material_make(0, tint, 0.1f, 0));
            break;
        case P06_K_TURRET:
            cadv_scene_draw(sc, &g->turret, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(u->yaw)), cadv_material_make(0, tint, 0.05f, 0));
            break;
        default:
            cadv_scene_draw(sc, &g->hover, cadv_m4_mul(cadv_m4_translate(cadv_v3(p.x, p.y + 0.4f + 0.2f * cadv_sinf(t * 3.0f + (float)i), p.z)), cadv_m4_rotate_y(u->yaw)),
                            cadv_material_make(0, tint, 0.05f, 0));
            break;
        }
    }
    /* beams light the snow; missiles leave trails */
    for (i = 0; i < P06_MAX_BEAMS; ++i) {
        const p06_beam *b = &s->beam[i];
        if (b->ttl > 0) {
            bool ppc = b->kind == P06_W_PPC;
            cadv_vec3 c = ppc ? cadv_v3(0.7f, 0.9f, 2.0f) : cadv_v3(2.0f, 0.4f, 0.3f);
            cadv_scene_draw(sc, &g->cube, segment(b->a, b->b, ppc ? 0.45f : 0.18f), cadv_material_make(0, cadv_v4(c.x, c.y, c.z, 1), 2.0f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            if (lights < 28) {
                cadv_scene_light(sc, b->b, c, ppc ? 18.0f : 9.0f, false);
                ++lights;
            }
        }
    }
    for (i = 0; i < P06_MAX_PROJ; ++i) {
        const p06_proj *p = &s->proj[i];
        if (p->active) {
            cadv_vec3 back = cadv_v3_sub(p->pos, cadv_v3_scale(p->vel, p->kind == P06_PROJ_MISSILE ? 0.08f : 0.03f));
            cadv_vec4 col = p->kind == P06_PROJ_MISSILE ? cadv_v4(1.6f, 1.2f, 0.8f, 1) : cadv_v4(1.8f, 1.5f, 0.6f, 1);
            cadv_scene_draw(sc, &g->cube, segment(back, p->pos, p->kind == P06_PROJ_MISSILE ? 0.3f : 0.15f), cadv_material_make(0, col, 1.5f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
        }
    }
    /* the coolant canister on its way down, then waiting in the snow */
    if (s->coolant_timer > 0 || s->coolant_down) {
        float y = s->coolant_pos.y + (s->coolant_down ? 0.0f : (float)s->coolant_timer * 0.6f);
        cadv_scene_draw(sc, &g->canister, cadv_m4_translate(cadv_v3(s->coolant_pos.x, y, s->coolant_pos.z)), cadv_material_make(0, white, 0.8f, 0));
        cadv_scene_light(sc, cadv_v3(s->coolant_pos.x, y + 2.0f, s->coolant_pos.z), cadv_v3(0.3f, 0.9f, 1.6f), 10.0f, false);
    }
    if (s->star_active) {
        cadv_vec3 p = cadv_v3(s->star_pos.x, s->star_pos.y + 0.4f * cadv_sinf(t * 2.0f), s->star_pos.z);
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(t * 1.5f)), cadv_material_make(0, white, 1.2f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, p, cadv_v3(2.0f, 1.7f, 0.6f), 16.0f, false);
    }
}

/* ---- the cockpit ------------------------------------------------------------------------ */

static cadv_vec4 zone_colour(const cadv_walker *w, const cadv_walker_def *d, cadv_walker_zone z)
{
    float f = (float)w->hp[z] / (float)d->armour[z];
    if (w->hp[z] <= 0) {
        return cadv_v4(0.15f, 0.15f, 0.15f, 1);
    }
    return f > 0.66f ? cadv_v4(0.3f, 0.9f, 0.4f, 1) : f > 0.33f ? cadv_v4(1.0f, 0.85f, 0.2f, 1) : cadv_v4(1.0f, 0.3f, 0.2f, 1);
}

static void damage_silhouette(cadv_ui *ui, const cadv_walker *w, const cadv_walker_def *d, float x, float y)
{
    cadv_ui_rect(ui, x + 7, y, 6, 4, zone_colour(w, d, CADV_WZ_HEAD));
    cadv_ui_rect(ui, x + 6, y + 5, 8, 11, zone_colour(w, d, CADV_WZ_CT));
    cadv_ui_rect(ui, x + 1, y + 5, 4, 10, zone_colour(w, d, CADV_WZ_LT));
    cadv_ui_rect(ui, x + 15, y + 5, 4, 10, zone_colour(w, d, CADV_WZ_RT));
    cadv_ui_rect(ui, x - 3, y + 6, 3, 12, zone_colour(w, d, CADV_WZ_LA));
    cadv_ui_rect(ui, x + 20, y + 6, 3, 12, zone_colour(w, d, CADV_WZ_RA));
    cadv_ui_rect(ui, x + 4, y + 17, 4, 12, zone_colour(w, d, CADV_WZ_LL));
    cadv_ui_rect(ui, x + 12, y + 17, 4, 12, zone_colour(w, d, CADV_WZ_RL));
}

static void radar(const p06_state *s, cadv_ui *ui, float cx, float cy, float r, float range)
{
    float ty = cadv_walker_torso_yaw(&s->mech);
    int i;
    cadv_vec4 green = cadv_v4(0.3f, 1.0f, 0.5f, 1);
    cadv_ui_rect(ui, cx - r, cy - r, r * 2, r * 2, cadv_v4(0.02f, 0.08f, 0.04f, 0.8f));
    cadv_ui_rect(ui, cx - 1, cy - 1, 2, 2, green);
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        const p06_unit *u = &s->u[i];
        cadv_vec3 p;
        float dx;
        float dz;
        float fx;
        float fz;
        if (!u->active || u->dead || u->kind == P06_K_WRECK) {
            continue;
        }
        /* Santa Claude relays every contact during mission 2 */
        p = cadv_p06_unit_pos(s, i);
        dx = p.x - s->mech.pos.x;
        dz = p.z - s->mech.pos.z;
        if (dx * dx + dz * dz > range * range && !(s->mission == P06_M2 && u->team == P06_TEAM_GRAIL)) {
            continue;
        }
        /* rotate into the torso's frame: up on the screen is where it faces */
        fx = dx * cadv_cosf(ty) - dz * cadv_sinf(ty);
        fz = dx * cadv_sinf(ty) + dz * cadv_cosf(ty);
        fx = cadv_clampf(fx / range * r, -r + 1, r - 2);
        fz = cadv_clampf(fz / range * r, -r + 1, r - 2);
        cadv_ui_rect(ui, cx + fx - 1, cy + fz - 1, 2, 2,
                     u->kind == P06_K_CRAWLER ? cadv_v4(1, 0.3f, 0.3f, 1) : u->team == P06_TEAM_US ? cadv_v4(0.4f, 0.7f, 1, 1)
                     : i == s->lock ? cadv_v4(1, 1, 0.3f, 1) : cadv_v4(1, 0.5f, 0.2f, 1));
    }
}

static void draw_lab(const p06_state *s, cadv_ui *ui)
{
    static const char *const mount[P06_SLOTS] = {"LEFT ARM", "RIGHT ARM", "LEFT TORSO", "RIGHT TORSO"};
    float cx = (float)ui->width * 0.5f;
    float y = 30.0f;
    char buf[96];
    int i;
    cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.02f, 0.05f, 0.04f, 0.75f));
    cadv_ui_text_centered(ui, cx, 10, 2, cadv_v4(0.4f, 1.0f, 0.6f, 1), "MECH LAB");
    for (i = 0; i < P06_SLOTS; ++i) {
        const p06_weapon_def *w = &cadv_p06_weapons()[s->loadout[i]];
        bool sel = i == s->lab_slot;
        cadv_ui_rect(ui, cx - 110, y - 1, 220, 11, sel ? cadv_v4(0.1f, 0.4f, 0.2f, 1) : cadv_v4(0.05f, 0.15f, 0.1f, 1));
        snprintf(buf, sizeof buf, "%d %-11s %-10s %dT", i + 1, mount[i], w->name, w->tons);
        cadv_ui_text(ui, cx - 106, y + 1, 1, sel ? cadv_v4(1, 1, 0.6f, 1) : cadv_v4(0.6f, 0.9f, 0.7f, 1), buf);
        y += 13.0f;
    }
    snprintf(buf, sizeof buf, "TONNAGE %d / %d", cadv_p06_tonnage(s), P06_TONNAGE);
    cadv_ui_text_centered(ui, cx, y + 4, 1, cadv_v4(0.6f, 0.9f, 0.7f, 1), buf);
    snprintf(buf, sizeof buf, "PAINT: %s", cadv_p06_paint_name(s, s->paint));
    cadv_ui_text_centered(ui, cx, y + 16, 1, s->paint == 2 ? cadv_v4(0.9f, 0.9f, 0.9f, 1) : cadv_v4(0.6f, 0.9f, 0.7f, 1), buf);
    cadv_ui_text_centered(ui, cx, (float)ui->height - 30, 1, cadv_v4(0.9f, 0.9f, 0.6f, 1), "1-4 HARDPOINT  Q/WHEEL SWAP  R-MOUSE PAINT");
    cadv_ui_text_centered(ui, cx, (float)ui->height - 18, 1, cadv_v4(1, 1, 1, 1), "E / FIRE: LAUNCH");
}

static void draw_hud(const p06_state *s, cadv_ui *ui)
{
    const cadv_walker *w = &s->mech;
    const cadv_walker_def *d = cadv_p06_walker_def(P06_K_WINGMAN);
    cadv_vec4 green = cadv_v4(0.35f, 1.0f, 0.55f, 1);
    cadv_vec4 dim = cadv_v4(0.2f, 0.5f, 0.3f, 1);
    float W = (float)ui->width;
    float H = (float)ui->height;
    float cx = W * 0.5f;
    float cy = H * 0.5f;
    float panel = H - 44.0f;
    char buf[96];
    int i;
    if (s->mission == P06_LAB) {
        draw_lab(s, ui);
        return;
    }
    /* crosshair; brackets when the missiles are locked */
    cadv_ui_rect(ui, cx - 6, cy, 4, 1, green);
    cadv_ui_rect(ui, cx + 3, cy, 4, 1, green);
    cadv_ui_rect(ui, cx, cy - 6, 1, 4, green);
    cadv_ui_rect(ui, cx, cy + 3, 1, 4, green);
    if (s->lock >= 0) {
        cadv_ui_rect(ui, cx - 10, cy - 10, 20, 1, cadv_v4(1, 1, 0.3f, 1));
        cadv_ui_rect(ui, cx - 10, cy + 9, 20, 1, cadv_v4(1, 1, 0.3f, 1));
        cadv_ui_text(ui, cx + 12, cy - 4, 1, cadv_v4(1, 1, 0.3f, 1), "LOCK");
    }
    /* compass strip: torso heading, a tick for the legs */
    {
        float ty = cadv_walker_torso_yaw(w);
        int deg = (int)(cadv_wrap_angle(-ty) * 180.0f / CADV_PI + 360.0f) % 360;
        float legs = cadv_wrap_angle(w->legs_yaw - ty);
        cadv_ui_rect(ui, cx - 50, 4, 100, 9, cadv_v4(0.02f, 0.08f, 0.04f, 0.8f));
        snprintf(buf, sizeof buf, "%03d", deg);
        cadv_ui_text_centered(ui, cx, 5, 1, green, buf);
        cadv_ui_rect(ui, cx - cadv_clampf(legs, -1.6f, 1.6f) * 30.0f - 1, 13, 3, 3, cadv_v4(1, 0.8f, 0.3f, 1));
    }
    /* the lower console: heat, throttle, damage, weapons, radar */
    cadv_ui_rect(ui, 0, panel, W, 44, cadv_v4(0.05f, 0.06f, 0.07f, 0.92f));
    cadv_ui_rect(ui, 0, panel, W, 1, cadv_v4(0.3f, 0.35f, 0.4f, 1));
    /* heat gauge */
    {
        float f = cadv_clampf(w->heat / d->heat_cap, 0.0f, 1.0f);
        cadv_vec4 hc = f > 0.8f ? cadv_v4(1, 0.25f, 0.2f, 1) : f > 0.5f ? cadv_v4(1, 0.8f, 0.2f, 1) : green;
        cadv_ui_rect(ui, 4, panel + 4, 8, 36, cadv_v4(0.1f, 0.1f, 0.1f, 1));
        cadv_ui_rect(ui, 4, panel + 4 + 36.0f * (1.0f - f), 8, 36.0f * f, hc);
        cadv_ui_text(ui, 14, panel + 4, 1, hc, "HEAT");
        if (w->shutdown || (f > 0.8f && (s->ticks / 10) % 2)) {
            cadv_ui_text_centered(ui, cx, cy - 30, 1, cadv_v4(1, 0.3f, 0.2f, 1), w->shutdown ? "SHUTDOWN" : "HEAT WARNING");
        }
    }
    /* throttle lever */
    {
        float y0 = panel + 22.0f;
        cadv_ui_rect(ui, 16, panel + 14, 6, 26, cadv_v4(0.1f, 0.1f, 0.1f, 1));
        cadv_ui_rect(ui, 16, y0, 6, 1, dim);
        cadv_ui_rect(ui, 14, y0 - w->throttle * 12.0f - 1, 10, 3, cadv_v4(0.9f, 0.9f, 0.9f, 1));
        snprintf(buf, sizeof buf, "%3.0f KPH", w->speed * 3.6f);
        cadv_ui_text(ui, 26, panel + 32, 1, green, buf);
    }
    damage_silhouette(ui, w, d, 70, panel + 8);
    /* weapons: group, name, ready / ammo */
    for (i = 0; i < P06_SLOTS; ++i) {
        const p06_weapon_def *wd = &cadv_p06_weapons()[s->loadout[i]];
        static const cadv_walker_zone mount[P06_SLOTS] = {CADV_WZ_LA, CADV_WZ_RA, CADV_WZ_LT, CADV_WZ_RT};
        bool live = cadv_walker_has(w, mount[i]);
        bool ready = live && s->cd[i] <= 0.0f && (wd->ammo == 0 || s->ammo[i] > 0);
        cadv_vec4 col = !live ? cadv_v4(0.4f, 0.15f, 0.15f, 1) : wd->group == s->group ? green : dim;
        if (wd->ammo > 0) {
            snprintf(buf, sizeof buf, "%d %-10s %3d", wd->group + 1, wd->name, s->ammo[i]);
        } else {
            snprintf(buf, sizeof buf, "%d %-10s", wd->group + 1, wd->name);
        }
        cadv_ui_text(ui, 102, panel + 4 + (float)i * 9.0f, 1, col, buf);
        cadv_ui_rect(ui, 98, panel + 5 + (float)i * 9.0f, 3, 5, ready ? green : cadv_v4(0.3f, 0.3f, 0.3f, 1));
    }
    if (s->chain) {
        cadv_ui_text(ui, 102, panel - 9, 1, dim, "CHAIN FIRE");
    }
    radar(s, ui, W - 24, panel + 22, 19, 300.0f);
    /* objective, coolant, radio subtitles */
    {
        const char *goal = s->mission == P06_M1 ? (s->star_active ? "COLLECT THE GIFT AT SLEIGHBELL" : "ESCORT SLEIGHBELL TO THE DEPOT")
                           : s->mission == P06_M2 ? "DESTROY THE DAM PUMPS"
                           : s->mission == P06_M3 ? "DEFEAT KRAMPUS"
                                                  : "CAMPAIGN COMPLETE";
        cadv_ui_text(ui, 4, 18, 1, green, goal);
        if (s->mission == P06_M1 && s->crawler >= 0) {
            const p06_unit *c = &s->u[s->crawler];
            snprintf(buf, sizeof buf, "SLEIGHBELL %d%%", c->hp * 100 / (c->hp_max > 0 ? c->hp_max : 1));
            cadv_ui_text(ui, 4, 27, 1, cadv_v4(1, 0.5f, 0.45f, 1), buf);
        } else if (s->mission == P06_M2) {
            snprintf(buf, sizeof buf, "PUMPS LEFT %d", s->pumps_left);
            cadv_ui_text(ui, 4, 27, 1, cadv_v4(1, 0.7f, 0.4f, 1), buf);
        }
        snprintf(buf, sizeof buf, "COOLANT %d (E)", s->coolant_left);
        cadv_ui_text(ui, W - 92, 18, 1, s->coolant_left > 0 ? cadv_v4(0.4f, 0.85f, 1, 1) : dim, buf);
    }
    if (s->radio_timer > 0) {
        /* subtitles, wrapped over up to three lines */
        char lines[3][64];
        int n = cadv_help_wrap(s->radio, (int)(W / 6.0f) - 2, lines, 3);
        cadv_ui_rect(ui, 0, panel - 4 - 9.0f * (float)n - 2, W, 9.0f * (float)n + 2, cadv_v4(0, 0, 0, 0.5f));
        for (i = 0; i < n; ++i) {
            cadv_ui_text_centered(ui, cx, panel - 4 - 9.0f * (float)(n - i), 1, cadv_v4(1, 0.85f, 0.75f, 1), lines[i]);
        }
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx, cy + 20, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->intro > 0) {
        const char *title = s->mission == P06_M1 ? "MISSION 1: THE SLEIGHBELL RUN" : s->mission == P06_M2 ? "MISSION 2: THE ICE DAM" : "MISSION 3: KRAMPUS";
        cadv_ui_rect(ui, 0, cy - 50, W, 16, cadv_v4(0, 0, 0, 0.55f));
        cadv_ui_text_centered(ui, cx, cy - 46, 1, cadv_v4(1, 0.9f, 0.6f, 1), title);
    }
    if (s->map_open) {
        radar(s, ui, cx, cy, 70, 520.0f);
    }
    if (s->hurt_flash > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.8f, 0.2f, 0.1f, 0.03f * (float)s->hurt_flash));
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, W, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(1, 0.8f, 0.4f, 1), "WINTERHOLD IS SAFE!");
        cadv_ui_text_centered(ui, cx, cy + 4, 1, cadv_v4(1, 1, 1, 1), "PRESS USE TO RETURN TO THE HALL");
    }
    if (s->lose_timer > 0) {
        cadv_ui_rect(ui, 0, 0, W, H, cadv_v4(0.5f, 0.1f, 0.1f, 0.35f));
    }
}

void cadv_p06_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p06_state *s = (p06_state *)state;
    const cadv_walker *w = &s->mech;
    float t = ((float)s->ticks + alpha) * P06_DT;
    float bob = cadv_absf(cadv_sinf(w->stride)) * 0.18f * cadv_minf(1.0f, cadv_absf(w->speed) / 6.0f);
    float shake = s->shake > 0 ? 0.08f * cadv_sinf((float)s->ticks * 2.3f) * (float)s->shake / 12.0f : 0.0f;
    refresh_terrain(s);
    cam->pos = cadv_p06_eye(s);
    cam->pos.y += bob - 0.1f;
    cam->yaw = cadv_walker_torso_yaw(w) + shake;
    cam->pitch = w->pitch + shake * 0.5f;
    cam->fovy = 1.1f;
    cam->znear = 0.2f;
    cam->zfar = 320.0f;
    /* dusk for the convoy, a grey storm at the dam, a pale crater day */
    if (s->mission == P06_M2) {
        sc->ambient = cadv_v3(0.22f, 0.24f, 0.32f);
        sc->sun_color = cadv_v3(0.5f, 0.55f, 0.7f);
        sc->clear_color = cadv_v3(0.2f, 0.22f, 0.3f);
        sc->fog_color = cadv_v3(0.24f, 0.26f, 0.33f);
    } else if (s->mission == P06_M3 || s->mission == P06_DONE) {
        sc->ambient = cadv_v3(0.36f, 0.36f, 0.42f);
        sc->sun_color = cadv_v3(0.95f, 0.9f, 0.85f);
        sc->clear_color = cadv_v3(0.55f, 0.6f, 0.7f);
        sc->fog_color = cadv_v3(0.62f, 0.65f, 0.72f);
    } else {
        sc->ambient = cadv_v3(0.28f, 0.26f, 0.34f);
        sc->sun_color = cadv_v3(1.0f, 0.62f, 0.45f);
        sc->clear_color = cadv_v3(0.35f, 0.28f, 0.38f);
        sc->fog_color = cadv_v3(0.42f, 0.36f, 0.44f);
    }
    sc->sun_dir = cadv_v3_norm(cadv_v3(-0.5f, -0.45f, -0.6f));
    sc->sun_shadow = true;
    sc->shadow_center = cadv_v3(cam->pos.x, 0, cam->pos.z);
    sc->shadow_extent = 70.0f;
    sc->fog_near = 90.0f;
    sc->fog_far = 300.0f;
    sc->light_bands = 5;
    sc->dither = 0.5f;
    sc->use_palette = true;
    draw_world(s, sc, cam, t);
    /* shoulder searchlight throwing shadows ahead */
    if (s->mission != P06_LAB && !w->shutdown) {
        cadv_vec3 f = cadv_walker_aim(w);
        cadv_vec3 p = cadv_v3_add(cam->pos, cadv_v3_scale(f, 14.0f));
        cadv_scene_light(sc, cadv_v3(p.x, cadv_maxf(p.y, cadv_terrain_height(&s->terrain, p.x, p.z) + 4.0f), p.z), cadv_v3(1.1f, 1.1f, 1.0f), 26.0f, true);
    }
    /* cockpit struts and the canopy frame, fixed to the view */
    if (s->mission != P06_LAB) {
        cadv_mat4 view = cadv_m4_mul(cadv_m4_translate(cam->pos), cadv_m4_mul(cadv_m4_rotate_y(cam->yaw), cadv_m4_rotate_x(cam->pitch)));
        cadv_vec4 frame = cadv_v4(0.16f, 0.17f, 0.2f, 1);
        uint32_t f = CADV_MAT_NO_SHADOW;
        cadv_scene_draw(sc, &s->g.cube, cadv_m4_mul(view, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.62f, 0.05f, -0.7f)), cadv_m4_scale(cadv_v3(0.05f, 0.9f, 0.05f)))),
                        cadv_material_make(0, frame, 0.0f, f));
        cadv_scene_draw(sc, &s->g.cube, cadv_m4_mul(view, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.62f, 0.05f, -0.7f)), cadv_m4_scale(cadv_v3(0.05f, 0.9f, 0.05f)))),
                        cadv_material_make(0, frame, 0.0f, f));
        cadv_scene_draw(sc, &s->g.cube, cadv_m4_mul(view, cadv_m4_mul(cadv_m4_translate(cadv_v3(0, 0.42f, -0.72f)), cadv_m4_scale(cadv_v3(1.4f, 0.05f, 0.05f)))),
                        cadv_material_make(0, frame, 0.0f, f));
    }
    if (ui) {
        draw_hud(s, ui);
    }
}

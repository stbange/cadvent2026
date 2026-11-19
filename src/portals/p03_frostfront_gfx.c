/* SPDX-License-Identifier: MIT */
/* GPU side of Day 3: tundra battlefield, Frostite light, voxel base and
 * units at human scale, tracers, the placement beam and the wrist tablet. */
#include "portals/p03_frostfront_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portals/p03_internal.h"

static unsigned make_tex(cadv_tex_kind kind, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, kind, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

static unsigned text_tex(const char *text, int *w_out)
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
        t.rgba[i * 4] = 30;
        t.rgba[i * 4 + 1] = 40;
        t.rgba[i * 4 + 2] = 50;
        t.rgba[i * 4 + 3] = 255;
    }
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y)) {
                    cadv_texdata_set(&t, 3 + i * 6 + x, 2 + y, 140, 255, 200, 255);
                }
            }
        }
    }
    tex = cadv_gl_texture(&t, false);
    *w_out = w;
    cadv_texdata_free(&t);
    return tex;
}

static void block(cadv_voxels *v, int x0, int y0, int z0, int x1, int y1, int z1, uint8_t c)
{
    int x;
    int y;
    int z;
    for (z = z0; z <= z1; ++z) {
        for (y = y0; y <= y1; ++y) {
            for (x = x0; x <= x1; ++x) {
                cadv_voxels_set(v, x, y, z, c);
            }
        }
    }
}

static void upload_voxels(cadv_gpu_mesh *g, cadv_voxels *v, float scale)
{
    cadv_mesh m;
    cadv_mesh_init(&m);
    cadv_voxels_mesh(v, scale, &m);
    cadv_gpu_mesh_upload(g, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(v);
}

/* ---- original structures (voxel, 0.5 m per voxel) ------------------------------------ */

static void make_building(cadv_voxels *v, int def)
{
    const cadv_rts_bdef *d = &cadv_p03_buildings()[def];
    int w = d->fw * 6;
    int dd = d->fh * 6;
    int k;
    int h = def == P03_B_SPIRE ? 18 : (def == P03_B_PYLON ? 12 : (def == P03_B_TOWER ? 9 : 7));
    cadv_voxels_alloc(v, w, h + 2, dd);
    v->pal[1] = cadv_rgba_make(200, 205, 215, 255); /* panels (tinted per side) */
    v->pal[2] = cadv_rgba_make(90, 95, 105, 255);   /* frame */
    v->pal[3] = cadv_rgba_make(255, 230, 140, 255); /* lights */
    v->pal[4] = cadv_rgba_make(120, 255, 220, 255); /* Frostite */
    v->pal[5] = cadv_rgba_make(255, 120, 60, 255);  /* ember */
    switch (def) {
    case P03_B_YARD: /* deployed mobile yard: crane and pad */
        block(v, 1, 0, 1, w - 2, 2, dd - 2, 2);
        block(v, 2, 3, 2, 8, 6, 8, 1);
        block(v, 12, 3, 12, 13, 8, 13, 2);
        block(v, 6, 8, 12, 13, 8, 13, 3);
        break;
    case P03_B_POWER: /* twin cooling stacks */
        block(v, 0, 0, 0, w - 1, 2, dd - 1, 2);
        block(v, 2, 3, 2, 4, 7, 4, 1);
        block(v, 7, 3, 7, 9, 7, 9, 1);
        cadv_voxels_set(v, 3, 8, 3, 3);
        cadv_voxels_set(v, 8, 8, 8, 3);
        break;
    case P03_B_REFINERY: /* silo, intake and a Frostite tank */
        block(v, 0, 0, 0, w - 1, 3, dd - 1, 2);
        block(v, 1, 4, 1, 7, 6, 7, 1);
        block(v, 11, 4, 2, 15, 8, 7, 4);
        block(v, 1, 4, 9, 16, 4, 10, 3);
        break;
    case P03_B_BARRACKS: /* low hut with a flag */
        block(v, 0, 0, 0, w - 1, 4, dd - 1, 1);
        block(v, 0, 5, 1, w - 1, 5, dd - 2, 2);
        block(v, 10, 6, 10, 10, 8, 10, 2);
        cadv_voxels_set(v, 10, 8, 9, 3);
        break;
    case P03_B_FACTORY: /* hangar with a big door */
        block(v, 0, 0, 0, w - 1, 6, dd - 1, 1);
        block(v, 3, 0, dd - 1, w - 4, 4, dd - 1, 2);
        block(v, 0, 7, 2, w - 1, 7, dd - 3, 2);
        break;
    case P03_B_TOWER:
        block(v, 1, 0, 1, 4, 6, 4, 2);
        block(v, 0, 7, 0, 5, 8, 5, 1);
        cadv_voxels_set(v, 2, 9, 2, 3);
        break;
    case P03_B_SPIRE: /* the Ember Spire */
        block(v, 0, 0, 0, w - 1, 3, dd - 1, 2);
        for (k = 0; k < 4; ++k) {
            block(v, 2 + k, 4 + k * 3, 2 + k, w - 3 - k, 6 + k * 3, dd - 3 - k, k % 2 ? 5 : 1);
        }
        block(v, 8, 16, 8, 9, 19, 9, 5);
        break;
    case P03_B_PYLON:
        block(v, 1, 0, 1, 4, 2, 4, 2);
        block(v, 2, 3, 2, 3, 11, 3, 1);
        block(v, 1, 12, 1, 4, 13, 4, 5);
        break;
    default: /* research lab */
        block(v, 0, 0, 0, w - 1, 5, dd - 1, 1);
        block(v, 2, 6, 2, 8, 9, 8, 4);
        block(v, 12, 6, 3, 15, 7, 8, 2);
        break;
    }
}

/* ---- original units (voxel sprites, 8 directions) --------------------------------------- */

static void make_unit(cadv_voxels *v, int def)
{
    switch (def) {
    case P03_U_HARVESTER:
    case P03_U_TANK:
    case P03_U_BUGGY: {
        int len = def == P03_U_HARVESTER ? 20 : (def == P03_U_TANK ? 16 : 12);
        cadv_voxels_alloc(v, 12, 10, len);
        v->pal[1] = cadv_rgba_make(220, 225, 235, 255);
        v->pal[2] = cadv_rgba_make(50, 50, 55, 255);
        v->pal[3] = cadv_rgba_make(255, 230, 140, 255);
        v->pal[4] = cadv_rgba_make(120, 255, 220, 255);
        block(v, 0, 0, 0, 2, 2, len - 1, 2);
        block(v, 9, 0, 0, 11, 2, len - 1, 2);
        block(v, 1, 2, 1, 10, 4, len - 2, 1);
        if (def == P03_U_TANK) {
            block(v, 3, 5, 4, 8, 7, 10, 1);
            block(v, 5, 6, 11, 6, 6, 15, 2);
        } else if (def == P03_U_HARVESTER) {
            block(v, 2, 5, 1, 9, 8, 11, 4);
            block(v, 3, 5, 14, 8, 7, 18, 1);
            block(v, 1, 1, 19, 10, 3, 19, 2);
        } else {
            block(v, 3, 5, 3, 8, 6, 6, 1);
            block(v, 5, 7, 4, 6, 8, 5, 2);
        }
        cadv_voxels_set(v, 2, 4, len - 1, 3);
        cadv_voxels_set(v, 9, 4, len - 1, 3);
        break;
    }
    default: /* infantry: rifle, rocket, engineer, commander */
        cadv_voxels_alloc(v, 8, 16, 8);
        v->pal[1] = cadv_rgba_make(220, 225, 235, 255);
        v->pal[2] = cadv_rgba_make(60, 60, 70, 255);
        v->pal[3] = cadv_rgba_make(240, 200, 170, 255);
        v->pal[4] = def == P03_U_ENGINEER ? cadv_rgba_make(250, 200, 40, 255) : cadv_rgba_make(90, 100, 90, 255);
        block(v, 2, 0, 3, 3, 6, 4, 2);
        block(v, 4, 0, 3, 5, 6, 4, 2);
        block(v, 2, 7, 2, 5, 11, 5, 1);
        block(v, 3, 12, 3, 4, 13, 4, 3);
        block(v, 2, 14, 2, 5, 14, 5, 4);
        if (def == P03_U_ROCKET) {
            block(v, 6, 9, 1, 6, 12, 6, 2);
        } else if (def == P03_U_RIFLE || def == P03_U_COMMANDER) {
            block(v, 6, 8, 4, 6, 8, 7, 2);
        } else {
            block(v, 0, 7, 3, 1, 9, 4, 4);
        }
        break;
    }
}

void cadv_p03_gpu_init(void *state)
{
    p03_state *s = (p03_state *)state;
    p03_gpu *g = &s->g;
    cadv_voxels v;
    cadv_mesh m;
    float size = (float)P03_W * P03_CELL;
    int i;
    int x;
    int z;
    memset(g, 0, sizeof *g);
    g->tex_snow = make_tex(CADV_TEX_NOISE, 31u, cadv_rgba_make(215, 220, 230, 255), cadv_rgba_make(240, 244, 250, 255));
    g->tex_water = make_tex(CADV_TEX_ICE, 32u, cadv_rgba_make(40, 70, 100, 255), cadv_rgba_make(90, 130, 160, 255));
    g->tex_rock = make_tex(CADV_TEX_STONE_BRICKS, 33u, cadv_rgba_make(110, 110, 120, 255), cadv_rgba_make(70, 70, 80, 255));
    cadv_mesh_init(&m);
    cadv_mesh_quad(&m, cadv_v3(0, 0, size), cadv_v3(size, 0, size), cadv_v3(size, 0, 0), cadv_v3(0, 0, 0), 48.0f, 48.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->ground, &m, false);
    /* water, bridge and rocks, merged in horizontal runs */
    cadv_mesh_clear(&m);
    {
        cadv_mesh bridge;
        cadv_mesh rocks;
        cadv_mesh_init(&bridge);
        cadv_mesh_init(&rocks);
        for (z = 0; z < P03_H; ++z) {
            for (x = 0; x < P03_W;) {
                int kind = s->terrain[z * P03_W + x];
                int x1 = x;
                float x0w;
                float x1w;
                while (x1 + 1 < P03_W && s->terrain[z * P03_W + x1 + 1] == kind) {
                    ++x1;
                }
                x0w = (float)x * P03_CELL;
                x1w = (float)(x1 + 1) * P03_CELL;
                if (kind == P03_T_WATER) {
                    cadv_mesh_quad(&m, cadv_v3(x0w, 0.05f, (float)(z + 1) * P03_CELL), cadv_v3(x1w, 0.05f, (float)(z + 1) * P03_CELL),
                                   cadv_v3(x1w, 0.05f, (float)z * P03_CELL), cadv_v3(x0w, 0.05f, (float)z * P03_CELL), 1.0f, 1.0f,
                                   cadv_rgba_make(255, 255, 255, 255));
                } else if (kind == P03_T_BRIDGE) {
                    cadv_mesh_box(&bridge, cadv_v3(x0w, 0.0f, (float)z * P03_CELL), cadv_v3(x1w, 0.4f, (float)(z + 1) * P03_CELL), 0.5f,
                                  cadv_rgba_make(150, 110, 70, 255), false);
                } else if (kind == P03_T_ROCK) {
                    cadv_mesh_box(&rocks, cadv_v3(x0w, 0.0f, (float)z * P03_CELL),
                                  cadv_v3(x1w, 2.0f + (float)((x * 7 + z * 13) % 3), (float)(z + 1) * P03_CELL), 0.5f,
                                  cadv_rgba_make(255, 255, 255, 255), false);
                }
                x = x1 + 1;
            }
        }
        cadv_gpu_mesh_upload(&g->water, &m, false);
        cadv_gpu_mesh_upload(&g->bridge, &bridge, false);
        cadv_gpu_mesh_upload(&g->rocks, &rocks, false);
        cadv_mesh_free(&bridge);
        cadv_mesh_free(&rocks);
    }
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, cadv_rgba_make(255, 255, 255, 255), false);
    cadv_gpu_mesh_upload(&g->cube, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-1, 0.06f, 1), cadv_v3(1, 0.06f, 1), cadv_v3(1, 0.06f, -1), cadv_v3(-1, 0.06f, -1), 1.0f, 1.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->ring, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->plaque, &m, false);
    cadv_mesh_free(&m);
    /* a Frostite crystal cluster */
    cadv_voxels_alloc(&v, 7, 9, 7);
    v.pal[1] = cadv_rgba_make(120, 255, 220, 255);
    v.pal[2] = cadv_rgba_make(60, 200, 255, 255);
    block(&v, 3, 0, 3, 3, 8, 3, 1);
    block(&v, 1, 0, 2, 1, 5, 2, 2);
    block(&v, 5, 0, 4, 5, 6, 4, 2);
    block(&v, 2, 0, 5, 2, 3, 5, 1);
    upload_voxels(&g->crystal, &v, 0.22f);
    /* the Shining Star */
    cadv_voxels_alloc(&v, 15, 15, 3);
    v.pal[1] = cadv_rgba_make(255, 225, 100, 255);
    for (z = 0; z < 15; ++z) {
        for (x = 0; x < 15; ++x) {
            float px = (float)x - 7.0f;
            float py = (float)z - 7.0f;
            float a = cadv_atan2f(py, px) + 0.5f * CADV_PI;
            float r = cadv_sqrtf(px * px + py * py);
            float k = cadv_cosf(5.0f * a);
            if (r <= 3.4f + 3.6f * (k > 0.0f ? k * k : 0.0f)) {
                block(&v, x, z, 0, x, z, 2, 1);
            }
        }
    }
    upload_voxels(&g->star, &v, 0.08f);
    for (i = 0; i < P03_BUILDINGS; ++i) {
        make_building(&v, i);
        upload_voxels(&g->building[i], &v, 0.5f);
    }
    for (i = 0; i < P03_UNITS; ++i) {
        cadv_texdata c;
        cadv_texdata n;
        make_unit(&v, i);
        cadv_voxels_bake_sprites(&v, 8, 4, &c, &n, &g->fw[i], &g->fh[i]);
        g->tex_sprite[i] = cadv_gl_texture(&c, false);
        g->tex_normal[i] = cadv_gl_texture(&n, false);
        cadv_texdata_free(&c);
        cadv_texdata_free(&n);
        cadv_voxels_free(&v);
    }
    for (i = 0; i < 3; ++i) {
        g->tex_mark[i] = text_tex(s->mark_text[i], &g->mark_w[i]);
    }
    s->gpu_ready = true;
}

void cadv_p03_gpu_free(void *state)
{
    p03_state *s = (p03_state *)state;
    p03_gpu *g = &s->g;
    int i;
    cadv_gpu_mesh_free(&g->ground);
    cadv_gpu_mesh_free(&g->water);
    cadv_gpu_mesh_free(&g->rocks);
    cadv_gpu_mesh_free(&g->bridge);
    cadv_gpu_mesh_free(&g->crystal);
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->ring);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    for (i = 0; i < P03_BUILDINGS; ++i) {
        cadv_gpu_mesh_free(&g->building[i]);
    }
    cadv_gl_delete_textures(1, &g->tex_snow);
    cadv_gl_delete_textures(1, &g->tex_water);
    cadv_gl_delete_textures(1, &g->tex_rock);
    cadv_gl_delete_textures(3, g->tex_mark);
    cadv_gl_delete_textures(P03_UNITS, g->tex_sprite);
    cadv_gl_delete_textures(P03_UNITS, g->tex_normal);
    s->gpu_ready = false;
}

/* ---- draw ------------------------------------------------------------------------------ */

static cadv_vec4 side_tint(int owner, float health)
{
    float k = 0.55f + 0.45f * health;
    if (owner == P03_ACCORD) {
        return cadv_v4(0.75f * k, 0.9f * k, 1.15f * k, 1);
    }
    if (owner == P03_ORDER) {
        return cadv_v4(1.2f * k, 0.6f * k, 0.5f * k, 1);
    }
    return cadv_v4(k, k, k, 1);
}

static cadv_mat4 segment(float x0, float z0, float x1, float z1, float y, float thick)
{
    float dx = x1 - x0;
    float dz = z1 - z0;
    float len = cadv_sqrtf(dx * dx + dz * dz);
    return cadv_m4_mul(cadv_m4_translate(cadv_v3(0.5f * (x0 + x1), y, 0.5f * (z0 + z1))),
                       cadv_m4_mul(cadv_m4_rotate_y(cadv_atan2f(dx, dz)), cadv_m4_scale(cadv_v3(thick, thick, len > 0.01f ? len : 0.01f))));
}

static cadv_vec4 map_colour(const p03_state *s, int c)
{
    const cadv_rts *r = &s->rts;
    cadv_vec4 col;
    if (!r->explored[P03_ACCORD][c]) {
        return cadv_v4(0.02f, 0.03f, 0.04f, 1);
    }
    if (r->grid.building[c] >= 0) {
        int owner = r->grid.owner[c];
        col = owner == P03_ACCORD ? cadv_v4(0.3f, 0.5f, 1, 1) : owner == P03_ORDER ? cadv_v4(1, 0.3f, 0.2f, 1) : cadv_v4(0.9f, 0.9f, 0.9f, 1);
    } else if (r->grid.ore[c] > 0) {
        col = cadv_v4(0.3f, 0.9f, 0.7f, 1);
    } else if (s->terrain[c] == P03_T_WATER) {
        col = cadv_v4(0.15f, 0.25f, 0.45f, 1);
    } else if (s->terrain[c] == P03_T_ROCK) {
        col = cadv_v4(0.35f, 0.35f, 0.4f, 1);
    } else {
        col = cadv_v4(0.6f, 0.62f, 0.66f, 1);
    }
    if (!r->visible[P03_ACCORD][c]) {
        col = cadv_v4(col.x * 0.55f, col.y * 0.55f, col.z * 0.55f, 1);
    }
    return col;
}

static bool same(cadv_vec4 a, cadv_vec4 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

static void minimap(const p03_state *s, cadv_ui *ui, float ox, float oy, int px)
{
    const cadv_rts *r = &s->rts;
    int x;
    int z;
    int i;
    cadv_ui_rect(ui, ox - 1, oy - 1, (float)(P03_W * px + 2), (float)(P03_H * px + 2), cadv_v4(0.2f, 0.5f, 0.5f, 1));
    for (z = 0; z < P03_H; ++z) {
        for (x = 0; x < P03_W;) {
            cadv_vec4 col = map_colour(s, z * P03_W + x);
            int x1 = x + 1;
            while (x1 < P03_W && same(map_colour(s, z * P03_W + x1), col)) {
                ++x1; /* one quad per run of equal colour */
            }
            cadv_ui_rect(ui, ox + (float)(x * px), oy + (float)(z * px), (float)((x1 - x) * px), (float)px, col);
            x = x1;
        }
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (u->alive && (u->owner == P03_ACCORD || cadv_rts_cell_visible(r, P03_ACCORD, u->x, u->z))) {
            cadv_vec4 col = i == s->commander ? cadv_v4(1, 1, 0.2f, 1) : u->owner == P03_ACCORD ? cadv_v4(0.5f, 0.8f, 1, 1) : cadv_v4(1, 0.4f, 0.3f, 1);
            cadv_ui_rect(ui, ox + u->x / P03_CELL * (float)px - 1, oy + u->z / P03_CELL * (float)px - 1, (float)px + 1, (float)px + 1, col);
        }
    }
}

static void draw_hud(const p03_state *s, cadv_ui *ui)
{
    const cadv_rts *r = &s->rts;
    const cadv_rts_side *me = &r->side[P03_ACCORD];
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    cadv_vec4 green = cadv_v4(0.5f, 1, 0.8f, 1);
    float cx = (float)ui->width * 0.5f;
    float cy = (float)ui->height * 0.5f;
    float px = (float)ui->width - 72.0f;
    float py = (float)ui->height - 112.0f;
    char buf[96];
    int i;
    int k;
    cadv_ui_rect(ui, cx - 1, cy - 4, 2, 3, white);
    cadv_ui_rect(ui, cx - 1, cy + 2, 2, 3, white);
    cadv_ui_rect(ui, cx - 5, cy, 3, 1, white);
    cadv_ui_rect(ui, cx + 3, cy, 3, 1, white);
    /* the wrist tablet: money and power, the picked item, both queues, minimap */
    cadv_ui_rect(ui, px - 2, py - 2, 72, 112, cadv_v4(0.02f, 0.06f, 0.07f, 0.85f));
    snprintf(buf, sizeof buf, "$%d", me->credits);
    cadv_ui_text(ui, px, py, 1, cadv_v4(1, 0.9f, 0.4f, 1), buf);
    snprintf(buf, sizeof buf, "%d/%d", me->power_used, me->power_made);
    cadv_ui_text(ui, px + 68 - 6.0f * (float)strlen(buf), py, 1, me->power_used > me->power_made ? cadv_v4(1, 0.3f, 0.3f, 1) : green, buf);
    cadv_ui_rect(ui, px - 1, py + 9, 70, 17, cadv_v4(0.1f, 0.35f, 0.3f, 1));
    snprintf(buf, sizeof buf, "%.11s", cadv_p03_tab_name(s->tab_item));
    cadv_ui_text(ui, px, py + 10, 1, white, buf);
    snprintf(buf, sizeof buf, "%d/%d  $%d", s->tab_item + 1, P03_TAB_ITEMS, cadv_p03_tab_cost(s->tab_item));
    cadv_ui_text(ui, px, py + 18, 1, cadv_v4(0.7f, 0.9f, 0.8f, 1), buf);
    for (k = 0; k < 2; ++k) {
        float y = py + 29.0f + (float)k * 5.0f;
        cadv_ui_rect(ui, px, y, 68, 3, cadv_v4(0.1f, 0.15f, 0.15f, 1));
        if (me->queued[k] > 0) {
            const cadv_rts_queue_item *q = &me->queue[k][0];
            int need = (q->building ? cadv_p03_buildings()[q->def].build_ticks : cadv_p03_units()[q->def].build_ticks) * 100;
            cadv_ui_rect(ui, px, y, 68.0f * cadv_minf(1.0f, (float)q->progress / (float)need), 3, k ? cadv_v4(0.4f, 0.7f, 1, 1) : green);
        }
    }
    if (me->ready_building >= 0) {
        cadv_ui_rect(ui, px - 1, py + 39, 70, 3, cadv_v4(1, 1, 0.4f, (s->ticks / 15) % 2 ? 1.0f : 0.3f));
    }
    if (s->tablet_open) {
        minimap(s, ui, cx - (float)P03_W, cy - (float)P03_H, 2);
    } else {
        minimap(s, ui, px + 2, py + 44, 1);
    }
    snprintf(buf, sizeof buf, "HP %d", s->commander >= 0 ? r->u[s->commander].hp : 0);
    cadv_ui_text(ui, 4, (float)ui->height - 12, 1, cadv_v4(1, 0.4f, 0.4f, 1), buf);
    {
        int n = 0;
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            n += s->selected[i] && r->u[i].alive;
        }
        if (n > 0) {
            snprintf(buf, sizeof buf, "SQUAD %d", n);
            cadv_ui_text(ui, 4, (float)ui->height - 22, 1, green, buf);
        }
    }
    if (s->drone_cd > 0 && s->drone == 0) {
        cadv_ui_rect(ui, 4, (float)ui->height - 28, 30.0f * (1.0f - (float)s->drone_cd / (20.0f * 60.0f)), 3, cadv_v4(0.4f, 0.8f, 1, 1));
    }
    if (s->quip_timer > 0) {
        cadv_ui_text_centered(ui, cx, 6, 1, cadv_v4(0.7f, 1, 0.8f, 1), s->quip);
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx - 30, cy + 22, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, (float)ui->width, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(0.5f, 0.9f, 1, 1), "THE FRONT HOLDS!");
        cadv_ui_text_centered(ui, cx, cy + 4, 1, white, "PRESS USE TO RETURN TO THE HALL");
    }
    if (s->lose_timer > 0 || s->commander < 0) {
        cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.5f, 0.1f, 0.1f, 0.3f));
    }
}

void cadv_p03_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p03_state *s = (p03_state *)state;
    const cadv_rts *r = &s->rts;
    p03_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float t = ((float)s->ticks + alpha) * P03_DT;
    int lights = 0;
    int i;
    cam->pos = cadv_p03_eye(s);
    cam->yaw = s->yaw;
    cam->pitch = s->drone > 0 ? cadv_minf(s->pitch, -0.9f) : s->pitch;
    cam->fovy = 1.2f;
    cam->znear = 0.1f;
    cam->zfar = 220.0f;
    sc->ambient = cadv_v3(0.28f, 0.28f, 0.36f);
    sc->sun_dir = cadv_v3_norm(cadv_v3(-0.6f, -0.45f, 0.5f));
    sc->sun_color = cadv_v3(0.95f, 0.65f, 0.5f);
    sc->sun_shadow = true;
    sc->shadow_center = cadv_v3(cam->pos.x, 0, cam->pos.z - 20.0f);
    sc->shadow_extent = 45.0f;
    sc->clear_color = cadv_v3(0.30f, 0.30f, 0.42f);
    sc->fog_color = cadv_v3(0.36f, 0.36f, 0.46f);
    sc->fog_near = 70.0f;
    sc->fog_far = 200.0f;
    sc->light_bands = 6;
    sc->dither = 0.6f;
    sc->use_palette = true;
    cadv_scene_draw(sc, &g->ground, cadv_m4_identity(), cadv_material_make(g->tex_snow, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->water, cadv_m4_identity(), cadv_material_make(g->tex_water, white, 0.15f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->bridge, cadv_m4_identity(), cadv_material_make(0, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->rocks, cadv_m4_identity(), cadv_material_make(g->tex_rock, white, 0.0f, 0));
    /* Frostite: crystals per cell, one light per field */
    for (i = 0; i < P03_W * P03_H; ++i) {
        int ore = r->grid.ore[i];
        if (ore > 0) {
            float x = ((float)(i % P03_W) + 0.5f) * P03_CELL;
            float z = ((float)(i / P03_W) + 0.5f) * P03_CELL;
            float k = 0.5f + 0.7f * cadv_minf(1.0f, (float)ore / 300.0f);
            if ((x - cam->pos.x) * (x - cam->pos.x) + (z - cam->pos.z) * (z - cam->pos.z) < 140.0f * 140.0f) {
                cadv_scene_draw(sc, &g->crystal, cadv_m4_mul(cadv_m4_translate(cadv_v3(x, 0, z)), cadv_m4_mul(cadv_m4_rotate_y((float)i), cadv_m4_scale(cadv_v3(k, k, k)))),
                                cadv_material_make(0, white, 0.7f, CADV_MAT_NO_SHADOW));
            }
            if (i % 5 == 0 && lights < 10 && (x - cam->pos.x) * (x - cam->pos.x) + (z - cam->pos.z) * (z - cam->pos.z) < 60.0f * 60.0f) {
                cadv_scene_light(sc, cadv_v3(x, 1.5f, z), cadv_v3(0.3f, 1.0f, 0.8f), 9.0f, false);
                ++lights;
            }
        }
    }
    /* structures: own always, others once explored */
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        const cadv_rts_bdef *d;
        float x;
        float z;
        if (!b->alive) {
            continue;
        }
        d = &cadv_p03_buildings()[b->def];
        if (b->owner != P03_ACCORD && !r->explored[P03_ACCORD][b->z * P03_W + b->x]) {
            continue;
        }
        x = ((float)b->x + 0.5f * (float)d->fw) * P03_CELL;
        z = ((float)b->z + 0.5f * (float)d->fh) * P03_CELL;
        cadv_scene_draw(sc, &g->building[b->def], cadv_m4_translate(cadv_v3(x, 0, z)),
                        cadv_material_make(0, side_tint(b->owner, (float)b->hp / (float)d->hp), b->def == P03_B_SPIRE ? 0.15f : 0.03f, 0));
        if (b->def == P03_B_SPIRE && lights < 24) {
            cadv_scene_light(sc, cadv_v3(x, 10.0f, z), cadv_v3(2.0f, 0.7f, 0.3f), 24.0f, false);
            ++lights;
        }
        if (b->def == P03_B_LAB) {
            cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(x, 3.4f, z + (float)d->fh * P03_CELL * 0.5f + 0.05f)),
                                                        cadv_m4_scale(cadv_v3((float)g->mark_w[0] / 8.0f, 1.6f, 1))),
                            cadv_material_make(g->tex_mark[0], white, 0.8f, CADV_MAT_NO_SHADOW));
        }
    }
    /* units: own always, enemies when in sight */
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        const cadv_rts_udef *d;
        cadv_sprite sp;
        int frame;
        float scale;
        if (!u->alive || i == s->commander || (u->owner != P03_ACCORD && !cadv_rts_cell_visible(r, P03_ACCORD, u->x, u->z))) {
            continue;
        }
        d = &cadv_p03_units()[u->def];
        scale = d->infantry ? 0.11f : 0.2f;
        memset(&sp, 0, sizeof sp);
        frame = cadv_sprite_frame(cadv_v3(u->x, 0, u->z), u->yaw, cam->pos, 8);
        sp.pos = cadv_v3(u->x, 0, u->z);
        sp.w = (float)g->fw[u->def] / 4.0f * scale;
        sp.h = (float)g->fh[u->def] / 4.0f * scale;
        sp.tex = g->tex_sprite[u->def];
        sp.normal_tex = g->tex_normal[u->def];
        sp.u0 = (float)frame / 8.0f;
        sp.u1 = (float)(frame + 1) / 8.0f;
        sp.v1 = 1.0f;
        sp.tint = side_tint(u->owner, (float)u->hp / (float)d->hp);
        sp.casts_shadow = true;
        cadv_scene_sprite(sc, &sp);
        if (s->selected[i]) {
            float rr = d->infantry ? 0.7f : 1.6f;
            cadv_scene_draw(sc, &g->ring, cadv_m4_mul(cadv_m4_translate(cadv_v3(u->x, 0, u->z)), cadv_m4_scale(cadv_v3(rr, 1, rr))),
                            cadv_material_make(0, cadv_v4(0.3f, 1, 0.5f, 1), 0.8f, CADV_MAT_NO_SHADOW | CADV_MAT_UNLIT));
        }
    }
    /* tracers and muzzle light */
    for (i = 0; i < CADV_RTS_SHOTS; ++i) {
        const cadv_rts_shot *sh = &r->shots[i];
        if (sh->ttl > 0) {
            cadv_vec3 c = sh->owner == P03_ACCORD ? cadv_v3(1.8f, 1.5f, 0.6f) : cadv_v3(1.8f, 0.5f, 0.3f);
            cadv_scene_draw(sc, &g->cube, segment(sh->x0, sh->z0, sh->x1, sh->z1, 1.1f, 0.08f),
                            cadv_material_make(0, cadv_v4(c.x, c.y, c.z, 1), 1.5f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            if (lights < 30 && sh->ttl > 5) {
                cadv_scene_light(sc, cadv_v3(sh->x0, 1.4f, sh->z0), c, 7.0f, lights == 0);
                ++lights;
            }
        }
    }
    /* the holo-placement beam */
    if (s->placing && r->side[P03_ACCORD].ready_building >= 0 && s->commander >= 0) {
        const cadv_rts_bdef *d = &cadv_p03_buildings()[r->side[P03_ACCORD].ready_building];
        float gx;
        float gz;
        if (cadv_p03_aim_ground(s, &gx, &gz)) {
            cadv_cell c = cadv_rts_cell_at(&r->grid, gx, gz);
            int x0 = c.x - d->fw / 2;
            int z0 = c.z - d->fh / 2;
            bool ok = cadv_rts_can_place(&r->grid, x0, z0, d->fw, d->fh, P03_ACCORD, 3, false) == CADV_PLACE_OK;
            cadv_vec4 col = ok ? cadv_v4(0.3f, 1.2f, 0.6f, 1) : cadv_v4(1.4f, 0.3f, 0.3f, 1);
            float ax = (float)x0 * P03_CELL;
            float az = (float)z0 * P03_CELL;
            float bx = (float)(x0 + d->fw) * P03_CELL;
            float bz = (float)(z0 + d->fh) * P03_CELL;
            uint32_t f = CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW;
            cadv_scene_draw(sc, &g->cube, segment(ax, az, bx, az, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
            cadv_scene_draw(sc, &g->cube, segment(ax, bz, bx, bz, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
            cadv_scene_draw(sc, &g->cube, segment(ax, az, ax, bz, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
            cadv_scene_draw(sc, &g->cube, segment(bx, az, bx, bz, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
            cadv_scene_light(sc, cadv_v3(gx, 1.0f, gz), cadv_v3(col.x * 0.6f, col.y * 0.6f, col.z * 0.6f), 8.0f, false);
        }
    }
    if (s->star_state == 1 || s->star_state == 2) {
        cadv_vec3 p = cadv_v3(s->star_x, 2.0f + 0.2f * cadv_sinf(t * 2.0f), s->star_z);
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(t * 1.5f)), cadv_material_make(0, white, 1.2f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, p, cadv_v3(2.0f, 1.7f, 0.6f), 12.0f, false);
    }
    /* the commander's wrist tablet glows at the bottom of the view */
    if (s->commander >= 0 && s->drone == 0) {
        cadv_vec3 e = cam->pos;
        cadv_mat4 body = cadv_m4_mul(cadv_m4_translate(cadv_v3(e.x, e.y - 0.55f, e.z)), cadv_m4_rotate_y(s->yaw));
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.28f, 0.0f, -0.5f)), cadv_m4_scale(cadv_v3(0.16f, 0.04f, 0.22f)))),
                        cadv_material_make(0, cadv_v4(0.3f, 0.9f, 0.8f, 1), 0.6f, CADV_MAT_NO_SHADOW));
    }
    if (ui) {
        draw_hud(s, ui);
    }
}

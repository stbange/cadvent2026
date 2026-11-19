/* SPDX-License-Identifier: MIT */
/* GPU side of Day 5: the snowy valley of Kindlewick, the frozen (later open)
 * lake, felled-as-you-go forests, gnome timber and troll bone structures,
 * unit sprites with the ferry, troll torches, the snow-owl, the tavern sign,
 * the fire pit with its star and the Captain's command stave. */
#include "portals/p05_hearthguard_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portals/p05_internal.h"

static unsigned make_tex(cadv_tex_kind kind, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, kind, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

/* A painted wooden sign: warm letters on dark boards. */
static unsigned sign_tex(const char *text, int *w_out)
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
        int board = (i / w) % 4 == 0 ? 10 : 0;
        t.rgba[i * 4] = (uint8_t)(70 - board);
        t.rgba[i * 4 + 1] = (uint8_t)(45 - board);
        t.rgba[i * 4 + 2] = (uint8_t)(25 - board / 2);
        t.rgba[i * 4 + 3] = 255;
    }
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y)) {
                    cadv_texdata_set(&t, 3 + i * 6 + x, 2 + y, 255, 210, 120, 255);
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

/* A pitched roof along x over [x0,x1] x [z0,z1], starting at height y. */
static void roof(cadv_voxels *v, int x0, int x1, int z0, int z1, int y, uint8_t c)
{
    int k = 0;
    while (z0 + k <= z1 - k) {
        block(v, x0, y + k, z0 + k, x1, y + k, z1 - k, c);
        ++k;
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
    const cadv_rts_bdef *d = &cadv_p05_buildings()[def];
    int w = d->fw * 6;
    int dd = d->fh * 6;
    cadv_voxels_alloc(v, w, 22, dd);
    v->pal[1] = cadv_rgba_make(205, 190, 165, 255); /* timber / hide (tinted per side) */
    v->pal[2] = cadv_rgba_make(95, 70, 50, 255);    /* beams */
    v->pal[3] = cadv_rgba_make(255, 215, 130, 255); /* warm windows */
    v->pal[4] = cadv_rgba_make(235, 240, 250, 255); /* snow on the roofs */
    v->pal[5] = cadv_rgba_make(225, 215, 190, 255); /* bone */
    v->pal[6] = cadv_rgba_make(120, 120, 130, 255); /* stone */
    v->pal[7] = cadv_rgba_make(190, 60, 40, 255);   /* banner red */
    switch (def) {
    case P05_B_HALL:
    case P05_B_KEEP:
        block(v, 1, 0, 1, w - 2, 6, dd - 2, def == P05_B_KEEP ? 6 : 1);
        block(v, 1, 0, 1, 1, 6, dd - 2, 2);
        block(v, w - 2, 0, 1, w - 2, 6, dd - 2, 2);
        roof(v, 0, w - 1, 0, dd - 1, 7, 4);
        block(v, 7, 0, dd - 2, 10, 3, dd - 2, 2);
        cadv_voxels_set(v, 4, 4, dd - 2, 3);
        cadv_voxels_set(v, 13, 4, dd - 2, 3);
        block(v, 14, 7, 3, 15, 15, 4, 6); /* chimney */
        if (def == P05_B_KEEP) {
            block(v, 0, 7, 0, 3, 13, 3, 6);
            block(v, w - 4, 7, 0, w - 1, 13, 3, 6);
            block(v, 1, 14, 1, 1, 17, 1, 2);
            block(v, 2, 16, 1, 3, 17, 1, 7);
        }
        break;
    case P05_B_FARM: /* cottage with a fenced patch */
        block(v, 0, 0, 0, w - 1, 0, dd - 1, 2);
        block(v, 1, 1, 1, 6, 4, 6, 1);
        roof(v, 0, 7, 0, 7, 5, 4);
        cadv_voxels_set(v, 3, 2, 7, 3);
        block(v, 8, 1, 2, 11, 1, 11, 2);
        break;
    case P05_B_BARRACKS:
        block(v, 0, 0, 1, w - 1, 5, dd - 2, 1);
        roof(v, 0, w - 1, 0, dd - 1, 6, 4);
        block(v, 7, 0, dd - 2, 10, 3, dd - 2, 2);
        block(v, w - 2, 6, 1, w - 2, 13, 1, 2);
        block(v, w - 4, 11, 1, w - 3, 13, 1, 7);
        break;
    case P05_B_MILL: /* saw shed and a log pile */
        block(v, 0, 0, 0, 7, 4, w - 1, 1);
        roof(v, 0, 7, 0, w - 1, 5, 4);
        block(v, 8, 0, 1, 11, 1, 10, 2);
        block(v, 9, 2, 2, 10, 2, 9, 2);
        break;
    case P05_B_TOWER:
        block(v, 1, 0, 1, 4, 10, 4, 2);
        block(v, 0, 11, 0, 5, 12, 5, 1);
        roof(v, 0, 5, 0, 5, 13, 4);
        cadv_voxels_set(v, 2, 12, 5, 3);
        break;
    case P05_B_SHIPYARD: /* slipway with a hull on the stocks */
        block(v, 0, 0, 0, w - 1, 1, dd - 1, 2);
        block(v, 4, 2, 2, 13, 4, 8, 1);
        block(v, 6, 5, 4, 11, 5, 6, 2);
        block(v, 1, 2, 1, 1, 9, 1, 2);
        block(v, 16, 2, 1, 16, 9, 1, 2);
        block(v, 1, 9, 1, 16, 9, 1, 2);
        break;
    case P05_B_WORKSHOP: /* forge with a tallow lamp */
        block(v, 0, 0, 0, w - 1, 5, dd - 1, 6);
        roof(v, 0, w - 1, 0, dd - 1, 6, 4);
        block(v, 9, 6, 2, 10, 11, 3, 6);
        block(v, 4, 1, dd - 1, 7, 3, dd - 1, 3);
        break;
    case P05_B_LONGHALL: /* the troll longhall: bone ribs over hides */
        block(v, 0, 0, 0, w - 1, 7, dd - 1, 1);
        roof(v, 0, w - 1, 0, dd - 1, 8, 2);
        block(v, 2, 0, dd - 1, 2, 16, dd - 1, 5);
        block(v, 15, 0, dd - 1, 15, 16, dd - 1, 5);
        block(v, 3, 15, dd - 1, 14, 15, dd - 1, 5);
        block(v, 7, 0, dd - 1, 10, 5, dd - 1, 2);
        break;
    case P05_B_HUT:
        block(v, 1, 0, 1, w - 2, 5, dd - 2, 1);
        roof(v, 0, w - 1, 0, dd - 1, 6, 2);
        block(v, 3, 6, 3, 3, 11, 3, 5);
        block(v, 14, 6, 3, 14, 11, 3, 5);
        break;
    case P05_B_TOTEM:
        block(v, 2, 0, 2, 3, 12, 3, 5);
        block(v, 1, 9, 1, 4, 11, 4, 5);
        cadv_voxels_set(v, 1, 10, 4, 7);
        cadv_voxels_set(v, 4, 10, 4, 7);
        break;
    default: /* the tavern */
        block(v, 0, 0, 0, w - 1, 6, dd - 1, 1);
        roof(v, 0, w - 1, 0, dd - 1, 7, 4);
        block(v, 2, 2, dd - 1, 4, 4, dd - 1, 3);
        block(v, 7, 2, dd - 1, 9, 4, dd - 1, 3);
        block(v, 9, 7, 2, 10, 13, 3, 6);
        break;
    }
}

/* ---- original units (voxel sprites, 8 directions) --------------------------------------- */

static void make_unit(cadv_voxels *v, int def)
{
    if (def == P05_U_BALLISTA || def == P05_U_FERRY) {
        int len = def == P05_U_FERRY ? 24 : 14;
        int wid = def == P05_U_FERRY ? 12 : 10;
        cadv_voxels_alloc(v, wid, 14, len);
        v->pal[1] = cadv_rgba_make(200, 180, 150, 255);
        v->pal[2] = cadv_rgba_make(90, 65, 45, 255);
        v->pal[3] = cadv_rgba_make(235, 235, 225, 255);
        v->pal[4] = cadv_rgba_make(255, 215, 130, 255);
        if (def == P05_U_FERRY) {
            block(v, 1, 0, 2, wid - 2, 1, len - 3, 2);
            block(v, 0, 2, 1, wid - 1, 3, len - 2, 1);
            block(v, 2, 2, 3, wid - 3, 2, len - 4, 2);
            block(v, 5, 4, 10, 6, 12, 11, 2);
            block(v, 2, 6, 11, 9, 11, 11, 3); /* sail */
            cadv_voxels_set(v, 6, 4, len - 2, 4);
        } else {
            block(v, 0, 0, 2, 1, 2, 4, 2);
            block(v, 8, 0, 2, 9, 2, 4, 2);
            block(v, 0, 0, 9, 1, 2, 11, 2);
            block(v, 8, 0, 9, 9, 2, 11, 2);
            block(v, 2, 2, 1, 7, 3, 12, 1);
            block(v, 0, 5, 9, 9, 5, 9, 2);
            block(v, 4, 4, 2, 5, 4, 13, 3);
        }
        return;
    }
    {
        bool troll = def == P05_U_AXE || def == P05_U_THROWER || def == P05_U_WARLORD;
        int h = troll ? 18 : 12; /* gnomes are short, trolls are not */
        cadv_voxels_alloc(v, 8, h + 4, 8);
        v->pal[1] = cadv_rgba_make(215, 205, 190, 255);
        v->pal[2] = cadv_rgba_make(70, 55, 45, 255);
        v->pal[3] = troll ? cadv_rgba_make(120, 170, 140, 255) : cadv_rgba_make(240, 200, 170, 255);
        v->pal[4] = troll ? cadv_rgba_make(80, 60, 40, 255) : cadv_rgba_make(200, 50, 40, 255); /* hair / gnome hat */
        v->pal[5] = cadv_rgba_make(180, 185, 195, 255);
        v->pal[6] = cadv_rgba_make(250, 250, 250, 255);
        block(v, 2, 0, 3, 3, h / 3, 4, 2);
        block(v, 4, 0, 3, 5, h / 3, 4, 2);
        block(v, 2, h / 3 + 1, 2, 5, h * 2 / 3, 5, 1);
        block(v, 2, h * 2 / 3 + 1, 2, 5, h * 2 / 3 + 2, 5, 3);
        if (troll) {
            block(v, 2, h * 2 / 3 + 3, 2, 5, h * 2 / 3 + 3, 5, 4);
            cadv_voxels_set(v, 2, h * 2 / 3 + 1, 5, 6); /* tusks */
            cadv_voxels_set(v, 5, h * 2 / 3 + 1, 5, 6);
        } else {
            /* the pointed gnome hat; workers get a beard */
            block(v, 2, h * 2 / 3 + 3, 2, 5, h * 2 / 3 + 3, 5, 4);
            block(v, 3, h * 2 / 3 + 4, 3, 4, h * 2 / 3 + 5, 4, 4);
            if (def == P05_U_WORKER) {
                block(v, 2, h * 2 / 3, 5, 5, h * 2 / 3, 5, 6);
            }
        }
        if (def == P05_U_ARCHER || def == P05_U_THROWER) {
            block(v, 6, h / 3, 3, 6, h * 2 / 3 + 1, 3, 2);
        } else if (def == P05_U_WORKER) {
            block(v, 6, h / 3, 4, 6, h / 2 + 2, 4, 2);
            cadv_voxels_set(v, 7, h / 2 + 2, 4, 5);
        } else {
            block(v, 6, h / 3, 4, 6, h * 2 / 3 + 2, 4, 5);
            block(v, 0, h / 3 + 1, 2, 1, h * 2 / 3, 5, def == P05_U_KNIGHT ? 5 : 2);
        }
    }
}

/* The forest as one mesh, rebuilt whenever a tree falls. */
static void build_forest(p05_state *s)
{
    const cadv_rts *r = &s->rts;
    cadv_mesh m;
    int i;
    cadv_mesh_init(&m);
    for (i = 0; i < P05_W * P05_H; ++i) {
        if (r->grid.ore[i] > 0 && r->grid.ore_kind[i] == 1) {
            float x = ((float)(i % P05_W) + 0.5f) * P05_CELL;
            float z = ((float)(i / P05_W) + 0.5f) * P05_CELL;
            float h = 4.0f + (float)((i * 7) % 5) * 0.5f;
            float j = (float)((i * 13) % 7) * 0.12f - 0.36f;
            cadv_mesh_box(&m, cadv_v3(x + j - 0.25f, 0, z - 0.25f), cadv_v3(x + j + 0.25f, 1.2f, z + 0.25f), 0.0f,
                          cadv_rgba_make(110, 80, 55, 255), false);
            cadv_mesh_box(&m, cadv_v3(x + j - 1.3f, 1.0f, z - 1.3f), cadv_v3(x + j + 1.3f, 1.0f + h * 0.45f, z + 1.3f), 0.0f,
                          cadv_rgba_make(50, 95, 70, 255), false);
            cadv_mesh_box(&m, cadv_v3(x + j - 0.8f, 1.0f + h * 0.45f, z - 0.8f), cadv_v3(x + j + 0.8f, 1.0f + h * 0.8f, z + 0.8f), 0.0f,
                          cadv_rgba_make(60, 110, 80, 255), false);
            cadv_mesh_box(&m, cadv_v3(x + j - 0.35f, 1.0f + h * 0.8f, z - 0.35f), cadv_v3(x + j + 0.35f, 1.2f + h, z + 0.35f), 0.0f,
                          cadv_rgba_make(235, 240, 250, 255), false);
        }
    }
    cadv_gpu_mesh_free(&s->g.tree);
    cadv_gpu_mesh_upload(&s->g.tree, &m, false);
    cadv_mesh_free(&m);
    s->g.grid_version = r->grid.version;
}

void cadv_p05_gpu_init(void *state)
{
    p05_state *s = (p05_state *)state;
    p05_gpu *g = &s->g;
    cadv_voxels v;
    cadv_mesh m;
    float size = (float)P05_W * P05_CELL;
    int i;
    int x;
    int z;
    memset(g, 0, sizeof *g);
    g->tex_snow = make_tex(CADV_TEX_NOISE, 51u, cadv_rgba_make(220, 225, 235, 255), cadv_rgba_make(245, 248, 252, 255));
    g->tex_ice = make_tex(CADV_TEX_ICE, 52u, cadv_rgba_make(150, 190, 215, 255), cadv_rgba_make(210, 230, 245, 255));
    g->tex_water = make_tex(CADV_TEX_ICE, 53u, cadv_rgba_make(30, 60, 95, 255), cadv_rgba_make(70, 110, 150, 255));
    g->tex_rock = make_tex(CADV_TEX_STONE_BRICKS, 54u, cadv_rgba_make(110, 110, 120, 255), cadv_rgba_make(70, 70, 80, 255));
    cadv_mesh_init(&m);
    cadv_mesh_quad(&m, cadv_v3(0, 0, size), cadv_v3(size, 0, size), cadv_v3(size, 0, 0), cadv_v3(0, 0, 0), 48.0f, 48.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->ground, &m, false);
    cadv_mesh_clear(&m);
    {
        float x0 = P05_CELL;
        float x1 = (float)(P05_W - 1) * P05_CELL;
        float z0 = (float)P05_LAKE_Z0 * P05_CELL;
        float z1 = (float)(P05_LAKE_Z1 + 1) * P05_CELL;
        cadv_mesh_quad(&m, cadv_v3(x0, 0.04f, z1), cadv_v3(x1, 0.04f, z1), cadv_v3(x1, 0.04f, z0), cadv_v3(x0, 0.04f, z0), 20.0f, 4.0f,
                       cadv_rgba_make(255, 255, 255, 255));
        cadv_gpu_mesh_upload(&g->lake, &m, false);
    }
    cadv_mesh_clear(&m);
    for (z = 0; z < P05_H; ++z) {
        for (x = 0; x < P05_W;) {
            int x1 = x;
            if (s->terrain[z * P05_W + x] != P05_T_ROCK) {
                ++x;
                continue;
            }
            while (x1 + 1 < P05_W && s->terrain[z * P05_W + x1 + 1] == P05_T_ROCK) {
                ++x1;
            }
            cadv_mesh_box(&m, cadv_v3((float)x * P05_CELL, 0.0f, (float)z * P05_CELL),
                          cadv_v3((float)(x1 + 1) * P05_CELL, 2.5f + (float)((x * 7 + z * 13) % 3), (float)(z + 1) * P05_CELL), 0.5f,
                          cadv_rgba_make(255, 255, 255, 255), false);
            x = x1 + 1;
        }
    }
    cadv_gpu_mesh_upload(&g->rocks, &m, false);
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
    /* the gold mine: a timbered adit in a rock face, gold glinting */
    cadv_voxels_alloc(&v, 12, 10, 12);
    v.pal[1] = cadv_rgba_make(120, 115, 120, 255);
    v.pal[2] = cadv_rgba_make(95, 70, 50, 255);
    v.pal[3] = cadv_rgba_make(255, 210, 80, 255);
    v.pal[4] = cadv_rgba_make(20, 15, 15, 255);
    block(&v, 0, 0, 0, 11, 8, 11, 1);
    block(&v, 3, 0, 11, 8, 5, 11, 2);
    block(&v, 4, 0, 11, 7, 4, 11, 4);
    cadv_voxels_set(&v, 2, 7, 11, 3);
    cadv_voxels_set(&v, 9, 3, 11, 3);
    cadv_voxels_set(&v, 10, 8, 6, 3);
    upload_voxels(&g->mine, &v, 0.5f);
    /* the whale-tallow platform: planks on stilts with a rendering pot */
    cadv_voxels_alloc(&v, 12, 8, 6);
    v.pal[1] = cadv_rgba_make(150, 110, 70, 255);
    v.pal[2] = cadv_rgba_make(60, 60, 65, 255);
    v.pal[3] = cadv_rgba_make(250, 235, 190, 255);
    block(&v, 0, 2, 0, 11, 2, 5, 1);
    block(&v, 0, 0, 0, 0, 1, 0, 1);
    block(&v, 11, 0, 0, 11, 1, 0, 1);
    block(&v, 0, 0, 5, 0, 1, 5, 1);
    block(&v, 11, 0, 5, 11, 1, 5, 1);
    block(&v, 3, 3, 1, 6, 6, 4, 2);
    block(&v, 4, 7, 2, 5, 7, 3, 3);
    block(&v, 8, 3, 1, 9, 4, 2, 3);
    upload_voxels(&g->platform, &v, 0.5f);
    /* the fire pit before the Longhall */
    cadv_voxels_alloc(&v, 8, 3, 8);
    v.pal[1] = cadv_rgba_make(110, 105, 100, 255);
    v.pal[2] = cadv_rgba_make(255, 140, 50, 255);
    block(&v, 0, 0, 0, 7, 1, 7, 1);
    block(&v, 2, 1, 2, 5, 2, 5, 2);
    upload_voxels(&g->fire, &v, 0.4f);
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
                block(&v, x, z, 0, x, z, 2, 1);
            }
        }
    }
    upload_voxels(&g->star, &v, 0.08f);
    for (i = 0; i < P05_BUILDINGS; ++i) {
        make_building(&v, i);
        upload_voxels(&g->building[i], &v, 0.5f);
    }
    for (i = 0; i < P05_UNITS; ++i) {
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
    /* the snow-owl: a white bird with spread wings, seen from below */
    {
        cadv_texdata c;
        cadv_texdata n;
        cadv_voxels_alloc(&v, 14, 4, 6);
        v.pal[1] = cadv_rgba_make(245, 245, 250, 255);
        v.pal[2] = cadv_rgba_make(250, 200, 60, 255);
        block(&v, 5, 0, 1, 8, 3, 4, 1);
        block(&v, 0, 2, 2, 13, 2, 3, 1);
        cadv_voxels_set(&v, 6, 3, 4, 2);
        cadv_voxels_set(&v, 7, 3, 4, 2);
        cadv_voxels_bake_sprites(&v, 1, 4, &c, &n, &g->owl_w, &g->owl_h);
        g->tex_owl = cadv_gl_texture(&c, false);
        cadv_texdata_free(&c);
        cadv_texdata_free(&n);
        cadv_voxels_free(&v);
    }
    g->tex_sign = sign_tex(s->mark_text[3], &g->sign_w);
    build_forest(s);
    s->gpu_ready = true;
}

void cadv_p05_gpu_free(void *state)
{
    p05_state *s = (p05_state *)state;
    p05_gpu *g = &s->g;
    int i;
    cadv_gpu_mesh_free(&g->ground);
    cadv_gpu_mesh_free(&g->lake);
    cadv_gpu_mesh_free(&g->rocks);
    cadv_gpu_mesh_free(&g->tree);
    cadv_gpu_mesh_free(&g->mine);
    cadv_gpu_mesh_free(&g->platform);
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->ring);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->fire);
    for (i = 0; i < P05_BUILDINGS; ++i) {
        cadv_gpu_mesh_free(&g->building[i]);
    }
    cadv_gl_delete_textures(1, &g->tex_snow);
    cadv_gl_delete_textures(1, &g->tex_ice);
    cadv_gl_delete_textures(1, &g->tex_water);
    cadv_gl_delete_textures(1, &g->tex_rock);
    cadv_gl_delete_textures(1, &g->tex_sign);
    cadv_gl_delete_textures(1, &g->tex_owl);
    cadv_gl_delete_textures(P05_UNITS, g->tex_sprite);
    cadv_gl_delete_textures(P05_UNITS, g->tex_normal);
    s->gpu_ready = false;
}

/* ---- draw ------------------------------------------------------------------------------ */

static cadv_vec4 side_tint(int owner, float health)
{
    float k = 0.55f + 0.45f * health;
    if (owner == P05_GNOMES) {
        return cadv_v4(1.0f * k, 0.95f * k, 1.1f * k, 1);
    }
    if (owner == P05_TROLLS) {
        return cadv_v4(0.85f * k, 1.05f * k, 0.8f * k, 1);
    }
    return cadv_v4(1.05f * k, 0.95f * k, 0.85f * k, 1);
}

static cadv_mat4 segment(float x0, float z0, float x1, float z1, float y, float thick)
{
    float dx = x1 - x0;
    float dz = z1 - z0;
    float len = cadv_sqrtf(dx * dx + dz * dz);
    return cadv_m4_mul(cadv_m4_translate(cadv_v3(0.5f * (x0 + x1), y, 0.5f * (z0 + z1))),
                       cadv_m4_mul(cadv_m4_rotate_y(cadv_atan2f(dx, dz)), cadv_m4_scale(cadv_v3(thick, thick, len > 0.01f ? len : 0.01f))));
}

static cadv_vec4 map_colour(const p05_state *s, int c)
{
    const cadv_rts *r = &s->rts;
    cadv_vec4 col;
    if (!r->explored[P05_GNOMES][c]) {
        return cadv_v4(0.02f, 0.03f, 0.04f, 1);
    }
    if (r->grid.building[c] >= 0) {
        int owner = r->grid.owner[c];
        col = owner == P05_GNOMES ? cadv_v4(0.4f, 0.6f, 1, 1) : owner == P05_TROLLS ? cadv_v4(1, 0.3f, 0.2f, 1) : cadv_v4(0.9f, 0.8f, 0.5f, 1);
    } else if (r->grid.ore[c] > 0) {
        int k = r->grid.ore_kind[c];
        col = k == 0 ? cadv_v4(1, 0.85f, 0.3f, 1) : k == 1 ? cadv_v4(0.2f, 0.45f, 0.3f, 1) : cadv_v4(0.95f, 0.9f, 0.75f, 1);
    } else if (s->terrain[c] == P05_T_LAKE) {
        col = s->thawed ? cadv_v4(0.15f, 0.25f, 0.45f, 1) : cadv_v4(0.55f, 0.7f, 0.85f, 1);
    } else if (s->terrain[c] == P05_T_ROCK) {
        col = cadv_v4(0.35f, 0.35f, 0.4f, 1);
    } else {
        col = cadv_v4(0.75f, 0.77f, 0.8f, 1);
    }
    if (!r->visible[P05_GNOMES][c]) {
        col = cadv_v4(col.x * 0.55f, col.y * 0.55f, col.z * 0.55f, 1);
    }
    return col;
}

static bool same(cadv_vec4 a, cadv_vec4 b) { return a.x == b.x && a.y == b.y && a.z == b.z; }

static void minimap(const p05_state *s, cadv_ui *ui, float ox, float oy, int px)
{
    const cadv_rts *r = &s->rts;
    int x;
    int z;
    int i;
    cadv_ui_rect(ui, ox - 1, oy - 1, (float)(P05_W * px + 2), (float)(P05_H * px + 2), cadv_v4(0.55f, 0.4f, 0.25f, 1));
    for (z = 0; z < P05_H; ++z) {
        for (x = 0; x < P05_W;) {
            cadv_vec4 col = map_colour(s, z * P05_W + x);
            int x1 = x + 1;
            while (x1 < P05_W && same(map_colour(s, z * P05_W + x1), col)) {
                ++x1; /* one quad per run of equal colour */
            }
            cadv_ui_rect(ui, ox + (float)(x * px), oy + (float)(z * px), (float)((x1 - x) * px), (float)px, col);
            x = x1;
        }
    }
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        if (u->alive && u->carrier < 0 && (u->owner == P05_GNOMES || cadv_rts_cell_visible(r, P05_GNOMES, u->x, u->z))) {
            cadv_vec4 col = i == s->captain ? cadv_v4(1, 1, 0.2f, 1) : u->owner == P05_GNOMES ? cadv_v4(0.6f, 0.85f, 1, 1) : cadv_v4(1, 0.4f, 0.3f, 1);
            cadv_ui_rect(ui, ox + u->x / P05_CELL * (float)px - 1, oy + u->z / P05_CELL * (float)px - 1, (float)px + 1, (float)px + 1, col);
        }
    }
}

static void draw_hud(const p05_state *s, cadv_ui *ui)
{
    const cadv_rts *r = &s->rts;
    const cadv_rts_side *me = &r->side[P05_GNOMES];
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    cadv_vec4 warm = cadv_v4(1, 0.85f, 0.55f, 1);
    float cx = (float)ui->width * 0.5f;
    float cy = (float)ui->height * 0.5f;
    float px = (float)ui->width - 72.0f;
    float py = (float)ui->height - 120.0f;
    char buf[96];
    int i;
    int k;
    int gold;
    int lumber;
    int tallow;
    cadv_ui_rect(ui, cx - 1, cy - 4, 2, 3, white);
    cadv_ui_rect(ui, cx - 1, cy + 2, 2, 3, white);
    cadv_ui_rect(ui, cx - 5, cy, 3, 1, white);
    cadv_ui_rect(ui, cx + 3, cy, 3, 1, white);
    /* the resource bar along the top left */
    cadv_ui_rect(ui, 2, 2, 150, 10, cadv_v4(0.08f, 0.05f, 0.03f, 0.75f));
    snprintf(buf, sizeof buf, "G%d", me->credits);
    cadv_ui_text(ui, 4, 3, 1, cadv_v4(1, 0.85f, 0.3f, 1), buf);
    snprintf(buf, sizeof buf, "L%d", me->lumber);
    cadv_ui_text(ui, 40, 3, 1, cadv_v4(0.5f, 0.9f, 0.5f, 1), buf);
    snprintf(buf, sizeof buf, "T%d", me->tallow);
    cadv_ui_text(ui, 76, 3, 1, cadv_v4(0.95f, 0.9f, 0.75f, 1), buf);
    snprintf(buf, sizeof buf, "%d/%d", me->supply_used, me->supply_cap);
    cadv_ui_text(ui, 112, 3, 1, me->supply_used >= me->supply_cap ? cadv_v4(1, 0.35f, 0.3f, 1) : white, buf);
    /* the raid clock, then spring */
    if (!s->thawed) {
        if (cadv_p05_raiders_alive(s) > 0) {
            snprintf(buf, sizeof buf, "RAID! %d LEFT", cadv_p05_raiders_alive(s));
        } else if (s->raid < CADV_P05_RAIDS) {
            int sec = s->raid_timer / 60;
            snprintf(buf, sizeof buf, "RAID %d IN %d:%02d", s->raid + 1, sec / 60, sec % 60);
        } else {
            snprintf(buf, sizeof buf, "THE ICE CRACKS...");
        }
        cadv_ui_text(ui, 4, 14, 1, cadv_v4(1, 0.55f, 0.35f, 1), buf);
    } else if (s->star_state == 0) {
        cadv_ui_text(ui, 4, 14, 1, cadv_v4(0.6f, 0.85f, 1, 1), "SPRING: BURN THE LONGHALL");
    } else if (s->star_state < 3) {
        cadv_ui_text(ui, 4, 14, 1, cadv_v4(1, 0.9f, 0.4f, 1), "BRING THE STAR HOME");
    }
    /* the command stave: picked item, cost, both queues, minimap */
    cadv_ui_rect(ui, px - 2, py - 2, 72, 120, cadv_v4(0.1f, 0.06f, 0.03f, 0.85f));
    cadv_ui_rect(ui, px - 1, py - 1, 70, 17, cadv_v4(0.35f, 0.22f, 0.12f, 1));
    snprintf(buf, sizeof buf, "%.11s", cadv_p05_stave_name(s->stave_item));
    cadv_ui_text(ui, px, py, 1, white, buf);
    cadv_p05_stave_cost(s->stave_item, &gold, &lumber, &tallow);
    if (tallow > 0) {
        snprintf(buf, sizeof buf, "%d %d %d", gold, lumber, tallow);
    } else {
        snprintf(buf, sizeof buf, "G%d L%d", gold, lumber);
    }
    cadv_ui_text(ui, px, py + 8, 1, me->credits >= gold && me->lumber >= lumber && me->tallow >= tallow ? warm : cadv_v4(0.8f, 0.4f, 0.3f, 1), buf);
    snprintf(buf, sizeof buf, "%d/%d", s->stave_item + 1, P05_STAVE_ITEMS);
    cadv_ui_text(ui, px, py + 18, 1, cadv_v4(0.7f, 0.6f, 0.5f, 1), buf);
    for (k = 0; k < 2; ++k) {
        float y = py + 28.0f + (float)k * 5.0f;
        cadv_ui_rect(ui, px, y, 68, 3, cadv_v4(0.15f, 0.1f, 0.07f, 1));
        if (me->queued[k] > 0) {
            const cadv_rts_queue_item *q = &me->queue[k][0];
            int need = (q->building ? cadv_p05_buildings()[q->def].build_ticks : cadv_p05_units()[q->def].build_ticks) * 100;
            cadv_ui_rect(ui, px, y, 68.0f * cadv_minf(1.0f, (float)q->progress / (float)need), 3, k ? cadv_v4(0.5f, 0.75f, 1, 1) : warm);
        }
    }
    if (s->keep_timer > 0) {
        cadv_ui_rect(ui, px, py + 38, 68.0f * (1.0f - (float)s->keep_timer / 900.0f), 2, cadv_v4(0.8f, 0.8f, 0.9f, 1));
    }
    if (me->ready_building >= 0) {
        cadv_ui_rect(ui, px - 1, py + 41, 70, 3, cadv_v4(1, 1, 0.4f, (s->ticks / 15) % 2 ? 1.0f : 0.3f));
    }
    if (s->map_open) {
        minimap(s, ui, cx - (float)P05_W, cy - (float)P05_H, 2);
    } else {
        minimap(s, ui, px + 2, py + 48, 1);
    }
    snprintf(buf, sizeof buf, "HP %d", s->captain >= 0 ? r->u[s->captain].hp : 0);
    cadv_ui_text(ui, 4, (float)ui->height - 12, 1, cadv_v4(1, 0.4f, 0.4f, 1), buf);
    {
        int n = 0;
        for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
            n += s->selected[i] && r->u[i].alive;
        }
        if (n > 0) {
            snprintf(buf, sizeof buf, "SQUAD %d", n);
            cadv_ui_text(ui, 4, (float)ui->height - 22, 1, warm, buf);
        }
        if (s->captain >= 0 && r->u[s->captain].carrier >= 0) {
            cadv_ui_text(ui, 4, (float)ui->height - 32, 1, cadv_v4(0.6f, 0.85f, 1, 1), "ABOARD - JUMP TO GO ASHORE");
        }
    }
    if (s->quip_timer > 0) {
        cadv_ui_text_centered(ui, cx, 26, 1, cadv_v4(1, 0.9f, 0.7f, 1), s->quip);
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx - 30, cy + 22, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, (float)ui->width, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(1, 0.8f, 0.4f, 1), "KINDLEWICK IS WARM!");
        cadv_ui_text_centered(ui, cx, cy + 4, 1, white, "PRESS USE TO RETURN TO THE HALL");
    }
    if (s->lose_timer > 0 || s->captain < 0) {
        cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.5f, 0.1f, 0.1f, 0.3f));
    }
}

static void draw_sprite(const p05_state *s, cadv_scene *sc, const cadv_camera *cam, int def, cadv_vec3 pos, float yaw, float scale,
                        cadv_vec4 tint)
{
    const p05_gpu *g = &s->g;
    cadv_sprite sp;
    int frame = cadv_sprite_frame(pos, yaw, cam->pos, 8);
    memset(&sp, 0, sizeof sp);
    sp.pos = pos;
    sp.w = (float)g->fw[def] / 4.0f * scale;
    sp.h = (float)g->fh[def] / 4.0f * scale;
    sp.tex = g->tex_sprite[def];
    sp.normal_tex = g->tex_normal[def];
    sp.u0 = (float)frame / 8.0f;
    sp.u1 = (float)(frame + 1) / 8.0f;
    sp.v1 = 1.0f;
    sp.tint = tint;
    sp.casts_shadow = true;
    cadv_scene_sprite(sc, &sp);
}

void cadv_p05_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p05_state *s = (p05_state *)state;
    const cadv_rts *r = &s->rts;
    p05_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float t = ((float)s->ticks + alpha) * P05_DT;
    int lights = 0;
    int i;
    if (g->grid_version != r->grid.version) {
        build_forest(s);
    }
    cam->pos = cadv_p05_eye(s);
    cam->yaw = s->yaw;
    cam->pitch = s->pitch;
    cam->fovy = 1.2f;
    cam->znear = 0.1f;
    cam->zfar = 220.0f;
    /* deep winter dusk; spring brings a brighter sky */
    sc->ambient = s->thawed ? cadv_v3(0.34f, 0.34f, 0.40f) : cadv_v3(0.24f, 0.26f, 0.36f);
    sc->sun_dir = cadv_v3_norm(cadv_v3(-0.5f, -0.5f, 0.6f));
    sc->sun_color = s->thawed ? cadv_v3(1.0f, 0.85f, 0.7f) : cadv_v3(0.85f, 0.6f, 0.55f);
    sc->sun_shadow = true;
    sc->shadow_center = cadv_v3(cam->pos.x, 0, cam->pos.z - 20.0f);
    sc->shadow_extent = 45.0f;
    sc->clear_color = s->thawed ? cadv_v3(0.45f, 0.5f, 0.62f) : cadv_v3(0.22f, 0.24f, 0.36f);
    sc->fog_color = s->thawed ? cadv_v3(0.5f, 0.55f, 0.65f) : cadv_v3(0.3f, 0.32f, 0.44f);
    sc->fog_near = 60.0f;
    sc->fog_far = 190.0f;
    sc->light_bands = 6;
    sc->dither = 0.6f;
    sc->use_palette = true;
    cadv_scene_draw(sc, &g->ground, cadv_m4_identity(), cadv_material_make(g->tex_snow, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->lake, cadv_m4_identity(),
                    cadv_material_make(s->thawed ? g->tex_water : g->tex_ice, white, s->thawed ? 0.15f : 0.05f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->rocks, cadv_m4_identity(), cadv_material_make(g->tex_rock, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->tree, cadv_m4_identity(), cadv_material_make(0, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->mine, cadv_m4_translate(cadv_v3(39.0f * P05_CELL, 0, 47.0f * P05_CELL)), cadv_material_make(0, white, 0.1f, 0));
    cadv_scene_draw(sc, &g->platform, cadv_m4_translate(cadv_v3(45.0f * P05_CELL, 0, 34.5f * P05_CELL)), cadv_material_make(0, white, 0.0f, 0));
    cadv_scene_light(sc, cadv_v3(45.0f * P05_CELL, 3.0f, 34.5f * P05_CELL), cadv_v3(1.2f, 0.8f, 0.4f), 8.0f, false);
    ++lights;
    /* structures: own always, others once explored */
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        const cadv_rts_building *b = &r->b[i];
        const cadv_rts_bdef *d;
        float x;
        float z;
        if (!b->alive) {
            continue;
        }
        d = &cadv_p05_buildings()[b->def];
        if (b->owner == P05_TROLLS && !r->explored[P05_GNOMES][b->z * P05_W + b->x]) {
            continue;
        }
        x = ((float)b->x + 0.5f * (float)d->fw) * P05_CELL;
        z = ((float)b->z + 0.5f * (float)d->fh) * P05_CELL;
        cadv_scene_draw(sc, &g->building[b->def], cadv_m4_translate(cadv_v3(x, 0, z)),
                        cadv_material_make(0, side_tint(b->owner, (float)b->hp / (float)d->hp), 0.04f, 0));
        if (lights < 20 && (b->def == P05_B_HALL || b->def == P05_B_KEEP || b->def == P05_B_TAVERN || b->def == P05_B_WORKSHOP)) {
            cadv_scene_light(sc, cadv_v3(x, 1.8f, z + (float)d->fh * P05_CELL * 0.5f + 1.5f), cadv_v3(0.6f, 0.42f, 0.22f), 6.0f, false);
            ++lights;
        }
        if (b->def == P05_B_LONGHALL) {
            /* the ward shimmers until spring; the fire pit burns before the door */
            cadv_vec3 pit = cadv_v3(x, 0, z + (float)d->fh * P05_CELL * 0.5f + 2.5f);
            cadv_scene_draw(sc, &g->fire, cadv_m4_translate(cadv_v3(pit.x - 1.6f, 0, pit.z - 1.6f)), cadv_material_make(0, white, 0.6f, 0));
            if (lights < 24) {
                cadv_scene_light(sc, cadv_v3(pit.x, 1.5f, pit.z), cadv_v3(0.9f, 0.4f, 0.15f), 9.0f, false);
                ++lights;
            }
            if (!s->thawed && lights < 24) {
                float k = 0.6f + 0.3f * cadv_sinf(t * 2.0f);
                cadv_scene_light(sc, cadv_v3(x, 6.0f, z), cadv_v3(0.4f * k, 0.8f * k, 1.6f * k), 16.0f, false);
                ++lights;
            }
        }
        if (b->def == P05_B_TAVERN) {
            cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(x, 2.4f, z + (float)d->fh * P05_CELL * 0.5f + 0.05f)),
                                                        cadv_m4_scale(cadv_v3((float)g->sign_w / 20.0f, 0.6f, 1))),
                            cadv_material_make(g->tex_sign, white, 0.8f, CADV_MAT_NO_SHADOW));
        }
    }
    /* units: own always, others when in sight */
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        const cadv_rts_unit *u = &r->u[i];
        const cadv_rts_udef *d;
        float scale;
        if (!u->alive || i == s->captain || u->carrier >= 0 || (u->owner != P05_GNOMES && !cadv_rts_cell_visible(r, P05_GNOMES, u->x, u->z))) {
            continue;
        }
        d = &cadv_p05_units()[u->def];
        scale = !d->infantry ? 0.2f : (u->owner == P05_TROLLS ? 0.1f : 0.075f);
        draw_sprite(s, sc, cam, u->def, cadv_v3(u->x, u->def == P05_U_FERRY ? -0.2f : 0.0f, u->z), u->yaw, scale,
                    side_tint(u->owner, (float)u->hp / (float)d->hp));
        if (u->owner == P05_TROLLS && s->raider[i] && lights < 30) {
            /* raiders carry torches across the ice */
            float f = 0.85f + 0.15f * cadv_sinf(t * 9.0f + (float)i);
            cadv_scene_light(sc, cadv_v3(u->x, 2.2f, u->z), cadv_v3(1.6f * f, 0.7f * f, 0.25f * f), 7.0f, false);
            ++lights;
        }
        if (s->selected[i]) {
            float rr = d->infantry ? 0.6f : (u->def == P05_U_FERRY ? 2.4f : 1.4f);
            cadv_scene_draw(sc, &g->ring, cadv_m4_mul(cadv_m4_translate(cadv_v3(u->x, 0, u->z)), cadv_m4_scale(cadv_v3(rr, 1, rr))),
                            cadv_material_make(0, cadv_v4(1, 0.85f, 0.3f, 1), 0.8f, CADV_MAT_NO_SHADOW | CADV_MAT_UNLIT));
        }
    }
    /* arrows, axes and bolts */
    for (i = 0; i < CADV_RTS_SHOTS; ++i) {
        const cadv_rts_shot *sh = &r->shots[i];
        if (sh->ttl > 0) {
            cadv_vec3 c = sh->owner == P05_GNOMES ? cadv_v3(1.4f, 1.2f, 0.8f) : cadv_v3(1.4f, 0.6f, 0.3f);
            cadv_scene_draw(sc, &g->cube, segment(sh->x0, sh->z0, sh->x1, sh->z1, 1.0f, 0.06f),
                            cadv_material_make(0, cadv_v4(c.x, c.y, c.z, 1), 0.8f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
        }
    }
    /* the snow-owl scout over the lake */
    {
        cadv_vec3 o = cadv_p05_owl(s);
        cadv_sprite sp;
        memset(&sp, 0, sizeof sp);
        sp.pos = o;
        sp.w = (float)g->owl_w / 4.0f * 0.25f;
        sp.h = (float)g->owl_h / 4.0f * 0.25f;
        sp.tex = g->tex_owl;
        sp.u1 = 1.0f;
        sp.v1 = 1.0f;
        sp.tint = white;
        sp.emissive = 0.3f;
        cadv_scene_sprite(sc, &sp);
    }
    /* the placement outline */
    if (s->placing && r->side[P05_GNOMES].ready_building >= 0 && s->captain >= 0) {
        const cadv_rts_bdef *d = &cadv_p05_buildings()[r->side[P05_GNOMES].ready_building];
        float gx;
        float gz;
        if (cadv_p05_aim_ground(s, &gx, &gz)) {
            cadv_cell c = cadv_rts_cell_at(&r->grid, gx, gz);
            int x0 = c.x - d->fw / 2;
            int z0 = c.z - d->fh / 2;
            bool yard = r->side[P05_GNOMES].ready_building == P05_B_SHIPYARD;
            bool ok = cadv_rts_can_place(&r->grid, x0, z0, d->fw, d->fh, P05_GNOMES, yard ? 0 : 3, yard) == CADV_PLACE_OK;
            cadv_vec4 col = ok ? cadv_v4(1.2f, 1.0f, 0.4f, 1) : cadv_v4(1.4f, 0.3f, 0.3f, 1);
            float ax = (float)x0 * P05_CELL;
            float az = (float)z0 * P05_CELL;
            float bx = (float)(x0 + d->fw) * P05_CELL;
            float bz = (float)(z0 + d->fh) * P05_CELL;
            uint32_t f = CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW;
            cadv_scene_draw(sc, &g->cube, segment(ax, az, bx, az, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
            cadv_scene_draw(sc, &g->cube, segment(ax, bz, bx, bz, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
            cadv_scene_draw(sc, &g->cube, segment(ax, az, ax, bz, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
            cadv_scene_draw(sc, &g->cube, segment(bx, az, bx, bz, 0.3f, 0.15f), cadv_material_make(0, col, 1.0f, f));
        }
    }
    if (s->star_state == 1 || s->star_state == 2) {
        cadv_vec3 p = cadv_v3(s->star_x, 2.0f + 0.2f * cadv_sinf(t * 2.0f), s->star_z);
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(t * 1.5f)), cadv_material_make(0, white, 1.2f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, p, cadv_v3(1.0f, 0.85f, 0.3f), 9.0f, false);
    }
    /* the Captain's command stave and shield at the edges of the view */
    if (s->captain >= 0) {
        cadv_vec3 e = cam->pos;
        cadv_mat4 body = cadv_m4_mul(cadv_m4_translate(e), cadv_m4_mul(cadv_m4_rotate_y(s->yaw), cadv_m4_rotate_x(s->pitch * 0.5f)));
        float bash = s->bash_cd > 0.6f ? (s->bash_cd - 0.6f) * 1.2f : 0.0f;
        uint32_t f = CADV_MAT_NO_SHADOW;
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.30f, -0.42f, -0.62f)), cadv_m4_scale(cadv_v3(0.025f, 0.42f, 0.025f)))),
                        cadv_material_make(0, cadv_v4(0.55f, 0.38f, 0.22f, 1), 0.0f, f));
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.30f, -0.19f, -0.62f)), cadv_m4_scale(cadv_v3(0.035f, 0.035f, 0.035f)))),
                        cadv_material_make(0, cadv_v4(1.0f, 0.75f, 0.35f, 1), 0.5f, f));
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.40f, -0.40f, -0.6f - bash)), cadv_m4_scale(cadv_v3(0.13f, 0.16f, 0.025f)))),
                        cadv_material_make(0, cadv_v4(0.7f, 0.22f, 0.18f, 1), 0.0f, f));
        cadv_scene_draw(sc, &g->cube, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.40f, -0.40f, -0.585f - bash)), cadv_m4_scale(cadv_v3(0.035f, 0.035f, 0.015f)))),
                        cadv_material_make(0, cadv_v4(0.9f, 0.8f, 0.5f, 1), 0.2f, f));
    }
    if (ui) {
        draw_hud(s, ui);
    }
}

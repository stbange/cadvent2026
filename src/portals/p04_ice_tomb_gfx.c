/* SPDX-License-Identifier: MIT */
/* GPU side of Day 4: a grid-readable tomb (one texture repeat per tile),
 * torch light with shadows, cold blue fog, water, and the frozen Colossus. */
#include "portals/p04_ice_tomb_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portals/p04_internal.h"

static const cadv_vec3 k_torches[] = {
    {11.0f, 7.0f, 92.0f}, {19.5f, 7.0f, 84.0f}, {11.0f, 8.5f, 76.0f}, {7.0f, 9.0f, 45.0f}, {25.5f, 9.0f, 64.5f},
    {7.0f, 9.0f, 64.5f}, {25.5f, 9.0f, 45.0f}, {14.3f, 8.5f, 37.0f}, {5.0f, 8.0f, 34.5f}, {29.5f, 8.0f, 34.5f},
    {32.3f, 9.0f, 40.0f}, {37.7f, 9.0f, 20.0f}, {32.3f, 9.0f, 8.0f}, {34.5f, 10.0f, 46.5f}, {2.5f, 8.5f, 51.5f},
};
#define TORCHES ((int)(sizeof k_torches / sizeof k_torches[0]))

static unsigned make_tex(cadv_tex_kind kind, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, kind, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

static unsigned text_tex(const char *text, int *w_out, cadv_rgba fg, cadv_rgba bg)
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

/* ---- tile meshing: faces of solid tiles that touch air or water ----------------------- */

static void face(cadv_mesh *m, cadv_vec3 o, int axis, int side)
{
    cadv_vec3 a;
    cadv_vec3 b;
    cadv_vec3 c;
    cadv_vec3 d;
    cadv_rgba w = cadv_rgba_make(255, 255, 255, 255);
    float s = side > 0 ? 1.0f : 0.0f;
    if (axis == 0) {
        float x = o.x + s;
        a = cadv_v3(x, o.y, o.z);
        b = cadv_v3(x, o.y, o.z + 1);
        c = cadv_v3(x, o.y + 1, o.z + 1);
        d = cadv_v3(x, o.y + 1, o.z);
    } else if (axis == 1) {
        float y = o.y + s;
        a = cadv_v3(o.x, y, o.z);
        b = cadv_v3(o.x + 1, y, o.z);
        c = cadv_v3(o.x + 1, y, o.z + 1);
        d = cadv_v3(o.x, y, o.z + 1);
    } else {
        float z = o.z + s;
        a = cadv_v3(o.x, o.y, z);
        b = cadv_v3(o.x, o.y + 1, z);
        c = cadv_v3(o.x + 1, o.y + 1, z);
        d = cadv_v3(o.x + 1, o.y, z);
    }
    /* winding: counter-clockwise seen from outside */
    if ((side > 0) == (axis == 1)) {
        cadv_mesh_quad(m, a, b, c, d, 1.0f, 1.0f, w);
    } else {
        cadv_mesh_quad(m, d, c, b, a, 1.0f, 1.0f, w);
    }
}

static void rebuild_tiles(p04_state *s)
{
    const cadv_tiles *t = &s->tiles;
    cadv_mesh m[CADV_TILE_KINDS];
    int k;
    int x;
    int y;
    int z;
    static const int nb[6][3] = {{-1, 0, 0}, {1, 0, 0}, {0, -1, 0}, {0, 1, 0}, {0, 0, -1}, {0, 0, 1}};
    for (k = 0; k < CADV_TILE_KINDS; ++k) {
        cadv_mesh_init(&m[k]);
    }
    for (y = 0; y < t->h; ++y) {
        for (z = 0; z < t->d; ++z) {
            for (x = 0; x < t->w; ++x) {
                cadv_tile c = cadv_tiles_get(t, x, y, z);
                int f;
                if (c == CADV_TILE_AIR) {
                    continue;
                }
                for (f = 0; f < 6; ++f) {
                    cadv_tile o = cadv_tiles_get(t, x + nb[f][0], y + nb[f][1], z + nb[f][2]);
                    bool show = c == CADV_TILE_WATER ? (o == CADV_TILE_AIR && f == 3) : !cadv_tile_solid(o);
                    if (show && !(y + nb[f][1] < 0)) {
                        face(&m[c], cadv_v3((float)x, (float)y, (float)z), f / 2, (f % 2) ? 1 : -1);
                    }
                }
            }
        }
    }
    for (k = 1; k < CADV_TILE_KINDS; ++k) {
        cadv_gpu_mesh_upload(&s->g.tiles[k], &m[k], true);
        cadv_mesh_free(&m[k]);
    }
    cadv_mesh_free(&m[0]);
    s->g.tiles_version = t->version;
}

/* ---- models --------------------------------------------------------------------------- */

static void make_enemy(cadv_voxels *v, int type)
{
    switch (type) {
    case P04_WOLF:
        cadv_voxels_alloc(v, 8, 10, 16);
        v->pal[1] = cadv_rgba_make(150, 150, 165, 255);
        v->pal[2] = cadv_rgba_make(90, 90, 100, 255);
        v->pal[3] = cadv_rgba_make(255, 220, 90, 255);
        block(v, 1, 0, 3, 2, 3, 4, 2);
        block(v, 5, 0, 3, 6, 3, 4, 2);
        block(v, 1, 0, 11, 2, 3, 12, 2);
        block(v, 5, 0, 11, 6, 3, 12, 2);
        block(v, 1, 4, 2, 6, 7, 13, 1);
        block(v, 2, 6, 13, 5, 9, 15, 1);
        cadv_voxels_set(v, 2, 8, 15, 3);
        cadv_voxels_set(v, 5, 8, 15, 3);
        block(v, 3, 6, 0, 4, 7, 1, 2);
        break;
    case P04_BAT:
        cadv_voxels_alloc(v, 16, 6, 6);
        v->pal[1] = cadv_rgba_make(150, 200, 240, 255);
        v->pal[2] = cadv_rgba_make(60, 80, 120, 255);
        block(v, 6, 1, 1, 9, 4, 4, 2);
        block(v, 0, 3, 2, 5, 3, 4, 1);
        block(v, 10, 3, 2, 15, 3, 4, 1);
        break;
    default: /* frost bear */
        cadv_voxels_alloc(v, 12, 14, 20);
        v->pal[1] = cadv_rgba_make(235, 240, 250, 255);
        v->pal[2] = cadv_rgba_make(120, 130, 150, 255);
        v->pal[3] = cadv_rgba_make(30, 30, 40, 255);
        block(v, 1, 0, 3, 3, 4, 6, 2);
        block(v, 8, 0, 3, 10, 4, 6, 2);
        block(v, 1, 0, 13, 3, 4, 16, 2);
        block(v, 8, 0, 13, 10, 4, 16, 2);
        block(v, 0, 5, 2, 11, 11, 17, 1);
        block(v, 3, 8, 17, 8, 13, 19, 1);
        cadv_voxels_set(v, 4, 11, 19, 3);
        cadv_voxels_set(v, 7, 11, 19, 3);
        break;
    }
}

void cadv_p04_gpu_init(void *state)
{
    p04_state *s = (p04_state *)state;
    p04_gpu *g = &s->g;
    cadv_voxels v;
    cadv_mesh m;
    int i;
    int x;
    int y;
    memset(g, 0, sizeof *g);
    g->tex_rock = make_tex(CADV_TEX_STONE_BRICKS, 41u, cadv_rgba_make(120, 125, 140, 255), cadv_rgba_make(70, 75, 90, 255));
    g->tex_ice = make_tex(CADV_TEX_ICE, 42u, cadv_rgba_make(140, 200, 240, 255), cadv_rgba_make(220, 245, 255, 255));
    g->tex_water = make_tex(CADV_TEX_ICE, 43u, cadv_rgba_make(30, 80, 120, 255), cadv_rgba_make(70, 140, 180, 255));
    g->tex_block = make_tex(CADV_TEX_STONE_BRICKS, 44u, cadv_rgba_make(170, 150, 110, 255), cadv_rgba_make(110, 90, 60, 255));
    g->tex_door = make_tex(CADV_TEX_METAL, 45u, cadv_rgba_make(160, 130, 60, 255), cadv_rgba_make(230, 200, 110, 255));
    rebuild_tiles(s);
    cadv_mesh_init(&m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, cadv_rgba_make(255, 255, 255, 255), false);
    cadv_gpu_mesh_upload(&g->cube, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->plaque, &m, false);
    cadv_mesh_clear(&m);
    /* lever: a base and a handle (drawn tilted when pulled) */
    cadv_mesh_box(&m, cadv_v3(-0.06f, 0.0f, -0.06f), cadv_v3(0.06f, 0.9f, 0.06f), 0.0f, cadv_rgba_make(200, 170, 90, 255), false);
    cadv_mesh_box(&m, cadv_v3(-0.12f, 0.8f, -0.12f), cadv_v3(0.12f, 1.0f, 0.12f), 0.0f, cadv_rgba_make(220, 60, 50, 255), false);
    cadv_gpu_mesh_upload(&g->lever, &m, false);
    cadv_mesh_clear(&m);
    /* the blade: a long curved sliver of bronze */
    cadv_mesh_box(&m, cadv_v3(-0.6f, 0.0f, -0.04f), cadv_v3(0.6f, 0.25f, 0.04f), 0.0f, cadv_rgba_make(210, 180, 120, 255), false);
    cadv_mesh_box(&m, cadv_v3(-0.04f, 0.25f, -0.04f), cadv_v3(0.04f, 3.0f, 0.04f), 0.0f, cadv_rgba_make(90, 80, 70, 255), false);
    cadv_gpu_mesh_upload(&g->blade, &m, false);
    cadv_mesh_clear(&m);
    /* solar door: a great disc with three sockets */
    for (i = 0; i < 24; ++i) {
        float a0 = (float)i / 24.0f * 2.0f * CADV_PI;
        float a1 = (float)(i + 1) / 24.0f * 2.0f * CADV_PI;
        uint32_t c0 = cadv_mesh_vertex(&m, cadv_v3(0, 2.4f, 0), cadv_v3(0, 0, -1), 0.5f, 0.5f, cadv_rgba_make(255, 220, 140, 255));
        uint32_t c1 = cadv_mesh_vertex(&m, cadv_v3(2.2f * cadv_cosf(a0), 2.4f + 2.2f * cadv_sinf(a0), 0), cadv_v3(0, 0, -1), 0, 0,
                                       cadv_rgba_make(160, 120, 60, 255));
        uint32_t c2 = cadv_mesh_vertex(&m, cadv_v3(2.2f * cadv_cosf(a1), 2.4f + 2.2f * cadv_sinf(a1), 0), cadv_v3(0, 0, -1), 1, 0,
                                       cadv_rgba_make(160, 120, 60, 255));
        cadv_mesh_tri(&m, c0, c2, c1);
    }
    cadv_gpu_mesh_upload(&g->door, &m, false);
    cadv_mesh_clear(&m);
    /* hand with a pistol */
    cadv_mesh_box(&m, cadv_v3(-0.05f, -0.05f, -0.05f), cadv_v3(0.05f, 0.05f, 0.08f), 0.0f, cadv_rgba_make(225, 185, 150, 255), false);
    cadv_mesh_box(&m, cadv_v3(-0.03f, 0.0f, -0.28f), cadv_v3(0.03f, 0.06f, -0.02f), 0.0f, cadv_rgba_make(50, 50, 60, 255), false);
    cadv_mesh_box(&m, cadv_v3(-0.06f, -0.06f, 0.08f), cadv_v3(0.06f, 0.06f, 0.3f), 0.0f, cadv_rgba_make(70, 90, 110, 255), false);
    cadv_gpu_mesh_upload(&g->hand, &m, false);
    cadv_mesh_free(&m);
    /* sun gear */
    cadv_voxels_alloc(&v, 11, 11, 3);
    v.pal[1] = cadv_rgba_make(255, 200, 70, 255);
    v.pal[2] = cadv_rgba_make(255, 240, 160, 255);
    for (y = 0; y < 11; ++y) {
        for (x = 0; x < 11; ++x) {
            float px = (float)x - 5.0f;
            float py = (float)y - 5.0f;
            float r = cadv_sqrtf(px * px + py * py);
            float a = cadv_atan2f(py, px);
            float tooth = cadv_cosf(a * 8.0f) > 0.3f ? 5.4f : 4.4f;
            if (r <= tooth && r >= 1.2f) {
                block(&v, x, y, 0, x, y, 2, r < 2.5f ? 2 : 1);
            }
        }
    }
    upload_voxels(&g->gear, &v, 0.07f);
    /* jade ornament: a tiny coiled dragon */
    cadv_voxels_alloc(&v, 6, 6, 4);
    v.pal[1] = cadv_rgba_make(90, 220, 140, 255);
    block(&v, 1, 0, 1, 4, 1, 2, 1);
    block(&v, 0, 1, 1, 1, 4, 2, 1);
    block(&v, 1, 4, 1, 4, 5, 2, 1);
    block(&v, 4, 2, 0, 5, 3, 3, 1);
    upload_voxels(&g->jade, &v, 0.07f);
    /* save crystal */
    cadv_voxels_alloc(&v, 5, 12, 5);
    v.pal[1] = cadv_rgba_make(120, 200, 255, 255);
    block(&v, 2, 0, 2, 2, 11, 2, 1);
    block(&v, 1, 2, 1, 3, 9, 3, 1);
    block(&v, 0, 4, 2, 4, 7, 2, 1);
    block(&v, 2, 4, 0, 2, 7, 4, 1);
    upload_voxels(&g->crystal, &v, 0.1f);
    /* the Shining Star */
    cadv_voxels_alloc(&v, 15, 15, 3);
    v.pal[1] = cadv_rgba_make(255, 225, 100, 255);
    for (y = 0; y < 15; ++y) {
        for (x = 0; x < 15; ++x) {
            float px = (float)x - 7.0f;
            float py = (float)y - 7.0f;
            float a = cadv_atan2f(py, px) + 0.5f * CADV_PI;
            float r = cadv_sqrtf(px * px + py * py);
            float k = cadv_cosf(5.0f * a);
            if (r <= 3.4f + 3.6f * (k > 0.0f ? k * k : 0.0f)) {
                block(&v, x, y, 0, x, y, 2, 1);
            }
        }
    }
    upload_voxels(&g->star, &v, 0.06f);
    /* the frozen Colossus: a giant of ice and stone */
    cadv_voxels_alloc(&v, 16, 26, 12);
    v.pal[1] = cadv_rgba_make(170, 220, 250, 255);
    v.pal[2] = cadv_rgba_make(90, 100, 120, 255);
    v.pal[3] = cadv_rgba_make(120, 255, 255, 255);
    block(&v, 3, 0, 4, 6, 9, 8, 2);
    block(&v, 9, 0, 4, 12, 9, 8, 2);
    block(&v, 2, 10, 2, 13, 19, 9, 1);
    block(&v, 0, 12, 4, 1, 21, 7, 1);
    block(&v, 14, 12, 4, 15, 21, 7, 1);
    block(&v, 5, 20, 3, 10, 25, 8, 1);
    block(&v, 6, 23, 9, 7, 23, 9, 3);
    block(&v, 8, 23, 9, 9, 23, 9, 3);
    upload_voxels(&g->colossus, &v, 0.32f);
    for (i = 0; i < P04_COLOSSUS; ++i) {
        cadv_texdata c;
        cadv_texdata n;
        make_enemy(&v, i);
        cadv_voxels_bake_sprites(&v, 8, 4, &c, &n, &g->fw[i], &g->fh[i]);
        g->tex_sprite[i] = cadv_gl_texture(&c, false);
        g->tex_normal[i] = cadv_gl_texture(&n, false);
        cadv_texdata_free(&c);
        cadv_texdata_free(&n);
        cadv_voxels_free(&v);
    }
    for (i = 0; i < P04_PLAQUES; ++i) {
        g->tex_plaque[i] = text_tex(s->mark_text[i], &g->plaque_w[i], i == P04_PLAQUE_JOURNAL ? cadv_rgba_make(60, 40, 20, 255) : cadv_rgba_make(200, 230, 255, 255),
                                    i == P04_PLAQUE_JOURNAL ? cadv_rgba_make(230, 215, 180, 255) : cadv_rgba_make(60, 70, 90, 255));
    }
    s->gpu_ready = true;
}

void cadv_p04_gpu_free(void *state)
{
    p04_state *s = (p04_state *)state;
    p04_gpu *g = &s->g;
    int i;
    for (i = 0; i < CADV_TILE_KINDS; ++i) {
        cadv_gpu_mesh_free(&g->tiles[i]);
    }
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->gear);
    cadv_gpu_mesh_free(&g->jade);
    cadv_gpu_mesh_free(&g->crystal);
    cadv_gpu_mesh_free(&g->lever);
    cadv_gpu_mesh_free(&g->blade);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->door);
    cadv_gpu_mesh_free(&g->hand);
    cadv_gpu_mesh_free(&g->colossus);
    cadv_gl_delete_textures(1, &g->tex_rock);
    cadv_gl_delete_textures(1, &g->tex_ice);
    cadv_gl_delete_textures(1, &g->tex_water);
    cadv_gl_delete_textures(1, &g->tex_block);
    cadv_gl_delete_textures(1, &g->tex_door);
    cadv_gl_delete_textures(P04_PLAQUES, g->tex_plaque);
    cadv_gl_delete_textures(P04_COLOSSUS, g->tex_sprite);
    cadv_gl_delete_textures(P04_COLOSSUS, g->tex_normal);
    s->gpu_ready = false;
}

/* ---- draw --------------------------------------------------------------------------------- */

static void draw_hud(const p04_state *s, cadv_ui *ui, bool underwater)
{
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float cx = (float)ui->width * 0.5f;
    float cy = (float)ui->height * 0.5f;
    char buf[64];
    int i;
    cadv_ui_rect(ui, cx - 1, cy - 1, 2, 2, white);
    /* health and breath */
    cadv_ui_rect(ui, 4, (float)ui->height - 10, 62, 6, cadv_v4(0, 0, 0, 0.6f));
    cadv_ui_rect(ui, 5, (float)ui->height - 9, 60.0f * (float)s->hp / (float)P04_HP, 4, cadv_v4(0.9f, 0.25f, 0.25f, 1));
    if (underwater || s->me.air < s->cdef.air_max - 0.1f) {
        cadv_ui_rect(ui, 4, (float)ui->height - 18, 62, 6, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_rect(ui, 5, (float)ui->height - 17, 60.0f * s->me.air / s->cdef.air_max, 4, cadv_v4(0.3f, 0.7f, 1, 1));
    }
    /* the three Sun Gears, jade and crystals */
    for (i = 0; i < 3; ++i) {
        bool have = (s->gear_bits & (1u << (unsigned)i)) != 0;
        cadv_ui_rect(ui, (float)ui->width - 40.0f + (float)i * 12.0f, 4, 9, 9, have ? cadv_v4(1, 0.8f, 0.3f, 1) : cadv_v4(0.25f, 0.25f, 0.3f, 0.8f));
    }
    snprintf(buf, sizeof buf, "JADE %d/3", s->jade);
    cadv_ui_text(ui, (float)ui->width - 6.0f * (float)strlen(buf) - 4, 16, 1, cadv_v4(0.5f, 1, 0.7f, 1), buf);
    snprintf(buf, sizeof buf, "SAVES %d", s->crystal_uses);
    cadv_ui_text(ui, (float)ui->width - 6.0f * (float)strlen(buf) - 4, 26, 1, cadv_v4(0.6f, 0.8f, 1, 1), buf);
    if (s->door_timer > 0 && !s->door_jammed) {
        snprintf(buf, sizeof buf, "DOOR %d", s->door_timer / 60 + 1);
        cadv_ui_text_centered(ui, cx, 6, 1, cadv_v4(1, 0.8f, 0.4f, 1), buf);
    }
    if (s->chase) {
        cadv_ui_text_centered(ui, cx, 6, 1, (s->ticks / 10) % 2 ? cadv_v4(1, 0.3f, 0.3f, 1) : white, "RUN!");
    }
    if (s->quip_timer > 0) {
        cadv_ui_text_centered(ui, cx, (float)ui->height - 30, 1, cadv_v4(0.8f, 0.95f, 1, 1), s->quip);
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx, cy + 22, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, (float)ui->width, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(1, 0.85f, 0.4f, 1), "SPRING RETURNS!");
        cadv_ui_text_centered(ui, cx, cy + 4, 1, white, "PRESS USE TO RETURN TO THE HALL");
    }
    if (s->hurt_flash > 0) {
        cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.6f, 0.1f, 0.1f, 0.3f));
    }
    if (underwater) {
        cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.05f, 0.2f, 0.35f, 0.25f));
    }
}

void cadv_p04_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p04_state *s = (p04_state *)state;
    p04_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float t = ((float)s->ticks + alpha) * P04_DT;
    cadv_vec3 eye = cadv_climber_eye(&s->me, &s->cdef);
    bool underwater = cadv_tiles_at(&s->tiles, eye) == CADV_TILE_WATER;
    int lights = 0;
    int i;
    int nearest = -1;
    float nd = 1e9f;
    if (g->tiles_version != s->tiles.version) {
        rebuild_tiles(s);
    }
    cam->pos = eye;
    cam->yaw = s->me.yaw;
    cam->pitch = s->me.pitch;
    cam->fovy = 1.25f;
    cam->znear = 0.05f;
    cam->zfar = 90.0f;
    sc->ambient = cadv_v3(0.16f, 0.19f, 0.26f);
    sc->clear_color = cadv_v3(0.02f, 0.03f, 0.06f);
    sc->fog_color = underwater ? cadv_v3(0.05f, 0.2f, 0.3f) : cadv_v3(0.06f, 0.09f, 0.15f);
    sc->fog_near = underwater ? 1.0f : 10.0f;
    sc->fog_far = underwater ? 14.0f : 45.0f;
    sc->light_bands = 6;
    sc->dither = 0.6f;
    sc->use_palette = true;
    cadv_scene_draw(sc, &g->tiles[CADV_TILE_ROCK], cadv_m4_identity(), cadv_material_make(g->tex_rock, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->tiles[CADV_TILE_ICE], cadv_m4_identity(), cadv_material_make(g->tex_ice, white, 0.25f, 0));
    cadv_scene_draw(sc, &g->tiles[CADV_TILE_BLOCK], cadv_m4_identity(), cadv_material_make(g->tex_block, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->tiles[CADV_TILE_DOOR], cadv_m4_identity(), cadv_material_make(g->tex_door, white, 0.05f, 0));
    cadv_scene_draw(sc, &g->tiles[CADV_TILE_GRATE], cadv_m4_identity(), cadv_material_make(g->tex_door, cadv_v4(0.5f, 0.5f, 0.6f, 1), 0.0f, 0));
    cadv_scene_draw(sc, &g->tiles[CADV_TILE_WATER], cadv_m4_translate(cadv_v3(0, -0.08f + 0.03f * cadv_sinf(t * 1.3f), 0)),
                    cadv_material_make(g->tex_water, cadv_v4(0.7f, 0.9f, 1.0f, 1), 0.35f, CADV_MAT_NO_SHADOW));
    /* torches: the nearest one throws shadows */
    for (i = 0; i < TORCHES; ++i) {
        float d = cadv_v3_dist(k_torches[i], eye);
        if (d < nd) {
            nd = d;
            nearest = i;
        }
    }
    for (i = 0; i < TORCHES; ++i) {
        if (cadv_v3_dist(k_torches[i], eye) < 34.0f && lights < 14) {
            float flick = 1.0f + 0.12f * cadv_sinf(t * 9.0f + (float)i * 2.1f);
            cadv_scene_light(sc, k_torches[i], cadv_v3(1.5f * flick, 0.95f * flick, 0.5f * flick), 11.0f, i == nearest);
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(k_torches[i]), cadv_m4_scale(cadv_v3(0.15f, 0.3f, 0.15f))),
                            cadv_material_make(0, cadv_v4(1.8f, 1.1f, 0.4f, 1), 1.5f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            ++lights;
        }
    }
    /* levers, crystals, the blade, plaques, the solar door */
    for (i = 0; i < P04_LEVERS; ++i) {
        cadv_vec3 p = cadv_p04_lever_pos(i);
        cadv_scene_draw(sc, &g->lever, cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_x(s->lever[i] ? 0.7f : -0.7f)),
                        cadv_material_make(0, white, 0.1f, 0));
    }
    for (i = 0; i < P04_CRYSTALS; ++i) {
        cadv_vec3 p = cadv_p04_crystal_pos(i);
        float glow = s->crystal_used[i] ? 1.2f : 0.6f;
        cadv_scene_draw(sc, &g->crystal, cadv_m4_mul(cadv_m4_translate(cadv_v3(p.x, p.y + 0.1f * cadv_sinf(t * 2.0f), p.z)), cadv_m4_rotate_y(t)),
                        cadv_material_make(0, white, glow, CADV_MAT_NO_SHADOW));
        if (lights < 20 && cadv_v3_dist(p, eye) < 20.0f) {
            cadv_scene_light(sc, cadv_v3_add(p, cadv_v3(0, 0.8f, 0)), cadv_v3(0.3f * glow, 0.6f * glow, 1.0f * glow), 6.0f, false);
            ++lights;
        }
    }
    {
        cadv_vec3 b = cadv_p04_blade_pos(s);
        float swing = cadv_sinf(t * 2.0f * CADV_PI / 2.4f + 0.5f * CADV_PI) * 0.5f;
        cadv_scene_draw(sc, &g->blade, cadv_m4_mul(cadv_m4_translate(cadv_v3(b.x, 6.6f, b.z)), cadv_m4_rotate_x(swing * 0.2f)),
                        cadv_material_make(0, white, 0.1f, 0));
    }
    for (i = 0; i < P04_PLAQUES; ++i) {
        float yaw;
        cadv_vec3 p = cadv_p04_plaque_pos(i, &yaw);
        float w = (float)g->plaque_w[i] / 30.0f;
        cadv_scene_draw(sc, &g->plaque, cadv_m4_mul(cadv_m4_translate(cadv_v3(p.x, p.y - 0.2f, p.z)),
                                                    cadv_m4_mul(cadv_m4_rotate_y(yaw), cadv_m4_scale(cadv_v3(w, w * 12.0f / (float)g->plaque_w[i], 1)))),
                        cadv_material_make(g->tex_plaque[i], white, 0.4f, CADV_MAT_NO_SHADOW));
    }
    {
        cadv_vec3 d = cadv_p04_solar_door();
        cadv_scene_draw(sc, &g->door, cadv_m4_translate(cadv_v3(d.x, d.y, 52.95f)),
                        cadv_material_make(0, white, s->gears_set ? 1.0f : 0.15f, CADV_MAT_NO_SHADOW));
        for (i = 0; i < 3; ++i) {
            if (s->gears_set) {
                float a = (float)i / 3.0f * 2.0f * CADV_PI + t;
                cadv_scene_draw(sc, &g->gear, cadv_m4_mul(cadv_m4_translate(cadv_v3(d.x + 1.3f * cadv_cosf(a), d.y + 2.4f + 1.3f * cadv_sinf(a), 52.85f)),
                                                          cadv_m4_rotate_y(CADV_PI)),
                                cadv_material_make(0, white, 0.8f, CADV_MAT_NO_SHADOW));
            }
        }
        if (s->gears_set) {
            cadv_scene_light(sc, cadv_v3(d.x, d.y + 3.0f, 51.0f), cadv_v3(2.0f, 1.7f, 0.9f), 14.0f, false);
        }
    }
    /* pickups */
    for (i = 0; i < s->npickups; ++i) {
        const p04_pickup *p = &s->pickups[i];
        cadv_mat4 xf;
        if (!p->active) {
            continue;
        }
        xf = cadv_m4_mul(cadv_m4_translate(cadv_v3(p->pos.x, p->pos.y + 0.4f + 0.08f * cadv_sinf(t * 2.0f + (float)i), p->pos.z)), cadv_m4_rotate_y(t * 1.4f));
        if (p->kind == P04_PK_GEAR) {
            cadv_scene_draw(sc, &g->gear, xf, cadv_material_make(0, white, 0.8f, CADV_MAT_NO_SHADOW));
            if (lights < 24) {
                cadv_scene_light(sc, cadv_v3_add(p->pos, cadv_v3(0, 0.6f, 0)), cadv_v3(1.6f, 1.2f, 0.4f), 5.0f, false);
                ++lights;
            }
        } else if (p->kind == P04_PK_JADE) {
            cadv_scene_draw(sc, &g->jade, xf, cadv_material_make(0, white, 0.6f, CADV_MAT_NO_SHADOW));
        } else {
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(xf, cadv_m4_scale(cadv_v3(0.35f, 0.25f, 0.25f))), cadv_material_make(0, cadv_v4(0.95f, 0.95f, 0.95f, 1), 0.3f, 0));
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(xf, cadv_m4_scale(cadv_v3(0.37f, 0.08f, 0.08f))), cadv_material_make(0, cadv_v4(1, 0.2f, 0.2f, 1), 0.6f, 0));
        }
    }
    /* creatures and the Colossus */
    for (i = 0; i < P04_MAX_ENEMIES; ++i) {
        const p04_enemy *e = &s->enemies[i];
        if (!e->active) {
            continue;
        }
        if (e->type == P04_COLOSSUS) {
            cadv_scene_draw(sc, &g->colossus, cadv_m4_mul(cadv_m4_translate(e->pos), cadv_m4_rotate_y(e->yaw)),
                            cadv_material_make(0, white, s->chase ? 0.2f : 0.05f, 0));
            if (s->chase && lights < 28) {
                cadv_scene_light(sc, cadv_v3_add(e->pos, cadv_v3(0, 7.5f, 1.0f)), cadv_v3(0.4f, 1.4f, 1.6f), 9.0f, false);
                ++lights;
            }
        } else {
            cadv_sprite sp;
            float scale = e->type == P04_BEAR ? 0.14f : 0.1f;
            int frame = cadv_sprite_frame(e->pos, e->yaw, eye, 8);
            memset(&sp, 0, sizeof sp);
            sp.pos = e->pos;
            sp.w = (float)g->fw[e->type] / 4.0f * scale;
            sp.h = (float)g->fh[e->type] / 4.0f * scale;
            sp.tex = g->tex_sprite[e->type];
            sp.normal_tex = g->tex_normal[e->type];
            sp.u0 = (float)frame / 8.0f;
            sp.u1 = (float)(frame + 1) / 8.0f;
            sp.v1 = 1.0f;
            sp.tint = e->hurt > 0 ? cadv_v4(1, 0.5f, 0.5f, 1) : white;
            sp.casts_shadow = true;
            cadv_scene_sprite(sc, &sp);
        }
    }
    if (s->star_active) {
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(s->star_pos), cadv_m4_rotate_y(t * 1.5f)), cadv_material_make(0, white, 1.2f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, s->star_pos, cadv_v3(2.0f, 1.7f, 0.6f), 10.0f, false);
    }
    /* Mara's hands: pistols out on the ground, reaching up while hanging */
    {
        cadv_mat4 body = cadv_m4_mul(cadv_m4_translate(eye), cadv_m4_mul(cadv_m4_rotate_y(s->me.yaw), cadv_m4_rotate_x(s->me.pitch)));
        bool reach = s->me.state == CADV_CLIMB_HANG || s->me.state == CADV_CLIMB_PULLUP;
        float kick = s->muzzle > 0 ? 0.04f : 0.0f;
        for (i = 0; i < 2; ++i) {
            float side = i == 0 ? 0.2f : -0.2f;
            cadv_vec3 off = reach ? cadv_v3(side, 0.12f, -0.5f) : cadv_v3(side, -0.17f + (s->gun_side == i ? kick : 0.0f), -0.55f);
            cadv_scene_draw(sc, &g->hand, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(off), cadv_m4_scale(cadv_v3(0.7f, 0.7f, 0.7f)))),
                            cadv_material_make(0, white, 0.1f, CADV_MAT_NO_SHADOW));
        }
        if (s->muzzle > 0) {
            cadv_scene_light(sc, cadv_v3_add(eye, cadv_v3_scale(cadv_climber_forward(&s->me), 0.6f)), cadv_v3(2.0f, 1.6f, 0.8f), 6.0f, false);
        }
    }
    if (ui) {
        draw_hud(s, ui, underwater);
    }
}

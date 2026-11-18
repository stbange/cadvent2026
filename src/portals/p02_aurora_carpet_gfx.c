/* SPDX-License-Identifier: MIT */
/* GPU side of Day 2: deformable terrain chunks, sea, aurora, voxel castles. */
#include "portals/p02_aurora_carpet_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portals/p02_internal.h"

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

static void upload_voxels(cadv_gpu_mesh *g, cadv_voxels *v, float scale)
{
    cadv_mesh m;
    cadv_mesh_init(&m);
    cadv_voxels_mesh(v, scale, &m);
    cadv_gpu_mesh_upload(g, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(v);
}

static void ball(cadv_voxels *v, float cx, float cy, float cz, float rx, float ry, float rz, uint8_t c)
{
    int x;
    int y;
    int z;
    for (z = 0; z < v->d; ++z) {
        for (y = 0; y < v->h; ++y) {
            for (x = 0; x < v->w; ++x) {
                float dx = ((float)x + 0.5f - cx) / rx;
                float dy = ((float)y + 0.5f - cy) / ry;
                float dz = ((float)z + 0.5f - cz) / rz;
                if (dx * dx + dy * dy + dz * dz <= 1.0f) {
                    cadv_voxels_set(v, x, y, z, c);
                }
            }
        }
    }
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

/* ---- terrain ---------------------------------------------------------------------- */

static cadv_rgba ground_color(const cadv_terrain *t, float h, float slope)
{
    float above = h - t->sea;
    if (above < 0.6f) {
        return cadv_rgba_make(150, 140, 110, 255); /* shingle */
    }
    if (slope > 0.55f) {
        return cadv_rgba_make(105, 100, 110, 255); /* rock */
    }
    if (above > 14.0f) {
        return cadv_rgba_make(235, 240, 250, 255); /* snow */
    }
    if (above > 8.0f) {
        return cadv_rgba_make(200, 205, 215, 255); /* frosted heath */
    }
    return cadv_rgba_make(110, 125, 95, 255); /* tundra */
}

static void build_chunk(const p02_state *s, int cx, int cz, cadv_mesh *m)
{
    const cadv_terrain *t = &s->terrain;
    int x;
    int z;
    int x0 = cx * CADV_TERRAIN_CHUNK;
    int z0 = cz * CADV_TERRAIN_CHUNK;
    cadv_mesh_clear(m);
    for (z = 0; z <= CADV_TERRAIN_CHUNK; ++z) {
        for (x = 0; x <= CADV_TERRAIN_CHUNK; ++x) {
            float wx = (float)(x0 + x) * t->cell;
            float wz = (float)(z0 + z) * t->cell;
            float h = cadv_terrain_sample(t, x0 + x, z0 + z);
            cadv_vec3 n = cadv_terrain_normal(t, wx, wz);
            cadv_mesh_vertex(m, cadv_v3(wx, h, wz), n, wx / 8.0f, wz / 8.0f, ground_color(t, h, 1.0f - n.y));
        }
    }
    for (z = 0; z < CADV_TERRAIN_CHUNK; ++z) {
        for (x = 0; x < CADV_TERRAIN_CHUNK; ++x) {
            uint32_t a = (uint32_t)(z * (CADV_TERRAIN_CHUNK + 1) + x);
            uint32_t b = a + 1;
            uint32_t c = a + CADV_TERRAIN_CHUNK + 1;
            uint32_t d = c + 1;
            cadv_mesh_tri(m, a, c, b);
            cadv_mesh_tri(m, b, c, d);
        }
    }
}

static void refresh_terrain(p02_state *s)
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
            cadv_gpu_mesh_upload(&s->g.chunk[i], &m, true);
            s->g.chunk_seen[i] = s->terrain.chunk_ver[i];
        }
    }
    if (any) {
        cadv_mesh_free(&m);
    }
}

/* ---- models ----------------------------------------------------------------------- */

static void make_castle(cadv_voxels *v, int level, bool vael)
{
    int w = 10 + level * 4;
    int h = 8 + level * 4;
    int c = w / 2;
    int k;
    cadv_voxels_alloc(v, w + 1, h + 6, w + 1);
    v->pal[1] = vael ? cadv_rgba_make(170, 200, 230, 255) : cadv_rgba_make(200, 180, 140, 255);
    v->pal[2] = vael ? cadv_rgba_make(120, 80, 200, 255) : cadv_rgba_make(170, 50, 40, 255);
    v->pal[3] = vael ? cadv_rgba_make(120, 230, 255, 255) : cadv_rgba_make(255, 200, 90, 255);
    v->pal[4] = cadv_rgba_make(60, 55, 60, 255);
    /* outer wall with crenellations */
    block(v, 0, 0, 0, w, 2 + level, w, 1);
    block(v, 2, 1, 2, w - 2, 2 + level, w - 2, 4);
    for (k = 0; k <= w; k += 2) {
        block(v, k, 3 + level, 0, k, 3 + level, 0, 1);
        block(v, k, 3 + level, w, k, 3 + level, w, 1);
        block(v, 0, 3 + level, k, 0, 3 + level, k, 1);
        block(v, w, 3 + level, k, w, 3 + level, k, 1);
    }
    /* keep */
    block(v, c - 2 - level / 2, 0, c - 2 - level / 2, c + 2 + level / 2, h - 2, c + 2 + level / 2, 1);
    block(v, c - 1, h - 1, c - 1, c + 1, h + 1, c + 1, 2);
    cadv_voxels_set(v, c, h + 2, c, 3);
    /* corner towers from level 2 */
    if (level >= 2) {
        int t[4][2] = {{1, 1}, {w - 1, 1}, {1, w - 1}, {w - 1, w - 1}};
        for (k = 0; k < 4; ++k) {
            block(v, t[k][0] - 1, 0, t[k][1] - 1, t[k][0] + 1, h - 4, t[k][1] + 1, 1);
            block(v, t[k][0] - 1, h - 3, t[k][1] - 1, t[k][0] + 1, h - 3, t[k][1] + 1, 2);
            cadv_voxels_set(v, t[k][0], h - 2, t[k][1], 3);
        }
    }
    /* windows glow */
    block(v, c, h / 2, c - 2 - level / 2, c, h / 2 + 1, c - 2 - level / 2, 3);
    block(v, c, h / 2, c + 2 + level / 2, c, h / 2 + 1, c + 2 + level / 2, 3);
}

static void make_creature(cadv_voxels *v, int type)
{
    switch (type) {
    case P02_WOLF: /* snow-wolf */
        cadv_voxels_alloc(v, 8, 10, 16);
        v->pal[1] = cadv_rgba_make(225, 230, 240, 255);
        v->pal[2] = cadv_rgba_make(150, 160, 180, 255);
        v->pal[3] = cadv_rgba_make(90, 200, 255, 255);
        block(v, 1, 0, 3, 2, 3, 4, 2);
        block(v, 5, 0, 3, 6, 3, 4, 2);
        block(v, 1, 0, 11, 2, 3, 12, 2);
        block(v, 5, 0, 11, 6, 3, 12, 2);
        ball(v, 4, 5, 8, 3.2f, 2.4f, 5.5f, 1);
        ball(v, 4, 6.5f, 13.5f, 2.2f, 2.2f, 2.2f, 1);
        block(v, 3, 5, 15, 4, 6, 15, 2);
        cadv_voxels_set(v, 2, 7, 15, 3);
        cadv_voxels_set(v, 5, 7, 15, 3);
        block(v, 2, 8, 13, 2, 9, 13, 1);
        block(v, 5, 8, 13, 5, 9, 13, 1);
        block(v, 3, 5, 0, 4, 6, 2, 2);
        break;
    case P02_GRIFFIN: /* snow-griffin */
        cadv_voxels_alloc(v, 22, 12, 14);
        v->pal[1] = cadv_rgba_make(200, 170, 120, 255);
        v->pal[2] = cadv_rgba_make(240, 240, 245, 255);
        v->pal[3] = cadv_rgba_make(250, 200, 60, 255);
        ball(v, 11, 6, 7, 3.0f, 2.6f, 5.0f, 1);
        ball(v, 11, 8, 12, 2.0f, 2.0f, 2.0f, 2);
        block(v, 11, 7, 13, 11, 7, 13, 3);
        block(v, 0, 7, 5, 8, 8, 9, 2);
        block(v, 14, 7, 5, 21, 8, 9, 2);
        block(v, 10, 3, 0, 12, 5, 2, 1);
        break;
    default: /* Vael on his ice-crystal carpet */
        cadv_voxels_alloc(v, 14, 18, 14);
        v->pal[1] = cadv_rgba_make(150, 220, 255, 255);
        v->pal[2] = cadv_rgba_make(70, 50, 130, 255);
        v->pal[3] = cadv_rgba_make(235, 230, 220, 255);
        v->pal[4] = cadv_rgba_make(200, 240, 255, 255);
        block(v, 0, 0, 1, 13, 0, 12, 1);
        block(v, 4, 1, 5, 9, 9, 9, 2);
        ball(v, 6.5f, 11.5f, 7, 2.2f, 2.2f, 2.2f, 3);
        block(v, 5, 13, 5, 8, 13, 9, 2);
        block(v, 6, 14, 6, 7, 16, 8, 2);
        block(v, 6, 9, 9, 7, 10, 10, 4); /* beard */
        block(v, 10, 4, 7, 10, 12, 7, 4); /* crystal staff */
        cadv_voxels_set(v, 10, 13, 7, 1);
        break;
    }
}

void cadv_p02_gpu_init(void *state)
{
    p02_state *s = (p02_state *)state;
    p02_gpu *g = &s->g;
    cadv_voxels v;
    cadv_mesh m;
    int i;
    int k;
    float size = cadv_terrain_size(&s->terrain);
    memset(g, 0, sizeof *g);
    g->tex_ground = make_tex(CADV_TEX_NOISE, 21u, cadv_rgba_make(210, 210, 210, 255), cadv_rgba_make(250, 250, 250, 255));
    g->tex_water = make_tex(CADV_TEX_ICE, 22u, cadv_rgba_make(20, 50, 80, 255), cadv_rgba_make(60, 110, 140, 255));
    g->tex_carpet = make_tex(CADV_TEX_CLOTH, 23u, cadv_rgba_make(150, 30, 60, 255), cadv_rgba_make(230, 180, 60, 255));
    refresh_terrain(s);
    cadv_mesh_init(&m);
    cadv_mesh_quad(&m, cadv_v3(-200, s->terrain.sea, size + 200), cadv_v3(size + 200, s->terrain.sea, size + 200),
                   cadv_v3(size + 200, s->terrain.sea, -200), cadv_v3(-200, s->terrain.sea, -200), 40.0f, 40.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->water, &m, false);
    /* aurora curtains: three wavy ribbons high over the fjords */
    cadv_mesh_clear(&m);
    for (k = 0; k < 3; ++k) {
        float z0 = 40.0f + 70.0f * (float)k;
        for (i = 0; i < 24; ++i) {
            float x0 = -40.0f + (float)i * (size + 80.0f) / 24.0f;
            float x1 = x0 + (size + 80.0f) / 24.0f;
            float za = z0 + 18.0f * cadv_sinf(x0 * 0.03f + (float)k);
            float zb = z0 + 18.0f * cadv_sinf(x1 * 0.03f + (float)k);
            float y0 = 88.0f + 8.0f * (float)k;
            uint32_t a = cadv_mesh_vertex(&m, cadv_v3(x0, y0, za), cadv_v3(0, 0, 1), 0, 0, cadv_rgba_make(40, 255, 140, 255));
            uint32_t b = cadv_mesh_vertex(&m, cadv_v3(x1, y0, zb), cadv_v3(0, 0, 1), 1, 0, cadv_rgba_make(40, 255, 140, 255));
            uint32_t c = cadv_mesh_vertex(&m, cadv_v3(x1, y0 + 26.0f, zb + 6.0f), cadv_v3(0, 0, 1), 1, 1, cadv_rgba_make(150, 60, 255, 255));
            uint32_t d = cadv_mesh_vertex(&m, cadv_v3(x0, y0 + 26.0f, za + 6.0f), cadv_v3(0, 0, 1), 0, 1, cadv_rgba_make(150, 60, 255, 255));
            cadv_mesh_tri(&m, a, b, c);
            cadv_mesh_tri(&m, a, c, d);
            cadv_mesh_tri(&m, a, c, b); /* both sides */
            cadv_mesh_tri(&m, a, d, c);
        }
    }
    cadv_gpu_mesh_upload(&g->aurora, &m, false);
    /* the carpet's front edge with tassels, and a casting hand */
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-0.7f, 0, 0), cadv_v3(0.7f, 0, 0), cadv_v3(0.7f, 0, -1.2f), cadv_v3(-0.7f, 0, -1.2f), 2.0f, 2.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    for (i = 0; i < 8; ++i) {
        float x = -0.63f + (float)i * 0.18f;
        cadv_mesh_box(&m, cadv_v3(x - 0.02f, -0.12f, -1.24f), cadv_v3(x + 0.02f, 0.0f, -1.2f), 0.0f, cadv_rgba_make(255, 210, 80, 255), false);
    }
    cadv_gpu_mesh_upload(&g->carpet, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-0.06f, -0.05f, -0.2f), cadv_v3(0.06f, 0.05f, 0.05f), 0.0f, cadv_rgba_make(240, 200, 170, 255), false);
    cadv_mesh_box(&m, cadv_v3(-0.08f, -0.07f, 0.05f), cadv_v3(0.08f, 0.07f, 0.3f), 0.0f, cadv_rgba_make(90, 40, 120, 255), false);
    cadv_gpu_mesh_upload(&g->hand, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, 0, 0), cadv_v3(0.5f, 0, 0), cadv_v3(0.5f, 1, 0), cadv_v3(-0.5f, 1, 0), 1.0f, 1.0f,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->plaque, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, -0.5f, -0.5f), cadv_v3(0.5f, 0.5f, 0.5f), 0.0f, cadv_rgba_make(255, 255, 255, 255), false);
    cadv_gpu_mesh_upload(&g->cube, &m, false);
    cadv_mesh_free(&m);
    /* orb, balloon, scroll, star */
    cadv_voxels_alloc(&v, 5, 5, 5);
    v.pal[1] = cadv_rgba_make(255, 255, 255, 255);
    ball(&v, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 1);
    upload_voxels(&g->orb, &v, 0.22f);
    cadv_voxels_alloc(&v, 9, 14, 9);
    v.pal[1] = cadv_rgba_make(255, 255, 255, 255);
    v.pal[2] = cadv_rgba_make(140, 100, 60, 255);
    ball(&v, 4.5f, 9.5f, 4.5f, 4.5f, 4.5f, 4.5f, 1);
    block(&v, 3, 0, 3, 5, 1, 5, 2);
    block(&v, 4, 2, 4, 4, 4, 4, 2);
    upload_voxels(&g->balloon, &v, 0.25f);
    cadv_voxels_alloc(&v, 3, 6, 3);
    v.pal[1] = cadv_rgba_make(255, 230, 170, 255);
    v.pal[2] = cadv_rgba_make(255, 80, 30, 255);
    block(&v, 0, 0, 0, 2, 5, 2, 1);
    block(&v, 1, 0, 1, 1, 5, 1, 2);
    upload_voxels(&g->scroll, &v, 0.3f);
    cadv_voxels_alloc(&v, 15, 15, 3);
    v.pal[1] = cadv_rgba_make(255, 225, 100, 255);
    {
        int x;
        int y;
        int z;
        for (y = 0; y < 15; ++y) {
            for (x = 0; x < 15; ++x) {
                float px = (float)x - 7.0f;
                float py = (float)y - 7.0f;
                float a = cadv_atan2f(py, px) + 0.5f * CADV_PI;
                float r = cadv_sqrtf(px * px + py * py);
                float kk = cadv_cosf(5.0f * a);
                float lim = 3.4f + 3.6f * (kk > 0.0f ? kk * kk : 0.0f);
                for (z = 0; z < 3 && r <= lim; ++z) {
                    cadv_voxels_set(&v, x, y, z, 1);
                }
            }
        }
    }
    upload_voxels(&g->star, &v, 0.2f);
    for (k = 0; k < 2; ++k) {
        for (i = 0; i < P02_MAX_CASTLE_LEVEL; ++i) {
            make_castle(&v, i + 1, k == 1);
            upload_voxels(&g->castle[k][i], &v, 0.5f);
        }
    }
    for (i = 0; i < 3; ++i) {
        cadv_texdata c;
        cadv_texdata n;
        make_creature(&v, i);
        cadv_voxels_bake_sprites(&v, 8, 4, &c, &n, &g->fw[i], &g->fh[i]);
        g->tex_sprite[i] = cadv_gl_texture(&c, false);
        g->tex_normal[i] = cadv_gl_texture(&n, false);
        cadv_texdata_free(&c);
        cadv_texdata_free(&n);
        cadv_voxels_free(&v);
    }
    g->tex_mark = text_tex(s->mark_text[2], &g->mark_w, cadv_rgba_make(255, 90, 160, 255), cadv_rgba_make(30, 25, 40, 255));
    s->gpu_ready = true;
}

void cadv_p02_gpu_free(void *state)
{
    p02_state *s = (p02_state *)state;
    p02_gpu *g = &s->g;
    int i;
    int k;
    for (i = 0; i < CADV_TERRAIN_MAX_CHUNKS; ++i) {
        cadv_gpu_mesh_free(&g->chunk[i]);
    }
    cadv_gpu_mesh_free(&g->water);
    cadv_gpu_mesh_free(&g->aurora);
    cadv_gpu_mesh_free(&g->orb);
    cadv_gpu_mesh_free(&g->balloon);
    cadv_gpu_mesh_free(&g->carpet);
    cadv_gpu_mesh_free(&g->hand);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->scroll);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->cube);
    for (k = 0; k < 2; ++k) {
        for (i = 0; i < P02_MAX_CASTLE_LEVEL; ++i) {
            cadv_gpu_mesh_free(&g->castle[k][i]);
        }
    }
    cadv_gl_delete_textures(1, &g->tex_ground);
    cadv_gl_delete_textures(1, &g->tex_water);
    cadv_gl_delete_textures(1, &g->tex_carpet);
    cadv_gl_delete_textures(1, &g->tex_mark);
    cadv_gl_delete_textures(3, g->tex_sprite);
    cadv_gl_delete_textures(3, g->tex_normal);
    s->gpu_ready = false;
}

/* ---- draw --------------------------------------------------------------------------- */

static cadv_vec4 owner_tint(int owner)
{
    return owner == P02_PLAYER ? cadv_v4(1.0f, 0.6f, 0.2f, 1) : owner == P02_VAEL ? cadv_v4(0.3f, 0.85f, 1.0f, 1) : cadv_v4(1.0f, 0.95f, 0.7f, 1);
}

static void draw_hud(const p02_state *s, cadv_ui *ui)
{
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float cx = (float)ui->width * 0.5f;
    float cy = (float)ui->height * 0.5f;
    float bar = (float)ui->width * 0.5f;
    float mine = cadv_p02_share(s, P02_PLAYER);
    float vael = cadv_p02_share(s, P02_VAEL);
    char buf[96];
    const p02_castle *c = &s->castle[P02_PLAYER];
    cadv_ui_rect(ui, cx - 1, cy - 4, 2, 3, white);
    cadv_ui_rect(ui, cx - 1, cy + 2, 2, 3, white);
    cadv_ui_rect(ui, cx - 5, cy, 3, 1, white);
    cadv_ui_rect(ui, cx + 3, cy, 3, 1, white);
    /* mana ring as a bar: yours from the left, Vael's from the right, 75 % mark */
    cadv_ui_rect(ui, cx - bar * 0.5f - 1, 3, bar + 2, 6, cadv_v4(0, 0, 0, 0.6f));
    cadv_ui_rect(ui, cx - bar * 0.5f, 4, bar * mine, 4, cadv_v4(1, 0.6f, 0.2f, 1));
    cadv_ui_rect(ui, cx + bar * 0.5f - bar * vael, 4, bar * vael, 4, cadv_v4(0.3f, 0.85f, 1, 1));
    cadv_ui_rect(ui, cx - bar * 0.5f + bar * CADV_P02_WIN_SHARE, 2, 1, 8, white);
    snprintf(buf, sizeof buf, "MANA %d%%", (int)(mine * 100.0f + 0.5f));
    cadv_ui_text_centered(ui, cx, 11, 1, cadv_v4(1, 0.85f, 0.6f, 1), buf);
    snprintf(buf, sizeof buf, "HP %d", s->hp);
    cadv_ui_text(ui, 4, (float)ui->height - 12, 1, cadv_v4(1, 0.4f, 0.4f, 1), buf);
    if (c->exists) {
        snprintf(buf, sizeof buf, "CASTLE L%d  %d", c->level, c->mana);
    } else {
        snprintf(buf, sizeof buf, "NO CASTLE  %d", c->mana);
    }
    cadv_ui_text(ui, 4, (float)ui->height - 22, 1, cadv_v4(1, 0.8f, 0.5f, 1), buf);
    snprintf(buf, sizeof buf, "%d %s (%d)", s->spell + 1, cadv_p02_spell_name(s->spell), cadv_p02_spell_cost(s->spell));
    cadv_ui_text(ui, (float)ui->width - 6.0f * (float)strlen(buf) - 4, (float)ui->height - 12, 1, cadv_v4(1, 0.9f, 0.5f, 1), buf);
    if (s->quip_timer > 0) {
        cadv_ui_text_centered(ui, cx, (float)ui->height - 34, 1, cadv_v4(0.7f, 1, 0.8f, 1), s->quip);
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx, cy + 22, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, (float)ui->width, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(0.5f, 1, 0.7f, 1), "THE AURORA RETURNS!");
        cadv_ui_text_centered(ui, cx, cy + 4, 1, white, "PRESS USE TO RETURN TO THE HALL");
    }
    if (s->hurt_flash > 0 || s->lose_timer > 0) {
        cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.6f, 0.1f, 0.1f, 0.3f));
    }
}

static void draw_sprite(cadv_scene *sc, const p02_gpu *g, int kind, cadv_vec3 pos, float yaw, cadv_vec3 eye, float scale, cadv_vec4 tint)
{
    cadv_sprite sp;
    int frame = cadv_sprite_frame(pos, yaw, eye, 8);
    memset(&sp, 0, sizeof sp);
    sp.pos = pos;
    sp.w = (float)g->fw[kind] / 4.0f * scale;
    sp.h = (float)g->fh[kind] / 4.0f * scale;
    sp.tex = g->tex_sprite[kind];
    sp.normal_tex = g->tex_normal[kind];
    sp.u0 = (float)frame / 8.0f;
    sp.u1 = (float)(frame + 1) / 8.0f;
    sp.v1 = 1.0f;
    sp.tint = tint;
    sp.casts_shadow = true;
    cadv_scene_sprite(sc, &sp);
}

void cadv_p02_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p02_state *s = (p02_state *)state;
    p02_gpu *g = &s->g;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float t = ((float)s->ticks + alpha) * P02_DT;
    bool night = s->phase >= P02_PHASE_NIGHT;
    int chunks = cadv_terrain_chunks_per_side(&s->terrain);
    int lights = 0;
    int i;
    int k;
    cadv_mat4 body;
    refresh_terrain(s);
    cam->pos = cadv_v3_add(s->player.pos, cadv_v3(0, 0.9f, 0));
    if (s->shake > 0) {
        cam->pos.x += 0.12f * cadv_sinf(t * 70.0f);
        cam->pos.y += 0.10f * cadv_sinf(t * 53.0f);
    }
    cam->yaw = s->player.yaw;
    cam->pitch = s->player.pitch;
    cam->fovy = 1.2f;
    cam->znear = 0.1f;
    cam->zfar = 300.0f;
    sc->light_bands = 6;
    sc->dither = 0.6f;
    sc->use_palette = true;
    sc->sun_shadow = true;
    sc->shadow_center = cadv_v3(s->player.pos.x, s->terrain.sea + 8.0f, s->player.pos.z);
    sc->shadow_extent = 70.0f;
    if (night) {
        sc->ambient = cadv_v3(0.10f, 0.12f, 0.20f);
        sc->sun_dir = cadv_v3_norm(cadv_v3(-0.3f, -1.0f, 0.2f));
        sc->sun_color = cadv_v3(0.18f, 0.28f, 0.30f);
        sc->clear_color = cadv_v3(0.01f, 0.02f, 0.06f);
        sc->fog_color = cadv_v3(0.02f, 0.04f, 0.08f);
    } else {
        sc->ambient = cadv_v3(0.30f, 0.26f, 0.34f);
        sc->sun_dir = cadv_v3_norm(cadv_v3(0.9f, -0.35f, 0.2f));
        sc->sun_color = cadv_v3(1.0f, 0.62f, 0.42f);
        sc->clear_color = cadv_v3(0.35f, 0.22f, 0.35f);
        sc->fog_color = cadv_v3(0.40f, 0.28f, 0.40f);
    }
    sc->fog_near = 90.0f;
    sc->fog_far = 260.0f;
    /* land, sea and sky */
    for (i = 0; i < chunks * chunks; ++i) {
        cadv_scene_draw(sc, &g->chunk[i], cadv_m4_identity(), cadv_material_make(g->tex_ground, white, 0.0f, 0));
    }
    cadv_scene_draw(sc, &g->water, cadv_m4_identity(),
                    cadv_material_make(g->tex_water, night ? cadv_v4(0.5f, 0.9f, 0.8f, 1) : cadv_v4(0.9f, 0.8f, 1.0f, 1), night ? 0.35f : 0.15f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->aurora, cadv_m4_translate(cadv_v3(4.0f * cadv_sinf(t * 0.2f), 0, 0)),
                    cadv_material_make(0, white, (night ? 1.3f : 0.6f) + 0.2f * cadv_sinf(t * 0.7f), CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW | CADV_MAT_NO_FOG));
    /* castles */
    for (k = P02_PLAYER; k <= P02_VAEL; ++k) {
        const p02_castle *c = &s->castle[k];
        if (c->exists) {
            cadv_vec4 tint = c->hit_flash > 0 ? cadv_v4(1.6f, 0.7f, 0.6f, 1) : white;
            cadv_scene_draw(sc, &g->castle[k - 1][c->level - 1], cadv_m4_translate(c->pos), cadv_material_make(0, tint, 0.05f, 0));
            if (lights < 24) {
                cadv_scene_light(sc, cadv_v3_add(c->pos, cadv_v3(0, 6.0f + (float)c->level * 2.0f, 0)),
                                 k == P02_PLAYER ? cadv_v3(1.6f, 0.9f, 0.4f) : cadv_v3(0.4f, 0.9f, 1.6f), 22.0f, false);
                ++lights;
            }
        }
    }
    /* mana orbs (lights for the nearest few) */
    for (i = 0; i < s->norbs; ++i) {
        const p02_orb *o = &s->orbs[i];
        cadv_vec4 c = owner_tint(o->owner);
        if (o->state == P02_ORB_GONE) {
            continue;
        }
        cadv_scene_draw(sc, &g->orb, cadv_m4_translate(cadv_v3(o->pos.x, o->pos.y - 0.5f + 0.15f * cadv_sinf(t * 2.0f + (float)i), o->pos.z)),
                        cadv_material_make(0, c, 0.9f, CADV_MAT_NO_SHADOW));
        if (lights < 20 && cadv_v3_dist(o->pos, cam->pos) < 30.0f) {
            cadv_scene_light(sc, o->pos, cadv_v3(c.x * 0.8f, c.y * 0.8f, c.z * 0.8f), 5.0f, false);
            ++lights;
        }
    }
    for (i = 0; i < P02_MAX_BALLOONS; ++i) {
        const p02_balloon *b = &s->balloons[i];
        if (b->active) {
            cadv_scene_draw(sc, &g->balloon, cadv_m4_translate(cadv_v3_sub(b->pos, cadv_v3(0, 1.2f, 0))), cadv_material_make(0, owner_tint(b->owner), 0.15f, 0));
        }
    }
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        const p02_creature *c = &s->creatures[i];
        if (c->active) {
            draw_sprite(sc, g, c->type, c->pos, c->yaw, cam->pos, c->type == P02_WOLF ? 0.16f : 0.2f,
                        c->hurt > 0 ? cadv_v4(1, 0.5f, 0.5f, 1) : white);
        }
    }
    if (s->vael.alive) {
        draw_sprite(sc, g, 2, cadv_v3_sub(s->vael.fly.pos, cadv_v3(0, 0.8f, 0)), s->vael.fly.yaw, cam->pos, 0.2f, white);
        if (lights < 26) {
            cadv_scene_light(sc, s->vael.fly.pos, cadv_v3(0.3f, 0.7f, 1.2f), 8.0f, false);
            ++lights;
        }
    }
    /* spells in flight; the nearest fireball throws shadows */
    {
        int nearest = -1;
        float nd = 1e9f;
        for (i = 0; i < P02_MAX_PROJ; ++i) {
            if (s->proj[i].active && s->proj[i].kind == P02_PROJ_FIREBALL && cadv_v3_dist(s->proj[i].pos, cam->pos) < nd) {
                nd = cadv_v3_dist(s->proj[i].pos, cam->pos);
                nearest = i;
            }
        }
        for (i = 0; i < P02_MAX_PROJ; ++i) {
            const p02_proj *p = &s->proj[i];
            cadv_vec3 c;
            if (!p->active) {
                continue;
            }
            c = p->kind == P02_PROJ_CLAIM ? cadv_v3(1.0f, 1.0f, 0.6f) : (p->owner == P02_VAEL ? cadv_v3(0.4f, 0.8f, 1.6f) : cadv_v3(1.8f, 0.7f, 0.2f));
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(p->pos), cadv_m4_scale(cadv_v3(0.5f, 0.5f, 0.5f))),
                            cadv_material_make(0, cadv_v4(c.x, c.y, c.z, 1), 1.5f, CADV_MAT_NO_SHADOW | CADV_MAT_UNLIT));
            if (lights < 30) {
                cadv_scene_light(sc, p->pos, c, 9.0f, i == nearest);
                ++lights;
            }
        }
    }
    for (i = 0; i < 16; ++i) {
        if (s->flash_ttl[i] > 0 && lights < 31) {
            cadv_scene_light(sc, s->flash_pos[i], cadv_v3_scale(cadv_v3(2.0f, 1.0f, 0.4f), (float)s->flash_ttl[i] / 18.0f), 14.0f, false);
            ++lights;
        }
    }
    if (s->volcano_ticks > 0 && lights < 32) {
        cadv_scene_light(sc, cadv_v3_add(s->volcano_pos, cadv_v3(0, 8, 0)), cadv_v3(2.0f, 0.6f, 0.1f), 30.0f, false);
        ++lights;
    }
    if (s->scroll_active) {
        cadv_scene_draw(sc, &g->scroll, cadv_m4_mul(cadv_m4_translate(s->scroll_pos), cadv_m4_rotate_y(t)),
                        cadv_material_make(0, white, 1.0f, CADV_MAT_NO_SHADOW));
        if (lights < 32) {
            cadv_scene_light(sc, s->scroll_pos, cadv_v3(1.5f, 0.6f, 0.2f), 10.0f, false);
            ++lights;
        }
    }
    if (s->star_active) {
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(s->star_pos), cadv_m4_rotate_y(t * 1.5f)),
                        cadv_material_make(0, white, 1.2f, CADV_MAT_NO_SHADOW | CADV_MAT_NO_FOG));
        if (lights < 32) {
            cadv_scene_light(sc, s->star_pos, cadv_v3(2.0f, 1.7f, 0.6f), 20.0f, false);
            ++lights;
        }
    }
    /* Messerjocke's Rock wears its name */
    cadv_scene_draw(sc, &g->plaque,
                    cadv_m4_mul(cadv_m4_translate(cadv_v3(s->rock_pos.x, s->rock_pos.y + 1.0f, s->rock_pos.z - 5.5f)),
                                cadv_m4_mul(cadv_m4_rotate_y(CADV_PI), cadv_m4_scale(cadv_v3((float)g->mark_w / 6.0f, 2.0f, 1)))),
                    cadv_material_make(g->tex_mark, white, 0.8f, CADV_MAT_NO_SHADOW));
    /* the apprentice's carpet and casting hand */
    body = cadv_m4_mul(cadv_m4_translate(cadv_v3(s->player.pos.x, s->player.pos.y - 0.35f, s->player.pos.z)), cadv_m4_rotate_y(s->player.yaw));
    cadv_scene_draw(sc, &g->carpet, cadv_m4_mul(body, cadv_m4_translate(cadv_v3(0, 0.0f, 0.2f))), cadv_material_make(g->tex_carpet, white, 0.1f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->hand, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(cadv_v3(0.30f, 0.55f + (s->cast_cd > 0.0f ? 0.06f : 0.0f), -0.55f)), cadv_m4_scale(cadv_v3(0.6f, 0.6f, 0.6f)))),
                    cadv_material_make(0, white, 0.1f, CADV_MAT_NO_SHADOW));
    cadv_scene_draw(sc, &g->hand, cadv_m4_mul(body, cadv_m4_mul(cadv_m4_translate(cadv_v3(-0.30f, 0.52f, -0.52f)), cadv_m4_scale(cadv_v3(0.6f, 0.6f, 0.6f)))),
                    cadv_material_make(0, white, 0.1f, CADV_MAT_NO_SHADOW));
    if (s->cast_cd > 0.2f) {
        cadv_scene_light(sc, cadv_v3_add(cam->pos, cadv_v3_scale(cadv_flyer_forward(&s->player), 0.8f)), cadv_v3(1.4f, 0.8f, 0.4f), 6.0f, false);
    }
    if (ui) {
        draw_hud(s, ui);
    }
}

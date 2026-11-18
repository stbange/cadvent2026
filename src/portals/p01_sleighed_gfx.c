/* SPDX-License-Identifier: MIT */
/* GPU side of Day 1: procedural mall materials, original voxel characters. */
#include "portals/p01_sleighed_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portals/p01_internal.h"

typedef struct mat_def {
    const char *name;
    cadv_tex_kind kind;
    cadv_rgba a, b;
    float emissive;
} mat_def;

#define C(r, g, b) {r, g, b, 255}
static const mat_def k_mats[] = {
    {"concrete", CADV_TEX_PLASTER, C(120, 118, 112), C(90, 88, 84), 0},
    {"asphalt", CADV_TEX_NOISE, C(50, 50, 55), C(70, 70, 74), 0},
    {"pillar", CADV_TEX_STONE_BRICKS, C(150, 140, 60), C(40, 40, 40), 0},
    {"car_red", CADV_TEX_METAL, C(170, 30, 30), C(230, 220, 200), 0},
    {"car_blue", CADV_TEX_METAL, C(40, 70, 170), C(230, 220, 200), 0},
    {"marble_wall", CADV_TEX_PLASTER, C(200, 190, 175), C(160, 150, 140), 0},
    {"marble", CADV_TEX_FLOOR_TILES, C(210, 205, 195), C(120, 30, 40), 0},
    {"glass_roof", CADV_TEX_ICE, C(40, 60, 100), C(90, 120, 170), 0.2f},
    {"planter", CADV_TEX_PLANKS, C(110, 70, 40), C(40, 25, 15), 0},
    {"escalator", CADV_TEX_METAL, C(150, 150, 160), C(220, 220, 230), 0},
    {"steel", CADV_TEX_METAL, C(110, 120, 130), C(240, 200, 60), 0},
    {"shop_wall", CADV_TEX_PLASTER, C(170, 60, 70), C(200, 120, 120), 0},
    {"carpet", CADV_TEX_CLOTH, C(40, 90, 50), C(200, 170, 60), 0},
    {"shop_ceiling", CADV_TEX_PLASTER, C(150, 150, 150), C(120, 120, 120), 0},
    {"shelf", CADV_TEX_PLANKS, C(200, 160, 90), C(90, 60, 30), 0},
    {"tile_wall", CADV_TEX_FLOOR_TILES, C(220, 220, 210), C(60, 140, 160), 0},
    {"tile", CADV_TEX_FLOOR_TILES, C(220, 210, 180), C(70, 70, 90), 0},
    {"table", CADV_TEX_PLANKS, C(180, 120, 70), C(60, 40, 20), 0},
    {"velvet", CADV_TEX_CLOTH, C(120, 20, 40), C(220, 180, 70), 0},
    {"carpet_red", CADV_TEX_CLOTH, C(140, 30, 40), C(40, 10, 15), 0},
    {"dark", CADV_TEX_PLASTER, C(30, 25, 35), C(15, 12, 18), 0},
    {"screen", CADV_TEX_NOISE, C(120, 140, 200), C(200, 220, 255), 0.9f},
    {"seats", CADV_TEX_CLOTH, C(150, 30, 40), C(90, 20, 25), 0},
    {"booth", CADV_TEX_PLANKS, C(80, 60, 50), C(40, 30, 25), 0},
    {"crate", CADV_TEX_PLANKS, C(170, 130, 70), C(80, 55, 25), 0},
    {"pipes", CADV_TEX_METAL, C(70, 80, 70), C(140, 120, 60), 0},
    {"roof", CADV_TEX_NOISE, C(200, 205, 215), C(230, 235, 245), 0},
    {"parapet", CADV_TEX_STONE_BRICKS, C(120, 110, 105), C(50, 45, 45), 0},
    {"hut", CADV_TEX_METAL, C(90, 100, 110), C(200, 200, 60), 0},
    {"siphon_base", CADV_TEX_METAL, C(60, 30, 90), C(200, 120, 255), 0.3f},
    {"siphon_beam", CADV_TEX_NOISE, C(160, 80, 255), C(255, 220, 255), 1.6f},
    {"platform", CADV_TEX_METAL, C(80, 80, 95), C(150, 150, 170), 0},
    {"sign_tower", CADV_TEX_METAL, C(150, 40, 40), C(240, 230, 220), 0},
};
#undef C

static const mat_def *find_mat(const char *name)
{
    size_t i;
    for (i = 0; i < sizeof k_mats / sizeof k_mats[0]; ++i) {
        if (strcmp(k_mats[i].name, name) == 0) {
            return &k_mats[i];
        }
    }
    return &k_mats[0];
}

static unsigned tex_for(const mat_def *m, uint32_t seed)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, m->kind, 64, seed, m->a, m->b);
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

/* ---- original voxel characters ------------------------------------------------ */

static void sphere(cadv_voxels *v, float cx, float cy, float cz, float rx, float ry, float rz, uint8_t c)
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

static void make_enemy(cadv_voxels *v, int type)
{
    switch (type) {
    case P01_HOG: /* space hog trooper: pink snout, green visor armour, blaster */
    case P01_LOBBER:
    case P01_BOSS:
        cadv_voxels_alloc(v, 14, 20, 14);
        v->pal[1] = cadv_rgba_make(235, 150, 160, 255);
        v->pal[2] = type == P01_LOBBER ? cadv_rgba_make(160, 100, 40, 255) : cadv_rgba_make(60, 120, 70, 255);
        v->pal[3] = cadv_rgba_make(120, 230, 255, 255);
        v->pal[4] = cadv_rgba_make(40, 40, 50, 255);
        v->pal[5] = type == P01_BOSS ? cadv_rgba_make(230, 190, 60, 255) : cadv_rgba_make(200, 60, 60, 255);
        block(v, 4, 0, 5, 5, 5, 8, 4);
        block(v, 8, 0, 5, 9, 5, 8, 4);
        sphere(v, 7, 10, 7, 4.5f, 5, 4, 2);
        sphere(v, 7, 15.5f, 7.5f, 3.5f, 3.2f, 3.5f, 1);
        block(v, 5, 15, 11, 8, 16, 12, 1);
        cadv_voxels_set(v, 6, 15, 13, 4);
        cadv_voxels_set(v, 7, 15, 13, 4);
        block(v, 4, 17, 10, 9, 17, 10, 3);
        block(v, 3, 18, 6, 4, 19, 7, 1);
        block(v, 9, 18, 6, 10, 19, 7, 1);
        block(v, 11, 8, 9, 12, 9, 13, 5);
        if (type == P01_BOSS) {
            block(v, 4, 19, 6, 9, 19, 8, 5); /* crown-like helmet ridge */
        }
        break;
    case P01_BOAR: /* boar with a snowplow blade */
        cadv_voxels_alloc(v, 14, 14, 18);
        v->pal[1] = cadv_rgba_make(110, 70, 50, 255);
        v->pal[2] = cadv_rgba_make(240, 240, 230, 255);
        v->pal[3] = cadv_rgba_make(230, 120, 30, 255);
        v->pal[4] = cadv_rgba_make(30, 30, 30, 255);
        sphere(v, 7, 7, 8, 5, 4.5f, 7, 1);
        block(v, 3, 0, 3, 4, 3, 4, 4);
        block(v, 9, 0, 3, 10, 3, 4, 4);
        block(v, 3, 0, 11, 4, 3, 12, 4);
        block(v, 9, 0, 11, 10, 3, 12, 4);
        block(v, 1, 1, 16, 12, 6, 17, 3);
        cadv_voxels_set(v, 4, 7, 15, 2);
        cadv_voxels_set(v, 9, 7, 15, 2);
        break;
    case P01_DRONE: /* little elf drone: round body, antenna, propeller */
        cadv_voxels_alloc(v, 10, 10, 10);
        v->pal[1] = cadv_rgba_make(40, 160, 70, 255);
        v->pal[2] = cadv_rgba_make(230, 40, 40, 255);
        v->pal[3] = cadv_rgba_make(250, 230, 120, 255);
        v->pal[4] = cadv_rgba_make(200, 200, 210, 255);
        sphere(v, 5, 4, 5, 3.5f, 3, 3.5f, 1);
        block(v, 3, 4, 8, 6, 4, 8, 3);
        block(v, 5, 7, 5, 5, 8, 5, 4);
        block(v, 1, 9, 4, 8, 9, 5, 4);
        cadv_voxels_set(v, 5, 9, 5, 2);
        break;
    default: /* gremlin gift: wrapped box with a ribbon and teeth */
        cadv_voxels_alloc(v, 10, 10, 10);
        v->pal[1] = cadv_rgba_make(60, 110, 200, 255);
        v->pal[2] = cadv_rgba_make(240, 200, 60, 255);
        v->pal[3] = cadv_rgba_make(250, 250, 250, 255);
        block(v, 1, 0, 1, 8, 7, 8, 1);
        block(v, 4, 0, 1, 5, 8, 8, 2);
        block(v, 1, 0, 4, 8, 8, 5, 2);
        block(v, 2, 3, 9, 7, 3, 9, 3);
        break;
    }
}

/* ---- init / free ------------------------------------------------------------------------ */

static int mat_index(p01_gpu *g, const char *name)
{
    int i;
    for (i = 0; i < g->mat_count; ++i) {
        if (strcmp(g->mat_name[i], name) == 0) {
            return i;
        }
    }
    if (g->mat_count >= P01_MAX_MATS) {
        return 0;
    }
    snprintf(g->mat_name[g->mat_count], CADV_MAP_NAME, "%s", name);
    return g->mat_count++;
}

void cadv_p01_gpu_init(void *state)
{
    p01_state *s = (p01_state *)state;
    p01_gpu *g = &s->g;
    cadv_mesh meshes[P01_MAX_MATS];
    cadv_mesh m;
    cadv_voxels v;
    int i;
    cadv_rgba white = cadv_rgba_make(255, 255, 255, 255);
    memset(g, 0, sizeof *g);
    for (i = 0; i < P01_MAX_MATS; ++i) {
        cadv_mesh_init(&meshes[i]);
    }
    for (i = 0; i < s->map.nbox; ++i) {
        const cadv_map_box *b = &s->map.box[i];
        if (b->kind <= 1) {
            int k = mat_index(g, b->mat);
            cadv_mesh_box(&meshes[k], b->lo, b->hi, 0.5f, white, false);
        }
    }
    for (i = 0; i < g->mat_count; ++i) {
        const mat_def *md = find_mat(g->mat_name[i]);
        cadv_gpu_mesh_upload(&g->mat_mesh[i], &meshes[i], false);
        g->mat_tex[i] = tex_for(md, 100u + (uint32_t)i);
        cadv_mesh_free(&meshes[i]);
    }
    cadv_mesh_init(&m);
    for (i = 0; i < s->fps.breakable_count && i < CADV_FPS_MAX_BREAKABLES; ++i) {
        cadv_aabb b = s->fps.world.s[s->fps.breakables[i].solid].box;
        cadv_mesh_clear(&m);
        cadv_mesh_box(&m, b.lo, b.hi, 0.5f, white, false);
        cadv_gpu_mesh_upload(&g->glass[i], &m, false);
    }
    for (i = 0; i < s->fps.door_count; ++i) {
        cadv_door *d = &s->fps.doors[i];
        cadv_mesh_clear(&m);
        cadv_mesh_box(&m, cadv_v3(d->closed_lo.x, 0, d->closed_lo.z),
                      cadv_v3(d->closed_hi.x, d->closed_hi.y - d->closed_lo.y, d->closed_hi.z), 0.5f, white, false);
        cadv_gpu_mesh_upload(&g->door[i], &m, false);
    }
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-0.5f, 0, -0.5f), cadv_v3(0.5f, 1, 0.5f), 0, white, false);
    cadv_gpu_mesh_upload(&g->cube, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-0.45f, 0, -0.45f), cadv_v3(0.45f, 1.8f, 0.45f), 0.5f, white, false);
    cadv_gpu_mesh_upload(&g->prop, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-0.5f, -0.5f, 0), cadv_v3(0.5f, -0.5f, 0), cadv_v3(0.5f, 0.5f, 0),
                   cadv_v3(-0.5f, 0.5f, 0), 1, 1, white);
    cadv_gpu_mesh_upload(&g->plaque, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_prism(&m, cadv_v3(0, 0, 0), 1.0f, 30.0f, 8, 0, white);
    cadv_gpu_mesh_upload(&g->beam, &m, false);
    cadv_mesh_free(&m);
    /* star and key */
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
                float k = cadv_cosf(5.0f * a);
                float lim = 3.4f + 3.6f * (k > 0.0f ? k * k : 0.0f);
                for (z = 0; z < 3 && r <= lim; ++z) {
                    if (z == 1 || r < lim - 1.5f) {
                        cadv_voxels_set(&v, x, y, z, 1);
                    }
                }
            }
        }
    }
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.06f, &m);
    cadv_gpu_mesh_upload(&g->star, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    cadv_voxels_alloc(&v, 3, 6, 3); /* a fuse candle */
    v.pal[1] = cadv_rgba_make(240, 230, 200, 255);
    v.pal[2] = cadv_rgba_make(255, 160, 40, 255);
    block(&v, 0, 0, 0, 2, 4, 2, 1);
    cadv_voxels_set(&v, 1, 5, 1, 2);
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.08f, &m);
    cadv_gpu_mesh_upload(&g->key, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    for (i = 0; i < P01_ENEMY_TYPES; ++i) {
        cadv_texdata c;
        cadv_texdata n;
        make_enemy(&v, i);
        cadv_voxels_bake_sprites(&v, 8, 4, &c, &n, &g->fw[i], &g->fh[i]);
        g->tex_sprites[i] = cadv_gl_texture(&c, false);
        g->tex_normals[i] = cadv_gl_texture(&n, false);
        cadv_texdata_free(&c);
        cadv_texdata_free(&n);
        cadv_voxels_free(&v);
    }
    for (i = 0; i < s->map.nsign && i < 32; ++i) {
        g->tex_sign[i] = text_tex(s->map.sign[i].text, &g->sign_w[i], cadv_rgba_make(255, 230, 120, 255),
                                  cadv_rgba_make(30, 20, 40, 255));
    }
    for (i = 0; i < s->map.nmark && i < 16; ++i) {
        int idx = (int)s->map.mark[i].a;
        g->tex_mark[i] = text_tex(idx >= 0 && idx < 16 ? s->mark_text[idx] : "?", &g->mark_w[i],
                                  cadv_rgba_make(255, 90, 160, 255), cadv_rgba_make(20, 20, 30, 255));
    }
    {
        mat_def glass = {"glass", CADV_TEX_ICE, {150, 190, 210, 255}, {210, 230, 240, 255}, 0};
        mat_def target = {"target", CADV_TEX_METAL, {200, 40, 90, 255}, {255, 200, 255, 255}, 0};
        g->tex_glass = tex_for(&glass, 7u);
        g->tex_target = tex_for(&target, 8u);
    }
    s->gpu_ready = true;
}

void cadv_p01_gpu_free(void *state)
{
    p01_state *s = (p01_state *)state;
    p01_gpu *g = &s->g;
    int i;
    for (i = 0; i < g->mat_count; ++i) {
        cadv_gpu_mesh_free(&g->mat_mesh[i]);
    }
    cadv_gl_delete_textures(g->mat_count, g->mat_tex);
    for (i = 0; i < CADV_FPS_MAX_BREAKABLES; ++i) {
        cadv_gpu_mesh_free(&g->glass[i]);
    }
    for (i = 0; i < CADV_FPS_MAX_DOORS; ++i) {
        cadv_gpu_mesh_free(&g->door[i]);
    }
    cadv_gpu_mesh_free(&g->cube);
    cadv_gpu_mesh_free(&g->prop);
    cadv_gpu_mesh_free(&g->plaque);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->key);
    cadv_gpu_mesh_free(&g->beam);
    cadv_gl_delete_textures(P01_ENEMY_TYPES, g->tex_sprites);
    cadv_gl_delete_textures(P01_ENEMY_TYPES, g->tex_normals);
    cadv_gl_delete_textures(s->map.nsign < 32 ? s->map.nsign : 32, g->tex_sign);
    cadv_gl_delete_textures(s->map.nmark < 16 ? s->map.nmark : 16, g->tex_mark);
    cadv_gl_delete_textures(1, &g->tex_glass);
    cadv_gl_delete_textures(1, &g->tex_target);
    s->gpu_ready = false;
}

/* ---- draw ---------------------------------------------------------------------------------- */

static cadv_mat4 facing(cadv_vec3 pos, float yaw, float w, float h)
{
    return cadv_m4_mul(cadv_m4_translate(pos), cadv_m4_mul(cadv_m4_rotate_y(yaw), cadv_m4_scale(cadv_v3(w, h, 1))));
}

static void draw_hud(const p01_state *s, cadv_ui *ui, const cadv_fps *f)
{
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float cx = (float)ui->width * 0.5f;
    float cy = (float)ui->height * 0.5f;
    const cadv_weapon_def *w = cadv_fps_weapon(f, f->weapon);
    char buf[96];
    int secrets = 0;
    int i;
    for (i = 0; i < CADV_P01_SECRETS; ++i) {
        secrets += (s->secrets_found >> i) & 1u;
    }
    cadv_ui_rect(ui, cx - 1, cy - 4, 2, 3, white);
    cadv_ui_rect(ui, cx - 1, cy + 2, 2, 3, white);
    cadv_ui_rect(ui, cx - 5, cy, 3, 1, white);
    cadv_ui_rect(ui, cx + 3, cy, 3, 1, white);
    snprintf(buf, sizeof buf, "HP %d  AR %d", f->player.health, f->player.armor);
    cadv_ui_text(ui, 4, (float)ui->height - 12, 1, cadv_v4(1, 0.4f, 0.4f, 1), buf);
    snprintf(buf, sizeof buf, "%s %d", w->name, w->ammo_per_shot ? f->ammo[w->ammo_type] : 99);
    cadv_ui_text(ui, (float)ui->width - 6.0f * (float)strlen(buf) - 4, (float)ui->height - 12, 1, cadv_v4(1, 0.9f, 0.5f, 1), buf);
    snprintf(buf, sizeof buf, "FUSES %c%c%c  SECRETS %d/6", (f->player.keys & 1u) ? 'R' : '-',
             (f->player.keys & 2u) ? 'G' : '-', (f->player.keys & 4u) ? 'Y' : '-', secrets);
    cadv_ui_text(ui, 4, 4, 1, cadv_v4(0.9f, 0.9f, 1, 1), buf);
    if (f->jetpack) {
        cadv_ui_rect(ui, 4, (float)ui->height - 24, 40.0f * f->jet_fuel / CADV_JET_FUEL_MAX, 4, cadv_v4(0.3f, 0.8f, 1, 0.9f));
    }
    if (s->siphon_down && s->collapse_ticks > 0 && s->win_timer == 0) {
        snprintf(buf, sizeof buf, "DOME COLLAPSE IN %d", s->collapse_ticks / 60);
        cadv_ui_text_centered(ui, cx, 16, 1, cadv_v4(1, 0.3f, 0.2f, 1), buf);
    }
    if (s->quip_timer > 0) {
        cadv_ui_text_centered(ui, cx, (float)ui->height - 28, 1, cadv_v4(0.7f, 1, 0.7f, 1), s->quip);
    }
    if (s->message_timer > 0) {
        cadv_ui_text_centered(ui, cx, cy + 22, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
    }
    if (s->win_timer > 0) {
        cadv_ui_rect(ui, 0, cy - 22, (float)ui->width, 40, cadv_v4(0, 0, 0, 0.6f));
        cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(1, 0.85f, 0.3f, 1), "SLEIGHED IT!");
        cadv_ui_text_centered(ui, cx, cy + 4, 1, white, "PRESS USE TO RETURN TO THE HALL");
    }
    if (s->death_timer > 0) {
        cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.6f, 0.1f, 0.1f, 0.35f));
    }
}

void cadv_p01_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    p01_state *s = (p01_state *)state;
    p01_gpu *g = &s->g;
    const cadv_fps *f = &s->fps;
    const cadv_player *p = &f->player;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float t = ((float)s->ticks + alpha) * P01_DT;
    int i;
    cam->pos = cadv_player_eye(p);
    if (s->shake > 0) {
        cam->pos.x += 0.05f * cadv_sinf(t * 70.0f);
        cam->pos.y += 0.04f * cadv_sinf(t * 53.0f);
    }
    cam->yaw = p->yaw;
    cam->pitch = p->pitch;
    cam->fovy = 1.25f;
    cam->znear = 0.05f;
    cam->zfar = 80.0f;
    sc->ambient = p->pos.y > 10.0f ? cadv_v3(0.22f, 0.22f, 0.32f) : cadv_v3(0.16f, 0.15f, 0.18f);
    sc->clear_color = cadv_v3(0.02f, 0.02f, 0.06f);
    sc->fog_color = cadv_v3(0.03f, 0.02f, 0.06f);
    sc->fog_near = 18.0f;
    sc->fog_far = 60.0f;
    sc->light_bands = 6;
    sc->dither = 0.6f;
    sc->use_palette = true;
    if (p->pos.y > 10.0f) {
        sc->sun_dir = cadv_v3_norm(cadv_v3(0.3f, -1.0f, 0.4f));
        sc->sun_color = cadv_v3(0.25f, 0.28f, 0.45f);
        sc->sun_shadow = true;
        sc->shadow_center = cadv_v3(p->pos.x, 12.0f, p->pos.z - 6.0f);
        sc->shadow_extent = 22.0f;
    }
    for (i = 0; i < g->mat_count; ++i) {
        const mat_def *md = find_mat(g->mat_name[i]);
        bool beam = strcmp(md->name, "siphon_beam") == 0;
        bool base = strcmp(md->name, "siphon_base") == 0;
        if ((beam || base) && s->siphon_down) {
            continue;
        }
        cadv_scene_draw(sc, &g->mat_mesh[i], cadv_m4_identity(),
                        cadv_material_make(g->mat_tex[i], white, md->emissive * (beam ? 0.8f + 0.4f * cadv_sinf(t * 6.0f) : 1.0f),
                                           beam ? (CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW) : 0));
    }
    for (i = 0; i < f->breakable_count; ++i) {
        const cadv_breakable *b = &f->breakables[i];
        if (!b->broken) {
            bool tgt = b->group == 1;
            cadv_scene_draw(sc, &g->glass[i], cadv_m4_identity(),
                            cadv_material_make(tgt ? g->tex_target : g->tex_glass, white, tgt ? 0.6f + 0.3f * cadv_sinf(t * 8.0f) : 0.1f, 0));
            if (tgt) {
                cadv_aabb bx = f->world.s[b->solid].box;
                cadv_scene_light(sc, cadv_v3_scale(cadv_v3_add(bx.lo, bx.hi), 0.5f), cadv_v3(1.2f, 0.3f, 0.9f), 5.0f, false);
            }
        }
    }
    for (i = 0; i < f->door_count; ++i) {
        const cadv_door *d = &f->doors[i];
        if (d->open < 1.0f) {
            cadv_scene_draw(sc, &g->door[i], cadv_m4_translate(cadv_v3(0, f->world.s[d->solid].box.lo.y, 0)),
                            cadv_material_make(g->mat_tex[0], cadv_v4(0.6f, 0.65f, 0.75f, 1), 0, 0));
        }
    }
    for (i = 0; i < s->map.nprop; ++i) {
        const cadv_map_item *pr = &s->map.prop[i];
        bool vend = strcmp(pr->name, "vending") == 0;
        bool arcade = strcmp(pr->name, "arcade") == 0;
        cadv_vec4 tint = vend ? cadv_v4(0.9f, 0.2f, 0.2f, 1) : arcade ? cadv_v4(0.2f, 0.3f, 0.9f, 1) : cadv_v4(0.6f, 0.8f, 0.9f, 1);
        cadv_scene_draw(sc, &g->prop, cadv_m4_mul(cadv_m4_translate(pr->pos), cadv_m4_rotate_y(pr->yaw)),
                        cadv_material_make(0, tint, vend || arcade ? 0.35f : 0.1f, 0));
    }
    for (i = 0; i < s->map.nsign && i < 32; ++i) {
        const cadv_map_item *sg = &s->map.sign[i];
        float w = sg->a;
        float h = w * 12.0f / (float)g->sign_w[i];
        cadv_scene_draw(sc, &g->plaque, facing(sg->pos, sg->yaw, w, h), cadv_material_make(g->tex_sign[i], white, 0.7f, CADV_MAT_NO_SHADOW));
    }
    for (i = 0; i < s->map.nmark && i < 16; ++i) {
        const cadv_map_item *mk = &s->map.mark[i];
        float w = (float)g->mark_w[i] / 40.0f;
        cadv_scene_draw(sc, &g->plaque, facing(mk->pos, mk->yaw, w, w * 12.0f / (float)g->mark_w[i]),
                        cadv_material_make(g->tex_mark[i], white, 0.5f, CADV_MAT_NO_SHADOW));
    }
    for (i = 0; i < f->pickup_count; ++i) {
        const cadv_pickup *pk = &f->pickups[i];
        cadv_mat4 xf;
        if (!pk->active) {
            continue;
        }
        xf = cadv_m4_mul(cadv_m4_translate(cadv_v3(pk->pos.x, pk->pos.y + 0.1f * cadv_sinf(t * 2.0f + (float)i), pk->pos.z)),
                         cadv_m4_rotate_y(t * 1.5f));
        if (pk->kind == CADV_PICKUP_STAR) {
            cadv_scene_draw(sc, &g->star, xf, cadv_material_make(0, white, 1.0f, CADV_MAT_NO_SHADOW));
            cadv_scene_light(sc, pk->pos, cadv_v3(2.0f, 1.6f, 0.6f), 8.0f, false);
        } else if (pk->kind == CADV_PICKUP_KEY) {
            cadv_vec4 c = pk->value == 1 ? cadv_v4(1, 0.3f, 0.3f, 1) : pk->value == 2 ? cadv_v4(0.3f, 1, 0.4f, 1) : cadv_v4(1, 0.9f, 0.3f, 1);
            cadv_scene_draw(sc, &g->key, xf, cadv_material_make(0, c, 0.6f, 0));
            cadv_scene_light(sc, pk->pos, cadv_v3(c.x, c.y, c.z), 3.0f, false);
        } else {
            cadv_vec4 c = pk->kind == CADV_PICKUP_HEALTH ? cadv_v4(1, 0.2f, 0.2f, 1)
                          : pk->kind == CADV_PICKUP_WEAPON ? cadv_v4(0.9f, 0.9f, 0.9f, 1)
                          : pk->kind == CADV_PICKUP_JETPACK || pk->kind == CADV_PICKUP_FUEL ? cadv_v4(0.3f, 0.8f, 1, 1)
                                                                                            : cadv_v4(1, 0.8f, 0.3f, 1);
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(xf, cadv_m4_scale(cadv_v3(0.35f, 0.35f, 0.35f))), cadv_material_make(0, c, 0.4f, 0));
        }
    }
    for (i = 0; i < f->enemy_count; ++i) {
        const cadv_enemy *e = &f->enemies[i];
        cadv_sprite sp;
        int frame;
        int ty = e->type;
        float scale = ty == P01_BOSS ? 0.24f : 0.11f;
        float squash = e->state == CADV_ENEMY_DEAD ? 0.2f : (e->state == CADV_ENEMY_SHRUNK ? 0.3f : 1.0f);
        if (!e->active) {
            continue;
        }
        memset(&sp, 0, sizeof sp);
        frame = cadv_sprite_frame(e->pos, e->yaw, cam->pos, 8);
        sp.pos = e->pos;
        sp.w = (float)g->fw[ty] / 4.0f * scale * (e->state == CADV_ENEMY_SHRUNK ? 0.3f : 1.0f);
        sp.h = (float)g->fh[ty] / 4.0f * scale * squash;
        sp.tex = g->tex_sprites[ty];
        sp.normal_tex = g->tex_normals[ty];
        sp.u0 = (float)frame / 8.0f;
        sp.u1 = (float)(frame + 1) / 8.0f;
        sp.v1 = 1.0f;
        sp.tint = e->state == CADV_ENEMY_PAIN ? cadv_v4(1.0f, 0.5f, 0.5f, 1.0f) : white;
        sp.casts_shadow = e->state != CADV_ENEMY_DEAD;
        cadv_scene_sprite(sc, &sp);
    }
    for (i = 0; i < CADV_FPS_MAX_PROJECTILES; ++i) {
        const cadv_projectile *pj = &f->proj[i];
        if (pj->active) {
            cadv_vec3 c = pj->remote ? cadv_v3(0.4f, 1.0f, 0.4f) : cadv_v3(1.5f, 0.6f, 0.2f);
            cadv_scene_draw(sc, &g->cube, cadv_m4_mul(cadv_m4_translate(pj->pos), cadv_m4_scale(cadv_v3(0.2f, 0.2f, 0.2f))),
                            cadv_material_make(0, cadv_v4(c.x, c.y, c.z, 1), 1.5f, CADV_MAT_NO_SHADOW));
            cadv_scene_light(sc, pj->pos, cadv_v3_scale(c, pj->remote ? 0.6f : 1.0f), 4.0f, false);
        }
    }
    for (i = 0; i < s->map.nlight; ++i) {
        const cadv_map_item *l = &s->map.light[i];
        cadv_vec3 col = l->hi;
        if (s->siphon_down && l->pos.y > 18.0f) {
            continue; /* the siphon's glow dies with it */
        }
        if (cadv_v3_dist(l->pos, cam->pos) > 40.0f) {
            continue;
        }
        if (s->collapse_ticks > 0 && ((s->ticks / 20) & 1)) {
            col = cadv_v3(col.x * 1.4f, col.y * 0.4f, col.z * 0.3f); /* alarm flicker */
        }
        cadv_scene_light(sc, l->pos, col, l->a, l->b > 0.5f);
    }
    if (f->muzzle_flash > 0.0f) {
        cadv_scene_light(sc, cadv_v3_add(cam->pos, cadv_v3_scale(cadv_player_forward(p), 0.6f)), cadv_v3(2.0f, 1.6f, 0.9f), 7.0f, false);
    }
    if (f->star_taken && s->win_timer == 0) {
        const cadv_map_item *ex = cadv_map_find_trigger(&s->map, "exit");
        if (ex) {
            cadv_vec3 c = cadv_v3(0.5f * (ex->pos.x + ex->hi.x), ex->pos.y, 0.5f * (ex->pos.z + ex->hi.z));
            cadv_scene_draw(sc, &g->beam, cadv_m4_translate(c), cadv_material_make(0, cadv_v4(0.4f, 1, 0.6f, 1), 0.6f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            cadv_scene_light(sc, cadv_v3_add(c, cadv_v3(0, 1.5f, 0)), cadv_v3(0.5f, 1.5f, 0.8f), 8.0f, false);
        }
    }
    if (ui) {
        draw_hud(s, ui, f);
    }
}

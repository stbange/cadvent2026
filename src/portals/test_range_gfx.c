/* SPDX-License-Identifier: MIT */
/* GPU side of the test range: procedural textures, voxel models and sprites. */
#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "gfx/font.h"
#include "gfx/gl_util.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "portals/test_range_gfx.h"
#include "portals/test_range_internal.h"

/* ---- rendering ---- */

static void build_room_mesh(cadv_mesh *m)
{
    cadv_rgba w = cadv_rgba_make(255, 255, 255, 255);
    cadv_mesh_init(m);
    /* walls are drawn as the inside of boxes */
    cadv_mesh_box(m, cadv_v3(-8, 0, -8), cadv_v3(8, 4, 8), 0.5f, w, true);
    cadv_mesh_box(m, cadv_v3(-4, 0, -14), cadv_v3(4, 4, -8), 0.5f, w, true);
    cadv_mesh_box(m, cadv_v3(-3, 0, -2), cadv_v3(-2, 2, 2), 0.5f, w, false);
    cadv_mesh_box(m, cadv_v3(2, 0, -2), cadv_v3(3, 1.2f, 2), 0.5f, w, false);
    cadv_mesh_box(m, cadv_v3(-1, 2.6f, -8.5f), cadv_v3(1, 4, -8), 0.5f, w, false);
}

/* ---- rendering --------------------------------------------------------------- */

static void make_imp(cadv_voxels *v)
{
    /* an original snowball imp: round snow body, coal eyes, red scarf, twig arms */
    int x, y, z;
    cadv_voxels_alloc(v, 12, 16, 12);
    v->pal[1] = cadv_rgba_make(235, 240, 255, 255); /* snow */
    v->pal[2] = cadv_rgba_make(20, 20, 26, 255);    /* coal */
    v->pal[3] = cadv_rgba_make(190, 30, 40, 255);   /* scarf */
    v->pal[4] = cadv_rgba_make(110, 70, 40, 255);   /* twigs */
    v->pal[5] = cadv_rgba_make(255, 140, 30, 255);  /* carrot */
    for (z = 0; z < 12; ++z) {
        for (y = 0; y < 16; ++y) {
            for (x = 0; x < 12; ++x) {
                float dx = (float)x - 5.5f;
                float dz = (float)z - 5.5f;
                float lower = dx * dx + ((float)y - 4.5f) * ((float)y - 4.5f) + dz * dz;
                float upper = dx * dx + ((float)y - 11.0f) * ((float)y - 11.0f) * 1.2f + dz * dz;
                if (lower < 26.0f || upper < 14.0f) {
                    cadv_voxels_set(v, x, y, z, 1);
                }
                if (y == 8 && dx * dx + dz * dz < 20.0f) {
                    cadv_voxels_set(v, x, y, z, 3);
                }
            }
        }
    }
    cadv_voxels_set(v, 4, 12, 9, 2);
    cadv_voxels_set(v, 7, 12, 9, 2);
    cadv_voxels_set(v, 5, 11, 10, 5);
    cadv_voxels_set(v, 6, 11, 10, 5);
    cadv_voxels_set(v, 5, 11, 11, 5);
    for (x = 0; x < 2; ++x) {
        cadv_voxels_set(v, x, 6 + x, 6, 4);
        cadv_voxels_set(v, 11 - x, 6 + x, 6, 4);
    }
    cadv_voxels_set(v, 3, 9, 4, 3); /* scarf tail */
    cadv_voxels_set(v, 3, 8, 3, 3);
}

static void make_star(cadv_voxels *v)
{
    /* five-pointed star, 2 voxels thick */
    int x, y, z;
    cadv_voxels_alloc(v, 15, 15, 3);
    v->pal[1] = cadv_rgba_make(255, 220, 90, 255);
    for (y = 0; y < 15; ++y) {
        for (x = 0; x < 15; ++x) {
            float px = (float)x - 7.0f;
            float py = (float)y - 7.0f;
            float a = cadv_atan2f(py, px) + 0.5f * CADV_PI;
            float r = cadv_sqrtf(px * px + py * py);
            float k = cadv_cosf(5.0f * a);
            float limit = 3.4f + 3.6f * (k > 0.0f ? k * k : 0.0f);
            if (r <= limit) {
                for (z = 0; z < 3; ++z) {
                    if (z == 1 || r < limit - 1.5f) {
                        cadv_voxels_set(v, x, y, z, 1);
                    }
                }
            }
        }
    }
}

static unsigned text_texture(const char *text)
{
    cadv_texdata t;
    unsigned tex;
    int w;
    int h;
    int i;
    int len = (int)strlen(text);
    if (len > 40) {
        len = 40;
    }
    w = 8 * len + 8;
    h = 16;
    cadv_texdata_alloc(&t, w, h);
    for (i = 0; i < w * h; ++i) {
        t.rgba[i * 4 + 0] = 40;
        t.rgba[i * 4 + 1] = 28;
        t.rgba[i * 4 + 2] = 20;
        t.rgba[i * 4 + 3] = 255;
    }
    for (i = 0; i < len; ++i) {
        int x;
        int y;
        for (y = 0; y < 7; ++y) {
            for (x = 0; x < 5; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x, y)) {
                    cadv_texdata_set(&t, 4 + i * 8 + x, 4 + y, 240, 200, 90, 255);
                }
            }
        }
    }
    tex = cadv_gl_texture(&t, false);
    cadv_texdata_free(&t);
    return tex;
}

void cadv_test_range_gpu_init(void *state)
{
    tr_state *s = (tr_state *)state;
    tr_gpu *g = &s->g;
    cadv_texdata t;
    cadv_voxels v;
    cadv_mesh m;
    cadv_texdata nrm;
    int i;
    uint32_t seed = (uint32_t)s->day * 977u;
    float hue = (float)(s->day % 6) / 6.0f;
    cadv_rgba wall = cadv_rgba_make((uint8_t)(90 + 60 * hue), 80, (uint8_t)(120 - 40 * hue), 255);
    build_room_mesh(&m);
    cadv_gpu_mesh_upload(&g->room, &m, false);
    cadv_mesh_free(&m);
    cadv_procgen_texture(&t, CADV_TEX_STONE_BRICKS, 64, seed, wall, cadv_rgba_make(30, 26, 30, 255));
    g->tex_wall = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    cadv_procgen_texture(&t, CADV_TEX_FLOOR_TILES, 64, seed + 1, cadv_rgba_make(150, 140, 130, 255), cadv_rgba_make(60, 60, 80, 255));
    g->tex_floor = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    cadv_procgen_texture(&t, CADV_TEX_METAL, 64, seed + 2, cadv_rgba_make(120, 90, 60, 255), cadv_rgba_make(220, 200, 120, 255));
    g->tex_door = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    cadv_mesh_init(&m);
    cadv_mesh_quad(&m, cadv_v3(-8, 0.01f, 8), cadv_v3(8, 0.01f, 8), cadv_v3(8, 0.01f, -14), cadv_v3(-8, 0.01f, -14), 8, 11,
                   cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->floor, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_box(&m, cadv_v3(-1, 0, -0.15f), cadv_v3(1, 2.6f, 0.15f), 0.5f, cadv_rgba_make(255, 255, 255, 255), false);
    cadv_gpu_mesh_upload(&g->door, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_prism(&m, cadv_v3(0, 0, 0), 0.6f, 0.8f, 8, 0.5f, cadv_rgba_make(200, 190, 170, 255));
    cadv_gpu_mesh_upload(&g->pedestal, &m, false);
    cadv_mesh_clear(&m);
    cadv_mesh_quad(&m, cadv_v3(-1.5f, -0.25f, 0), cadv_v3(1.5f, -0.25f, 0), cadv_v3(1.5f, 0.25f, 0),
                   cadv_v3(-1.5f, 0.25f, 0), 1, 1, cadv_rgba_make(255, 255, 255, 255));
    cadv_gpu_mesh_upload(&g->plaque, &m, false);
    cadv_mesh_free(&m);
    make_star(&v);
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.06f, &m);
    cadv_gpu_mesh_upload(&g->star, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    cadv_voxels_alloc(&v, 6, 2, 2);
    v.pal[1] = cadv_rgba_make(255, 210, 60, 255);
    for (i = 0; i < 6; ++i) {
        cadv_voxels_set(&v, i, 0, 0, 1);
    }
    cadv_voxels_set(&v, 0, 1, 0, 1);
    cadv_voxels_set(&v, 4, 1, 0, 1);
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.08f, &m);
    cadv_gpu_mesh_upload(&g->key, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
    make_imp(&v);
    g->imp_dirs = 8;
    cadv_voxels_bake_sprites(&v, g->imp_dirs, 4, &t, &nrm, &g->imp_fw, &g->imp_fh);
    g->tex_imp = cadv_gl_texture(&t, false);
    g->tex_imp_n = cadv_gl_texture(&nrm, false);
    cadv_texdata_free(&t);
    cadv_texdata_free(&nrm);
    cadv_voxels_free(&v);
    for (i = 0; i < s->plaque_count; ++i) {
        g->tex_plaque[i] = text_texture(s->plaque_text[i]);
    }
    s->gpu_ready = true;
}

void cadv_test_range_gpu_free(void *state)
{
    tr_state *s = (tr_state *)state;
    tr_gpu *g = &s->g;
    int i;
    cadv_gpu_mesh_free(&g->room);
    cadv_gpu_mesh_free(&g->floor);
    cadv_gpu_mesh_free(&g->door);
    cadv_gpu_mesh_free(&g->star);
    cadv_gpu_mesh_free(&g->key);
    cadv_gpu_mesh_free(&g->pedestal);
    cadv_gpu_mesh_free(&g->plaque);
    {
        unsigned texs[5];
        texs[0] = g->tex_wall;
        texs[1] = g->tex_floor;
        texs[2] = g->tex_door;
        texs[3] = g->tex_imp;
        texs[4] = g->tex_imp_n;
        cadv_gl_delete_textures(5, texs);
    }
    for (i = 0; i < s->plaque_count; ++i) {
        cadv_gl_delete_textures(1, &g->tex_plaque[i]);
    }
    s->gpu_ready = false;
}

void cadv_test_range_draw(void *state, cadv_scene *sc, cadv_camera *cam, cadv_ui *ui, float alpha)
{
    tr_state *s = (tr_state *)state;
    tr_gpu *g = &s->g;
    const cadv_fps *f = &s->fps;
    const cadv_player *p = &f->player;
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float t = (float)s->ticks * DT + alpha * DT;
    char buf[96];
    int i;
    (void)alpha;
    cam->pos = cadv_player_eye(p);
    cam->yaw = p->yaw;
    cam->pitch = p->pitch;
    cam->fovy = 1.25f;
    cam->znear = 0.05f;
    cam->zfar = 60.0f;
    sc->ambient = cadv_v3(0.18f, 0.17f, 0.22f);
    sc->clear_color = cadv_v3(0.02f, 0.02f, 0.04f);
    sc->fog_color = cadv_v3(0.02f, 0.02f, 0.05f);
    sc->fog_near = 6.0f;
    sc->fog_far = 26.0f;
    sc->light_bands = 6;
    sc->dither = 0.6f;
    sc->use_palette = true;
    /* room: floor/walls share one mesh; tint by texture */
    cadv_scene_draw(sc, &g->room, cadv_m4_identity(), cadv_material_make(g->tex_wall, white, 0.0f, 0));
    cadv_scene_draw(sc, &g->floor, cadv_m4_identity(), cadv_material_make(g->tex_floor, white, 0.0f, 0));
    if (f->doors[0].open < 1.0f) {
        const cadv_solid *ds = &f->world.s[f->doors[0].solid];
        cadv_scene_draw(sc, &g->door, cadv_m4_translate(cadv_v3(0, ds->box.lo.y, -8.25f)),
                        cadv_material_make(g->tex_door, white, 0.0f, 0));
    }
    cadv_scene_draw(sc, &g->pedestal, cadv_m4_translate(cadv_v3(6.0f, 0.0f, 4.0f)),
                    cadv_material_make(0, cadv_v4(0.8f, 0.75f, 0.7f, 1), 0.0f, 0));
    for (i = 0; i < f->pickup_count; ++i) {
        const cadv_pickup *pk = &f->pickups[i];
        cadv_mat4 xf;
        if (!pk->active) {
            continue;
        }
        xf = cadv_m4_mul(cadv_m4_translate(cadv_v3(pk->pos.x, pk->pos.y + 0.1f * cadv_sinf(t * 2.0f), pk->pos.z)),
                         cadv_m4_rotate_y(t * 1.5f));
        if (pk->kind == CADV_PICKUP_STAR) {
            cadv_scene_draw(sc, &g->star, xf, cadv_material_make(0, white, 0.9f, CADV_MAT_NO_SHADOW));
            cadv_scene_light(sc, pk->pos, cadv_v3(1.6f, 1.3f, 0.5f), 6.0f, false);
        } else if (pk->kind == CADV_PICKUP_KEY) {
            cadv_scene_draw(sc, &g->key, xf, cadv_material_make(0, white, 0.5f, 0));
        } else {
            cadv_scene_draw(sc, &g->key, cadv_m4_mul(xf, cadv_m4_scale(cadv_v3(1.5f, 1.5f, 1.5f))),
                            cadv_material_make(0, pk->kind == CADV_PICKUP_HEALTH ? cadv_v4(1, 0.2f, 0.2f, 1) : cadv_v4(0.4f, 0.6f, 1, 1), 0.3f, 0));
        }
    }
    for (i = 0; i < s->plaque_count; ++i) {
        cadv_mat4 xf = cadv_m4_mul(cadv_m4_translate(s->plaque_pos[i]), cadv_m4_rotate_y(s->plaque_yaw[i]));
        cadv_scene_draw(sc, &g->plaque, xf, cadv_material_make(g->tex_plaque[i], white, 0.2f, CADV_MAT_NO_SHADOW));
    }
    for (i = 0; i < f->enemy_count; ++i) {
        const cadv_enemy *e = &f->enemies[i];
        cadv_sprite sp;
        int frame;
        float squash = e->state == CADV_ENEMY_DEAD ? 0.25f : 1.0f;
        memset(&sp, 0, sizeof sp);
        frame = cadv_sprite_frame(e->pos, e->yaw, cam->pos, g->imp_dirs);
        sp.pos = e->pos;
        sp.w = (float)g->imp_fw / 4.0f * 0.11f;
        sp.h = (float)g->imp_fh / 4.0f * 0.11f * squash;
        sp.tex = g->tex_imp;
        sp.normal_tex = g->tex_imp_n;
        sp.u0 = (float)frame / (float)g->imp_dirs;
        sp.u1 = (float)(frame + 1) / (float)g->imp_dirs;
        sp.v0 = 0.0f;
        sp.v1 = 1.0f;
        sp.tint = e->state == CADV_ENEMY_PAIN ? cadv_v4(1.0f, 0.5f, 0.5f, 1.0f)
                                              : (e->type == 1 ? cadv_v4(1.0f, 0.85f, 0.85f, 1) : white);
        sp.casts_shadow = e->state != CADV_ENEMY_DEAD;
        cadv_scene_sprite(sc, &sp);
    }
    for (i = 0; i < CADV_FPS_MAX_PROJECTILES; ++i) {
        if (f->proj[i].active) {
            cadv_scene_light(sc, f->proj[i].pos, cadv_v3(1.5f, 0.6f, 0.2f), 4.0f, false);
            cadv_scene_draw(sc, &g->key, cadv_m4_translate(f->proj[i].pos), cadv_material_make(0, cadv_v4(1, 0.5f, 0.1f, 1), 2.0f, CADV_MAT_NO_SHADOW));
        }
    }
    /* lamps: two shadow-casting ceiling lights + flicker */
    cadv_scene_light(sc, cadv_v3(-3.0f, 3.5f, 0.0f), cadv_v3(1.4f, 1.1f, 0.8f), 12.0f, true);
    cadv_scene_light(sc, cadv_v3(3.5f, 3.5f, 4.0f), cadv_v3(0.9f + 0.1f * cadv_sinf(t * 13.0f), 0.9f, 1.2f), 11.0f, true);
    cadv_scene_light(sc, cadv_v3(0.0f, 3.0f, -11.0f), cadv_v3(1.2f, 1.0f, 0.6f), 8.0f, true);
    if (f->muzzle_flash > 0.0f) {
        cadv_scene_light(sc, cadv_v3_add(cam->pos, cadv_v3_scale(cadv_player_forward(p), 0.6f)), cadv_v3(2.0f, 1.6f, 0.9f), 7.0f, false);
    }
    /* HUD */
    if (ui) {
        float cx = (float)ui->width * 0.5f;
        float cy = (float)ui->height * 0.5f;
        const cadv_weapon_def *w = cadv_fps_weapon(f, f->weapon);
        cadv_ui_rect(ui, cx - 1, cy - 4, 2, 3, white);
        cadv_ui_rect(ui, cx - 1, cy + 2, 2, 3, white);
        cadv_ui_rect(ui, cx - 5, cy, 3, 1, white);
        cadv_ui_rect(ui, cx + 3, cy, 3, 1, white);
        snprintf(buf, sizeof buf, "HP %d  AR %d", p->health, p->armor);
        cadv_ui_text(ui, 4, (float)ui->height - 12, 1, cadv_v4(1, 0.4f, 0.4f, 1), buf);
        snprintf(buf, sizeof buf, "%s %d", w->name, w->ammo_per_shot ? f->ammo[w->ammo_type] : 99);
        cadv_ui_text(ui, (float)ui->width - 6.0f * (float)strlen(buf) - 4, (float)ui->height - 12, 1, cadv_v4(1, 0.9f, 0.5f, 1), buf);
        snprintf(buf, sizeof buf, "DAY %d TEST RANGE  %s", s->day, (p->keys & KEY_BIT) ? "[KEY]" : "");
        cadv_ui_text(ui, 4, 4, 1, cadv_v4(0.8f, 0.85f, 1, 1), buf);
        if (s->message_timer > 0) {
            cadv_ui_text_centered(ui, cx, cy + 24, 1, cadv_v4(1, 1, 0.6f, 1), s->message);
        }
        if (s->win_timer > 0) {
            cadv_ui_rect(ui, 0, cy - 22, (float)ui->width, 40, cadv_v4(0, 0, 0, 0.6f));
            cadv_ui_text_centered(ui, cx, cy - 16, 2, cadv_v4(1, 0.85f, 0.3f, 1), "SHINING STAR!");
            cadv_ui_text_centered(ui, cx, cy + 4, 1, white, "PRESS USE TO RETURN TO THE HALL");
        }
        if (s->death_timer > 0) {
            cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0.3f, 0.5f, 0.9f, 0.35f));
        }
    }
}

/* SPDX-License-Identifier: MIT */
#include "hub/hub_gfx.h"

#include <stdio.h>
#include <string.h>

#include "core/mathx.h"
#include "core/rng.h"
#include "gfx/font.h"
#include "gfx/procgen.h"
#include "gfx/voxel.h"
#include "hub/layout.h"
#include "portal/portal.h"

static const cadv_rgba WHITE = {255, 255, 255, 255};

static unsigned tex_from(cadv_tex_kind k, uint32_t seed, cadv_rgba a, cadv_rgba b)
{
    cadv_texdata t;
    unsigned tex;
    cadv_procgen_texture(&t, k, 64, seed, a, b);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

static unsigned text_tex(const char *text, int scale, cadv_rgba fg, cadv_rgba bg)
{
    cadv_texdata t;
    unsigned tex;
    int len = (int)strlen(text);
    int w = (6 * len + 3) * scale;
    int h = 11 * scale;
    int i;
    int x;
    int y;
    cadv_texdata_alloc(&t, w, h);
    for (i = 0; i < w * h; ++i) {
        t.rgba[i * 4 + 0] = bg.r;
        t.rgba[i * 4 + 1] = bg.g;
        t.rgba[i * 4 + 2] = bg.b;
        t.rgba[i * 4 + 3] = bg.a;
    }
    for (i = 0; i < len; ++i) {
        for (y = 0; y < 7 * scale; ++y) {
            for (x = 0; x < 5 * scale; ++x) {
                if (cadv_font_pixel((unsigned char)text[i], x / scale, y / scale)) {
                    cadv_texdata_set(&t, (2 + i * 6) * scale + x, 2 * scale + y, fg.r, fg.g, fg.b, fg.a);
                }
            }
        }
    }
    tex = cadv_gl_texture(&t, false);
    cadv_texdata_free(&t);
    return tex;
}

static void upload(cadv_gpu_mesh *g, cadv_mesh *m)
{
    cadv_gpu_mesh_upload(g, m, false);
    cadv_mesh_clear(m);
}

static void wall(cadv_mesh *m, cadv_vec3 a, cadv_vec3 b, cadv_vec3 c, cadv_vec3 d)
{
    cadv_mesh_quad(m, a, b, c, d, cadv_v3_dist(a, b) * 0.25f, cadv_v3_dist(a, d) * 0.25f, WHITE);
}

static void build_shell(cadv_hub_gfx *g, cadv_mesh *m)
{
    float W = CADV_HALL_W;
    float H = CADV_HALL_HEIGHT;
    int i;
    int k;
    /* inward-facing wall quads; the north wall leaves an opening into the apse */
    float N = CADV_HALL_NORTH;
    float S = CADV_HALL_SOUTH;
    float A = CADV_APSE_NORTH;
    float h6 = CADV_APSE_HALF;
    wall(m, cadv_v3(W, 0, S), cadv_v3(-W, 0, S), cadv_v3(-W, H, S), cadv_v3(W, H, S));          /* south, faces -Z */
    wall(m, cadv_v3(-W, 0, S), cadv_v3(-W, 0, N), cadv_v3(-W, H, N), cadv_v3(-W, H, S));        /* west, faces +X */
    wall(m, cadv_v3(W, 0, N), cadv_v3(W, 0, S), cadv_v3(W, H, S), cadv_v3(W, H, N));            /* east, faces -X */
    wall(m, cadv_v3(-W, 0, N), cadv_v3(-h6, 0, N), cadv_v3(-h6, H, N), cadv_v3(-W, H, N));      /* north left, +Z */
    wall(m, cadv_v3(h6, 0, N), cadv_v3(W, 0, N), cadv_v3(W, H, N), cadv_v3(h6, H, N));          /* north right, +Z */
    wall(m, cadv_v3(-h6, 0, A), cadv_v3(h6, 0, A), cadv_v3(h6, H, A), cadv_v3(-h6, H, A));      /* apse back, +Z */
    wall(m, cadv_v3(-h6, 0, N), cadv_v3(-h6, 0, A), cadv_v3(-h6, H, A), cadv_v3(-h6, H, N));    /* apse west, +X */
    wall(m, cadv_v3(h6, 0, A), cadv_v3(h6, 0, N), cadv_v3(h6, H, N), cadv_v3(h6, H, A));        /* apse east, -X */
    wall(m, cadv_v3(-W, H, A), cadv_v3(W, H, A), cadv_v3(W, H, S), cadv_v3(-W, H, S));          /* ceiling, faces -Y */
    upload(&g->shell, m);
    cadv_mesh_quad(m, cadv_v3(-W, 0.01f, CADV_HALL_SOUTH), cadv_v3(W, 0.01f, CADV_HALL_SOUTH),
                   cadv_v3(W, 0.01f, CADV_APSE_NORTH), cadv_v3(-W, 0.01f, CADV_APSE_NORTH), 2.0f * W / 2.0f,
                   (CADV_HALL_SOUTH - CADV_APSE_NORTH) / 2.0f, WHITE);
    upload(&g->floor, m);
    /* balconies, railings and stairs */
    for (i = 0; i < 2; ++i) {
        float s = i == 0 ? -1.0f : 1.0f;
        float inner = s * (W - CADV_BALCONY_DEPTH);
        float lo = s < 0 ? -W : inner;
        float hi = s < 0 ? inner : W;
        cadv_mesh_box(m, cadv_v3(lo, CADV_BALCONY_Y - 0.3f, CADV_HALL_NORTH), cadv_v3(hi, CADV_BALCONY_Y, 0.0f), 0.5f, WHITE, false);
        cadv_mesh_box(m, cadv_v3(s < 0 ? inner - 0.25f : inner, CADV_BALCONY_Y, CADV_HALL_NORTH),
                      cadv_v3(s < 0 ? inner : inner + 0.25f, CADV_BALCONY_Y + 1.0f, 0.0f), 0.5f, WHITE, false);
        for (k = 0; k < 16; ++k) {
            float z0 = 6.0f - (float)k * 0.375f;
            cadv_mesh_box(m, cadv_v3(lo, 0.0f, z0 - 0.375f), cadv_v3(hi, 0.25f * (float)(k + 1), z0), 0.5f, WHITE, false);
        }
    }
    upload(&g->balconies, m);
    for (i = 0; i < 2; ++i) {
        float s = i == 0 ? -1.0f : 1.0f;
        for (k = 0; k < 5; ++k) {
            float cz = -6.5f - 5.0f * (float)k;
            float cx = s * (W - CADV_BALCONY_DEPTH - 0.6f);
            cadv_mesh_prism(m, cadv_v3(cx, 0, cz), 0.5f, H, 8, 0.5f, WHITE);
        }
    }
    upload(&g->columns, m);
}

static void build_props(cadv_hub_gfx *g, cadv_mesh *m)
{
    int day;
    cadv_mesh one;
    cadv_mesh_init(&one);
    /* all 24 arches in one static mesh */
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        cadv_portal_slot s = cadv_hall_portal(day);
        float scale = day == 24 ? 1.6f : 1.0f;
        cadv_mat4 xf = cadv_m4_mul(cadv_m4_translate(cadv_v3_add(s.base, cadv_v3_scale(s.normal, 0.25f))),
                                   cadv_m4_mul(cadv_m4_rotate_y(s.yaw + CADV_PI), cadv_m4_scale(cadv_v3(scale, scale, scale))));
        cadv_mesh_clear(&one);
        cadv_mesh_arch(&one, cadv_v3(0, 0, 0), CADV_ARCH_WIDTH, CADV_ARCH_HEIGHT, 0.45f, 0.5f, 10,
                       cadv_rgba_make(210, 190, 150, 255));
        cadv_mesh_append(m, &one, xf);
    }
    upload(&g->arches, m);
    cadv_mesh_free(&one);
    /* ice block / membrane / plate in local portal space (facing +Z) */
    cadv_mesh_box(m, cadv_v3(-1.2f, 0, -0.1f), cadv_v3(1.2f, 3.4f, 0.35f), 0.6f, WHITE, false);
    upload(&g->ice, m);
    {
        int i;
        uint32_t c = cadv_mesh_vertex(m, cadv_v3(0, 1.7f, 0), cadv_v3(0, 0, 1), 0.5f, 0.5f, WHITE);
        for (i = 0; i <= 24; ++i) {
            float a = CADV_TAU * (float)i / 24.0f;
            float rx = 1.2f;
            float ry = 1.7f;
            cadv_mesh_vertex(m, cadv_v3(rx * cadv_cosf(a), 1.7f + ry * cadv_sinf(a), 0), cadv_v3(0, 0, 1),
                             0.5f + 0.5f * cadv_cosf(a), 0.5f + 0.5f * cadv_sinf(a), WHITE);
            if (i > 0) {
                cadv_mesh_tri(m, c, c + (uint32_t)i, c + (uint32_t)i + 1);
            }
        }
    }
    upload(&g->membrane, m);
    cadv_mesh_quad(m, cadv_v3(-0.4f, -0.22f, 0), cadv_v3(0.4f, -0.22f, 0), cadv_v3(0.4f, 0.22f, 0),
                   cadv_v3(-0.4f, 0.22f, 0), 1, 1, WHITE);
    upload(&g->plate, m);
    cadv_mesh_quad(m, cadv_v3(-0.7f, -0.3f, 0), cadv_v3(0.7f, -0.3f, 0), cadv_v3(0.7f, 0.3f, 0),
                   cadv_v3(-0.7f, 0.3f, 0), 1, 1, WHITE);
    upload(&g->plaque, m);
    cadv_mesh_quad(m, cadv_v3(-0.9f, -2.5f, 0), cadv_v3(0.9f, -2.5f, 0), cadv_v3(0.9f, 2.5f, 0),
                   cadv_v3(-0.9f, 2.5f, 0), 1, 3, WHITE);
    upload(&g->banner, m);
    cadv_mesh_prism(m, cadv_v3(0, 0, 0), 0.05f, 0.3f, 6, 0.0f, cadv_rgba_make(240, 230, 200, 255));
    upload(&g->candle, m);
    cadv_mesh_box(m, cadv_v3(-0.08f, 0, -0.08f), cadv_v3(0.08f, 0.16f, 0.08f), 0.0f, WHITE, false);
    upload(&g->socket, m);
    cadv_mesh_prism(m, cadv_v3(0, 0, 0), 0.35f, 30.0f, 6, 0.0f, WHITE);
    upload(&g->beam, m);
    cadv_mesh_box(m, cadv_v3(-0.03f, -0.03f, -0.03f), cadv_v3(0.03f, 0.03f, 0.03f), 0.0f, WHITE, false);
    upload(&g->flake, m);
    {
        /* wreath: ring of small boxes */
        int i;
        for (i = 0; i < 16; ++i) {
            float a = CADV_TAU * (float)i / 16.0f;
            cadv_vec3 c = cadv_v3(1.1f * cadv_cosf(a), 0, 1.1f * cadv_sinf(a));
            cadv_mesh_box(m, cadv_v3_sub(c, cadv_v3(0.25f, 0.15f, 0.25f)), cadv_v3_add(c, cadv_v3(0.25f, 0.15f, 0.25f)), 0.0f,
                          cadv_rgba_make(30, 110, 50, 255), false);
        }
    }
    upload(&g->wreath, m);
}

static void make_star_voxels(cadv_voxels *v)
{
    int x;
    int y;
    int z;
    cadv_voxels_alloc(v, 15, 15, 3);
    v->pal[1] = cadv_rgba_make(255, 225, 100, 255);
    for (y = 0; y < 15; ++y) {
        for (x = 0; x < 15; ++x) {
            float px = (float)x - 7.0f;
            float py = (float)y - 7.0f;
            float a = cadv_atan2f(py, px) + 0.5f * CADV_PI;
            float r = cadv_sqrtf(px * px + py * py);
            float k = cadv_cosf(5.0f * a);
            float limit = 3.4f + 3.6f * (k > 0.0f ? k * k : 0.0f);
            for (z = 0; z < 3 && r <= limit; ++z) {
                if (z == 1 || r < limit - 1.5f) {
                    cadv_voxels_set(v, x, y, z, 1);
                }
            }
        }
    }
}

/* The great fir: stacked voxel cones, one ornament per completed portal and
 * the crowning star once the 24th star is placed. */
static void build_tree(cadv_hub_gfx *g, const cadv_hub *h)
{
    cadv_voxels v;
    cadv_mesh m;
    int x;
    int y;
    int z;
    int day;
    const int W = 48;
    const int H = 96;
    cadv_rng rng;
    cadv_voxels_alloc(&v, W, H, W);
    v.pal[1] = cadv_rgba_make(18, 70, 34, 255);
    v.pal[2] = cadv_rgba_make(28, 95, 44, 255);
    v.pal[3] = cadv_rgba_make(80, 50, 30, 255);
    v.pal[4] = cadv_rgba_make(240, 245, 255, 255);
    cadv_rng_seed(&rng, 2026u, 12u);
    for (y = 0; y < H; ++y) {
        float t = (float)y / (float)H;
        float layer = (float)(y % 16) / 16.0f; /* each tier flares out at its bottom */
        float radius = (1.0f - t) * 22.0f * (0.65f + 0.35f * (1.0f - layer)) + 1.0f;
        for (z = 0; z < W; ++z) {
            for (x = 0; x < W; ++x) {
                float dx = (float)x - 23.5f;
                float dz = (float)z - 23.5f;
                float d = cadv_sqrtf(dx * dx + dz * dz);
                if (y < 10 && d < 2.5f) {
                    cadv_voxels_set(&v, x, y, z, 3);
                } else if (y >= 8 && d < radius) {
                    uint32_t r = cadv_rng_next(&rng);
                    bool snow = d > radius - 1.5f && (r & 7u) == 0u && layer < 0.4f;
                    cadv_voxels_set(&v, x, y, z, snow ? 4 : (uint8_t)(1 + (r & 1u)));
                }
            }
        }
    }
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        if (h->progress->p[day - 1].completed) {
            float a = (float)day * 2.399963f; /* golden angle spiral */
            int oy = 14 + (day * 3) % 70;
            float r = (1.0f - (float)oy / (float)H) * 22.0f * 0.8f;
            int ox = (int)(23.5f + r * cadv_cosf(a));
            int oz = (int)(23.5f + r * cadv_sinf(a));
            uint8_t idx = (uint8_t)(10 + day);
            v.pal[idx] = cadv_rgba_make((uint8_t)(120 + (day * 53) % 136), (uint8_t)(60 + (day * 97) % 160),
                                        (uint8_t)(60 + (day * 29) % 180), 255);
            for (y = -1; y <= 1; ++y) {
                for (x = -1; x <= 1; ++x) {
                    cadv_voxels_set(&v, ox + x, oy + y, oz, idx);
                    cadv_voxels_set(&v, ox, oy + y, oz + x, idx);
                }
            }
        }
    }
    cadv_mesh_init(&m);
    cadv_voxels_mesh(&v, 0.1f, &m);
    cadv_gpu_mesh_upload(&g->tree, &m, false);
    cadv_mesh_free(&m);
    cadv_voxels_free(&v);
}

static int tree_version(const cadv_hub *h)
{
    int day;
    unsigned v = 0;
    for (day = 0; day < CADV_PORTAL_COUNT; ++day) {
        if (h->progress->p[day].completed) {
            v |= 1u << day;
        }
    }
    return (int)v;
}

void cadv_hub_gfx_init(cadv_hub_gfx *g, const cadv_hub *h)
{
    cadv_mesh m;
    cadv_voxels v;
    int i;
    char buf[48];
    memset(g, 0, sizeof *g);
    cadv_mesh_init(&m);
    build_shell(g, &m);
    build_props(g, &m);
    make_star_voxels(&v);
    cadv_voxels_mesh(&v, 0.05f, &m);
    upload(&g->star, &m);
    cadv_voxels_free(&v);
    cadv_mesh_free(&m);
    g->tex_stone = tex_from(CADV_TEX_STONE_BRICKS, 11u, cadv_rgba_make(78, 76, 84, 255), cadv_rgba_make(22, 20, 26, 255));
    g->tex_floor = tex_from(CADV_TEX_FLOOR_TILES, 12u, cadv_rgba_make(150, 140, 125, 255), cadv_rgba_make(40, 36, 44, 255));
    g->tex_wood = tex_from(CADV_TEX_PLANKS, 13u, cadv_rgba_make(96, 60, 36, 255), cadv_rgba_make(30, 18, 10, 255));
    g->tex_ice = tex_from(CADV_TEX_ICE, 14u, cadv_rgba_make(120, 190, 230, 255), cadv_rgba_make(220, 245, 255, 255));
    g->tex_cloth = tex_from(CADV_TEX_CLOTH, 15u, cadv_rgba_make(150, 22, 30, 255), cadv_rgba_make(220, 180, 70, 255));
    g->tex_gold = tex_from(CADV_TEX_METAL, 16u, cadv_rgba_make(200, 160, 70, 255), cadv_rgba_make(255, 230, 140, 255));
    g->tex_swirl = tex_from(CADV_TEX_NOISE, 17u, cadv_rgba_make(40, 10, 80, 255), cadv_rgba_make(120, 230, 255, 255));
    g->tex_snow = tex_from(CADV_TEX_PLASTER, 18u, cadv_rgba_make(230, 235, 255, 255), cadv_rgba_make(200, 210, 240, 255));
    for (i = 0; i < CADV_PORTAL_COUNT; ++i) {
        snprintf(buf, sizeof buf, "%d", i + 1);
        g->tex_day[i] = text_tex(buf, 2, cadv_rgba_make(255, 220, 120, 255), cadv_rgba_make(40, 26, 14, 255));
    }
    for (i = 0; i < 6 && i < h->friends->count; ++i) {
        char up[CADV_FRIEND_NAME];
        int k;
        for (k = 0; h->friends->name[i][k] && k < CADV_FRIEND_NAME - 1; ++k) {
            char c = h->friends->name[i][k];
            up[k] = (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
        }
        up[k] = '\0';
        g->tex_legend[i] = text_tex(up, 1, cadv_rgba_make(240, 210, 120, 255), cadv_rgba_make(30, 20, 14, 255));
    }
    build_tree(g, h);
    g->tree_version = tree_version(h);
    cadv_mesh_init(&g->batch);
    for (i = 0; i < CADV_HUB_SNOW; ++i) {
        float a = (float)i * 2.399963f;
        float r = 6.0f * cadv_sqrtf((float)i / (float)CADV_HUB_SNOW);
        g->snow[i] = cadv_v3(r * cadv_cosf(a), (float)((i * 37) % 120) / 10.0f, -16.0f + r * cadv_sinf(a));
    }
    g->ready = true;
}

void cadv_hub_gfx_free(cadv_hub_gfx *g)
{
    cadv_gpu_mesh *meshes[] = {&g->shell, &g->floor, &g->balconies, &g->columns, &g->arches, &g->ice,
                               &g->membrane, &g->plate, &g->tree, &g->star, &g->socket, &g->banner,
                               &g->plaque, &g->candle, &g->wreath, &g->beam, &g->flake, &g->batch_gpu};
    unsigned texs[8];
    size_t i;
    if (!g->ready) {
        return;
    }
    for (i = 0; i < sizeof meshes / sizeof meshes[0]; ++i) {
        cadv_gpu_mesh_free(meshes[i]);
    }
    texs[0] = g->tex_stone;
    texs[1] = g->tex_floor;
    texs[2] = g->tex_wood;
    texs[3] = g->tex_ice;
    texs[4] = g->tex_cloth;
    texs[5] = g->tex_gold;
    texs[6] = g->tex_swirl;
    texs[7] = g->tex_snow;
    cadv_gl_delete_textures(8, texs);
    cadv_gl_delete_textures(CADV_PORTAL_COUNT, g->tex_day);
    cadv_gl_delete_textures(6, g->tex_legend);
    cadv_mesh_free(&g->batch);
    memset(g, 0, sizeof *g);
}

static cadv_vec3 portal_color(int day)
{
    static const cadv_vec3 pal[6] = {{0.6f, 0.3f, 1.0f}, {0.2f, 0.8f, 1.0f}, {1.0f, 0.5f, 0.2f},
                                     {0.3f, 1.0f, 0.5f}, {1.0f, 0.3f, 0.4f}, {1.0f, 0.9f, 0.3f}};
    return pal[(day - 1) % 6];
}

static cadv_mat4 portal_xf(int day, float out)
{
    cadv_portal_slot s = cadv_hall_portal(day);
    float scale = day == 24 ? 1.6f : 1.0f;
    return cadv_m4_mul(cadv_m4_translate(cadv_v3_add(s.base, cadv_v3_scale(s.normal, out))),
                       cadv_m4_mul(cadv_m4_rotate_y(s.yaw + CADV_PI), cadv_m4_scale(cadv_v3(scale, scale, scale))));
}

static void draw_hud(const cadv_hub *h, const cadv_input *input, cadv_ui *ui, bool dev_marker)
{
    char buf[96];
    cadv_vec4 gold = cadv_v4(1.0f, 0.85f, 0.45f, 1.0f);
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float cx = (float)ui->width * 0.5f;
    int i;
    snprintf(buf, sizeof buf, "STARS %d/23  CARRIED %d  SNOWFLAKES %d", cadv_progress_stars_socketed(h->progress),
             cadv_progress_stars_carried(h->progress), cadv_progress_total_snowflakes(h->progress));
    cadv_ui_text(ui, 4, 4, 1, gold, buf);
    if (dev_marker) {
        cadv_ui_text(ui, (float)ui->width - 26, 4, 1, cadv_v4(1, 0.3f, 0.3f, 1), "DEV");
    }
    cadv_ui_rect(ui, cx - 1, (float)ui->height * 0.5f - 1, 2, 2, cadv_v4(1, 1, 1, 0.7f));
    if (h->prompt[0]) {
        cadv_ui_rect(ui, 0, (float)ui->height - 34, (float)ui->width, 12, cadv_v4(0, 0, 0, 0.5f));
        cadv_ui_text_centered(ui, cx, (float)ui->height - 32, 1, white, h->prompt);
    }
    if (h->message_timer > 0) {
        cadv_ui_text_centered(ui, cx, (float)ui->height * 0.5f + 20, 1, gold, h->message);
    }
    if (h->epilogue_timer > 0) {
        static const char *const lines[] = {
            "THE STAR IS SET", "TWENTY-FOUR NIGHTS, TWENTY-FOUR WORLDS", "",
            "FOR THE CREW", "", "MERRY CHRISTMAS"};
        int t = CADV_HUB_EPILOGUE_TICKS - h->epilogue_timer;
        cadv_ui_rect(ui, 0, 0, (float)ui->width, (float)ui->height, cadv_v4(0, 0, 0.05f, cadv_clampf((float)t / 120.0f, 0, 0.55f)));
        for (i = 0; i < 6; ++i) {
            float y = (float)ui->height - (float)t * 0.25f + (float)i * 14.0f;
            cadv_ui_text_centered(ui, cx, y, 1, gold, lines[i]);
        }
        for (i = 0; i < h->friends->count; ++i) {
            float y = (float)ui->height - (float)t * 0.25f + 90.0f + (float)i * 12.0f;
            cadv_ui_text_centered(ui, cx, y, 1, white, h->friends->name[i]);
        }
    }
    (void)input; /* touch buttons are drawn by the app over every view */
}

void cadv_hub_gfx_draw(cadv_hub_gfx *g, const cadv_hub *h, const cadv_input *input, cadv_scene *sc,
                       cadv_camera *cam, cadv_ui *ui, float alpha, bool dev_marker)
{
    cadv_vec4 white = cadv_v4(1, 1, 1, 1);
    float t = ((float)h->ticks + alpha) / 60.0f;
    int day;
    int i;
    int beams[24];
    int nb;
    cadv_mesh_clear(&g->batch);
    if (tree_version(h) != g->tree_version) {
        cadv_gpu_mesh_free(&g->tree);
        build_tree(g, h);
        g->tree_version = tree_version(h);
    }
    cam->pos = cadv_player_eye(&h->player);
    cam->yaw = h->player.yaw;
    cam->pitch = h->player.pitch;
    cam->fovy = 1.2f;
    cam->znear = 0.05f;
    cam->zfar = 90.0f;
    if (h->epilogue_timer > 0) {
        /* fly up past the tree towards the open roof */
        float k = (float)(CADV_HUB_EPILOGUE_TICKS - h->epilogue_timer) / (float)CADV_HUB_EPILOGUE_TICKS;
        cam->pos = cadv_v3(10.0f * cadv_cosf(k * 3.0f), 2.0f + 9.0f * k, -16.0f + 10.0f * cadv_sinf(k * 3.0f));
        cam->yaw = cadv_atan2f(cam->pos.x, cam->pos.z + 16.0f);
        cam->pitch = 0.2f * k;
    }
    sc->clear_color = cadv_v3(0.01f, 0.01f, 0.03f);
    sc->ambient = cadv_v3(0.10f, 0.10f, 0.15f);
    sc->sun_dir = cadv_v3_norm(cadv_v3(0.15f, -1.0f, 0.1f)); /* moonlight through the skylight */
    sc->sun_color = cadv_v3(0.18f, 0.22f, 0.38f);
    sc->sun_shadow = true;
    sc->shadow_center = cadv_v3(cam->pos.x, 0, cam->pos.z - 6.0f);
    sc->shadow_extent = 24.0f;
    sc->fog_color = cadv_v3(0.02f, 0.02f, 0.05f);
    sc->fog_near = 14.0f;
    sc->fog_far = 55.0f;
    sc->light_bands = 8;
    sc->dither = 0.5f;
    sc->use_palette = true;

    cadv_scene_draw(sc, &g->shell, cadv_m4_identity(), cadv_material_make(g->tex_stone, white, 0, 0));
    cadv_scene_draw(sc, &g->floor, cadv_m4_identity(), cadv_material_make(g->tex_floor, white, 0, 0));
    cadv_scene_draw(sc, &g->balconies, cadv_m4_identity(), cadv_material_make(g->tex_wood, white, 0, 0));
    cadv_scene_draw(sc, &g->columns, cadv_m4_identity(), cadv_material_make(g->tex_stone, cadv_v4(0.55f, 0.55f, 0.6f, 1), 0, 0));
    cadv_scene_draw(sc, &g->arches, cadv_m4_identity(), cadv_material_make(g->tex_gold, white, 0.05f, 0));
    cadv_scene_draw(sc, &g->tree, cadv_m4_translate(cadv_hall_tree()), cadv_material_make(0, white, 0.0f, 0));
    if (h->progress->tree_crowned) {
        cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(cadv_v3(0, 9.7f, -16.0f)), cadv_m4_rotate_y(t)),
                        cadv_material_make(0, white, 1.2f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, cadv_v3(0, 10.0f, -16.0f), cadv_v3(2.5f, 2.2f, 1.2f), 16.0f, false);
    }
    /* portals */
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        cadv_portal_state st = h->state[day - 1];
        cadv_vec3 col = portal_color(day);
        cadv_portal_slot s = cadv_hall_portal(day);
        int reveal = h->reveal_timer[day - 1];
        cadv_mat4 plate = cadv_m4_mul(portal_xf(day, 0.55f), cadv_m4_translate(cadv_v3(0, CADV_ARCH_HEIGHT + 0.7f, 0)));
        cadv_scene_draw(sc, &g->plate, plate, cadv_material_make(g->tex_day[day - 1], white, 0.4f, CADV_MAT_NO_SHADOW));
        if (st == CADV_PS_SEALED || st == CADV_PS_STILL_THAWING || st == CADV_PS_STARS_MISSING || reveal > 0) {
            float crack = reveal > 0 ? 1.0f - (float)reveal / 90.0f : 0.0f;
            cadv_vec4 tint = st == CADV_PS_STILL_THAWING ? cadv_v4(0.8f, 0.9f, 1.0f, 1) : cadv_v4(0.7f, 0.85f, 1.0f, 1);
            cadv_mat4 xf = cadv_m4_mul(portal_xf(day, 0.2f), cadv_m4_scale(cadv_v3(1.0f + crack * 0.3f, 1.0f - crack, 1.0f)));
            cadv_scene_draw(sc, &g->ice, xf, cadv_material_make(g->tex_ice, tint, 0.15f + crack, 0));
            if (reveal > 0) {
                cadv_scene_light(sc, cadv_v3_add(s.base, cadv_v3_add(cadv_v3_scale(s.normal, 1.0f), cadv_v3(0, 1.7f, 0))),
                                 cadv_v3_scale(cadv_v3(1.5f, 1.8f, 2.2f), crack * 2.0f), 9.0f, false);
            }
        } else {
            float pulse = 0.8f + 0.2f * cadv_sinf(t * 2.0f + (float)day);
            cadv_vec4 tint = st == CADV_PS_MASTERED ? cadv_v4(1.0f, 0.85f, 0.4f, 1) : cadv_v4(col.x, col.y, col.z, 1);
            {
                /* swirl: rotate the membrane disc around its own axis */
                cadv_mat4 spin = cadv_m4_mul(portal_xf(day, 0.3f),
                                             cadv_m4_mul(cadv_m4_translate(cadv_v3(0, 1.7f, 0)),
                                                         cadv_m4_mul(cadv_m4_mul(cadv_m4_rotate_x(-0.5f * CADV_PI),
                                                                                 cadv_m4_rotate_y(t * (0.8f + 0.1f * (float)(day % 4)))),
                                                                     cadv_m4_mul(cadv_m4_rotate_x(0.5f * CADV_PI),
                                                                                 cadv_m4_translate(cadv_v3(0, -1.7f, 0))))));
                cadv_scene_draw(sc, &g->membrane, spin, cadv_material_make(g->tex_swirl, tint, 1.2f * pulse, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
            }
            if (cadv_v3_dist(s.base, cam->pos) < 18.0f || day == 24) {
                cadv_scene_light(sc, cadv_v3_add(s.base, cadv_v3_add(cadv_v3_scale(s.normal, 0.8f), cadv_v3(0, 1.7f, 0))),
                                 cadv_v3_scale(col, 1.4f * pulse), 6.0f, false);
            }
        }
    }
    /* Star Arch sockets */
    for (day = 1; day < CADV_PORTAL_COUNT; ++day) {
        cadv_vec3 p = cadv_hall_socket(day);
        bool set = h->progress->p[day - 1].star_socketed;
        if (set) {
            cadv_scene_draw(sc, &g->star, cadv_m4_mul(cadv_m4_translate(cadv_v3(p.x, p.y - 0.37f, p.z + 0.1f)), cadv_m4_rotate_y(0.3f * cadv_sinf(t + (float)day))),
                            cadv_material_make(0, white, 1.0f, CADV_MAT_NO_SHADOW));
        } else {
            cadv_mesh_box(&g->batch, cadv_v3(p.x - 0.08f, p.y, p.z), cadv_v3(p.x + 0.08f, p.y + 0.16f, p.z + 0.16f), 0.0f,
                          cadv_rgba_make(40, 32, 26, 255), false);
        }
    }
    if (cadv_progress_stars_socketed(h->progress) > 0) {
        float k = (float)cadv_progress_stars_socketed(h->progress) / 23.0f;
        cadv_scene_light(sc, cadv_v3(0, 4.5f, CADV_APSE_NORTH + 2.0f), cadv_v3_scale(cadv_v3(1.6f, 1.4f, 0.8f), k * 1.5f), 12.0f, true);
    }
    /* light beams through the skylight */
    nb = cadv_hub_beams(h, beams, 24);
    for (i = 0; i < nb; ++i) {
        cadv_vec3 f = cadv_hall_portal_front(beams[i], 1.5f);
        cadv_scene_draw(sc, &g->beam, cadv_m4_translate(f), cadv_material_make(0, cadv_v4(0.5f, 0.6f, 1.0f, 1), 0.45f, CADV_MAT_UNLIT | CADV_MAT_NO_SHADOW));
    }
    /* banners, candles, chandeliers */
    for (i = 0; i < 10; ++i) {
        float z = -6.5f - 5.0f * (float)(i % 5);
        float x = i < 5 ? -11.39f : 11.39f;
        cadv_mat4 xf = cadv_m4_mul(cadv_m4_translate(cadv_v3(x + (i < 5 ? 0.55f : -0.55f), 8.5f, z)), cadv_m4_rotate_y(i < 5 ? -0.5f * CADV_PI : 0.5f * CADV_PI));
        cadv_scene_draw(sc, &g->banner, xf, cadv_material_make(g->tex_cloth, white, 0, CADV_MAT_NO_SHADOW));
    }
    cadv_scene_light(sc, cadv_v3(0, 8.0f, -6.0f), cadv_v3(1.3f + 0.1f * cadv_sinf(t * 9.0f), 0.95f, 0.55f), 20.0f, true);
    cadv_scene_light(sc, cadv_v3(0, 8.0f, -26.0f), cadv_v3(1.3f, 0.95f + 0.05f * cadv_sinf(t * 7.0f), 0.55f), 20.0f, true);
    for (i = 0; i < 6; ++i) {
        float side = (i & 1) ? 1.0f : -1.0f;
        float z = 4.0f - 12.0f * (float)(i / 2);
        cadv_vec3 p = cadv_v3(side * 10.8f, 1.2f, z);
        cadv_scene_draw(sc, &g->candle, cadv_m4_translate(p), cadv_material_make(0, white, 0.6f, CADV_MAT_NO_SHADOW));
        cadv_scene_light(sc, cadv_v3_add(p, cadv_v3(0, 0.45f, 0)), cadv_v3(1.0f, 0.6f + 0.1f * cadv_sinf(t * 11.0f + (float)i), 0.25f), 5.0f, false);
    }
    /* wreath with candles above the entrance */
    cadv_scene_draw(sc, &g->wreath, cadv_m4_translate(cadv_v3(0, 7.0f, CADV_HALL_SOUTH - 1.4f)), cadv_material_make(0, white, 0, 0));
    for (i = 0; i < 4; ++i) {
        float a = CADV_TAU * (float)i / 4.0f + 0.4f;
        cadv_vec3 p = cadv_v3(1.1f * cadv_cosf(a), 7.15f, CADV_HALL_SOUTH - 1.4f + 1.1f * cadv_sinf(a));
        bool lit = i < cadv_progress_candles(h->progress);
        cadv_scene_draw(sc, &g->candle, cadv_m4_translate(p), cadv_material_make(0, lit ? white : cadv_v4(0.5f, 0.1f, 0.1f, 1), lit ? 0.8f : 0.0f, CADV_MAT_NO_SHADOW));
        if (lit) {
            cadv_scene_light(sc, cadv_v3_add(p, cadv_v3(0, 0.4f, 0)), cadv_v3(1.0f, 0.6f, 0.2f), 4.0f, false);
        }
    }
    /* Wall of Legends */
    for (i = 0; i < 6 && i < h->friends->count; ++i) {
        int found;
        int total;
        int k;
        cadv_vec3 p = cadv_hall_legend_plaque(i);
        cadv_mat4 xf = cadv_m4_mul(cadv_m4_translate(p), cadv_m4_rotate_y(CADV_PI));
        cadv_friends_tally(h->progress, i, &found, &total);
        cadv_scene_draw(sc, &g->plaque, xf, cadv_material_make(g->tex_legend[i], white, found >= total && total > 0 ? 0.6f : 0.15f, CADV_MAT_NO_SHADOW));
        for (k = 0; k < total; ++k) {
            cadv_vec3 n = cadv_v3(p.x - 0.6f + 0.09f * (float)k, p.y - 0.5f, p.z - 0.1f);
            cadv_mesh_box(&g->batch, n, cadv_v3_add(n, cadv_v3(0.06f, 0.06f, 0.06f)), 0.0f,
                          k < found ? cadv_rgba_make(255, 200, 80, 255) : cadv_rgba_make(50, 50, 50, 255), false);
        }
    }
    cadv_scene_light(sc, cadv_v3(7.5f, 3.5f, CADV_HALL_SOUTH - 1.5f), cadv_v3(0.8f, 0.6f, 0.4f), 6.0f, false);
    /* snow drifting down around the tree */
    for (i = 0; i < CADV_HUB_SNOW; ++i) {
        cadv_vec3 p = g->snow[i];
        float y = CADV_HALL_HEIGHT - (float)((int)((t * 0.6f + p.y) * 100.0f) % 1200) / 100.0f;
        cadv_vec3 q = cadv_v3(p.x + 0.3f * cadv_sinf(t + (float)i), y, p.z);
        cadv_mesh_box(&g->batch, cadv_v3_sub(q, cadv_v3(0.03f, 0.03f, 0.03f)), cadv_v3_add(q, cadv_v3(0.03f, 0.03f, 0.03f)), 0.0f,
                      cadv_rgba_make(235, 240, 255, 255), false);
    }
    if (g->batch.ni > 0) {
        cadv_gpu_mesh_upload(&g->batch_gpu, &g->batch, true);
        cadv_scene_draw(sc, &g->batch_gpu, cadv_m4_identity(), cadv_material_make(0, white, 0.5f, CADV_MAT_NO_SHADOW));
    }
    if (ui) {
        draw_hud(h, input, ui, dev_marker);
    }
}

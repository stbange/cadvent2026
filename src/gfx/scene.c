/* SPDX-License-Identifier: MIT */
#include "gfx/scene.h"

#include <string.h>

#include "gfx/font.h"

cadv_vec4 cadv_v4(float x, float y, float z, float w)
{
    cadv_vec4 v;
    v.x = x;
    v.y = y;
    v.z = z;
    v.w = w;
    return v;
}

void cadv_scene_reset(cadv_scene *s)
{
    s->ndraws = 0;
    s->nsprites = 0;
    s->nlights = 0;
    s->ambient = cadv_v3(0.15f, 0.15f, 0.2f);
    s->sun_dir = cadv_v3_norm(cadv_v3(-0.3f, -1.0f, -0.2f));
    s->sun_color = cadv_v3(0, 0, 0);
    s->sun_shadow = false;
    s->shadow_center = cadv_v3(0, 0, 0);
    s->shadow_extent = 30.0f;
    s->fog_color = cadv_v3(0, 0, 0);
    s->fog_near = 0.0f;
    s->fog_far = 0.0f;
    s->clear_color = cadv_v3(0, 0, 0);
    s->light_bands = 0;
    s->dither = 0.0f;
    s->use_palette = false;
}

cadv_material cadv_material_make(unsigned tex, cadv_vec4 tint, float emissive, uint32_t flags)
{
    cadv_material m;
    m.tex = tex;
    m.tint = tint;
    m.emissive = emissive;
    m.flags = flags;
    return m;
}

bool cadv_scene_draw(cadv_scene *s, const cadv_gpu_mesh *mesh, cadv_mat4 model, cadv_material mat)
{
    cadv_draw *d;
    if (s->ndraws >= CADV_MAX_DRAWS) {
        return false;
    }
    d = &s->draws[s->ndraws++];
    d->mesh = mesh;
    d->model = model;
    d->mat = mat;
    return true;
}

bool cadv_scene_sprite(cadv_scene *s, const cadv_sprite *sp)
{
    if (s->nsprites >= CADV_MAX_SPRITES) {
        return false;
    }
    s->sprites[s->nsprites++] = *sp;
    return true;
}

bool cadv_scene_light(cadv_scene *s, cadv_vec3 pos, cadv_vec3 color, float radius, bool shadow)
{
    cadv_light *l;
    if (s->nlights >= CADV_MAX_LIGHTS) {
        return false;
    }
    l = &s->lights[s->nlights++];
    l->pos = pos;
    l->color = color;
    l->radius = radius;
    l->casts_shadow = shadow;
    return true;
}

void cadv_ui_reset(cadv_ui *ui, int width, int height)
{
    ui->count = 0;
    ui->width = width;
    ui->height = height;
}

static void push(cadv_ui *ui, float x, float y, float w, float h, int ch, cadv_vec4 color)
{
    cadv_ui_quad *q;
    int cx;
    int cy;
    if (ui->count >= CADV_UI_MAX_QUADS) {
        return;
    }
    q = &ui->q[ui->count++];
    cadv_font_cell(ch, &cx, &cy);
    q->x = x;
    q->y = y;
    q->w = w;
    q->h = h;
    q->u0 = (float)cx / (float)CADV_FONT_ATLAS_W;
    q->v0 = (float)cy / (float)CADV_FONT_ATLAS_H;
    q->u1 = (float)(cx + (ch == 127 ? 1 : 8)) / (float)CADV_FONT_ATLAS_W;
    q->v1 = (float)(cy + (ch == 127 ? 1 : 8)) / (float)CADV_FONT_ATLAS_H;
    q->color = color;
}

void cadv_ui_rect(cadv_ui *ui, float x, float y, float w, float h, cadv_vec4 color)
{
    push(ui, x, y, w, h, 127, color);
}

int cadv_ui_text(cadv_ui *ui, float x, float y, int scale, cadv_vec4 color, const char *text)
{
    float cx = x;
    float maxx = x;
    if (scale < 1) {
        scale = 1;
    }
    for (; *text; ++text) {
        unsigned char ch = (unsigned char)*text;
        if (ch == '\n') {
            cx = x;
            y += (float)(CADV_FONT_LINE * scale);
            continue;
        }
        if (ch != ' ') {
            push(ui, cx, y, (float)(8 * scale), (float)(8 * scale), ch, color);
        }
        cx += (float)(CADV_FONT_ADVANCE * scale);
        if (cx > maxx) {
            maxx = cx;
        }
    }
    return (int)(maxx - x);
}

void cadv_ui_text_centered(cadv_ui *ui, float cx, float y, int scale, cadv_vec4 color, const char *text)
{
    int w;
    int h;
    cadv_font_measure(text, &w, &h);
    cadv_ui_text(ui, cx - (float)(w * (scale < 1 ? 1 : scale)) * 0.5f, y, scale, color, text);
}

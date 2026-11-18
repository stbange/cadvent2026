/* SPDX-License-Identifier: MIT */
#include "gfx/renderer.h"

#include <stdlib.h>
#include <string.h>

#include "core/log.h"
#include "gfx/font.h"
#include "gfx/gl_api.h"

/* ---- shaders ------------------------------------------------------------- */

static const char k_lit_vs[] =
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 1) in vec3 a_nrm;\n"
    "layout(location = 2) in vec2 a_uv;\n"
    "layout(location = 3) in vec4 a_col;\n"
    "uniform mat4 u_model;\n"
    "uniform mat4 u_viewproj;\n"
    "uniform mat4 u_sun_mat;\n"
    "out vec3 v_wpos;\n"
    "out vec3 v_nrm;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_col;\n"
    "out vec4 v_sunpos;\n"
    "void main() {\n"
    "  vec4 w = u_model * vec4(a_pos, 1.0);\n"
    "  v_wpos = w.xyz;\n"
    "  v_nrm = mat3(u_model) * a_nrm;\n"
    "  v_uv = a_uv;\n"
    "  v_col = a_col;\n"
    "  v_sunpos = u_sun_mat * w;\n"
    "  gl_Position = u_viewproj * w;\n"
    "}\n";

static const char k_lit_fs[] =
    "uniform sampler2D u_tex;\n"
    "uniform sampler2D u_ntex;\n"
    "uniform sampler2DShadow u_sun_shadow;\n"
    "uniform samplerCubeShadow u_ps0;\n"
    "uniform samplerCubeShadow u_ps1;\n"
    "uniform samplerCubeShadow u_ps2;\n"
    "uniform samplerCubeShadow u_ps3;\n"
    "uniform vec4 u_tint;\n"
    "uniform float u_emissive;\n"
    "uniform int u_flags;\n" /* 1 unlit, 2 alpha test, 4 flat, 16 no fog, 256 sprite, 512 sprite normals */
    "uniform vec3 u_ambient;\n"
    "uniform vec3 u_sun_dir;\n"
    "uniform vec3 u_sun_col;\n"
    "uniform int u_sun_on;\n"
    "uniform int u_nlights;\n"
    "uniform vec4 u_lpos[32];\n"
    "uniform vec4 u_lcol[32];\n"
    "uniform vec2 u_pnf;\n"
    "uniform vec3 u_eye;\n"
    "uniform vec3 u_fog_col;\n"
    "uniform vec2 u_fog;\n"
    "uniform float u_bands;\n"
    "in vec3 v_wpos;\n"
    "in vec3 v_nrm;\n"
    "in vec2 v_uv;\n"
    "in vec4 v_col;\n"
    "in vec4 v_sunpos;\n"
    "out vec4 o_color;\n"
    "float pshadow(int slot, vec3 d, float f) {\n"
    "  vec3 a = abs(d);\n"
    "  float z = max(a.x, max(a.y, a.z));\n"
    "  float n = u_pnf.x;\n"
    "  float depth = ((f + n) / (f - n) - (2.0 * f * n) / ((f - n) * z)) * 0.5 + 0.5;\n"
    "  vec4 c = vec4(d, depth - 0.0015);\n"
    "  if (slot == 0) return texture(u_ps0, c);\n"
    "  if (slot == 1) return texture(u_ps1, c);\n"
    "  if (slot == 2) return texture(u_ps2, c);\n"
    "  return texture(u_ps3, c);\n"
    "}\n"
    "void main() {\n"
    "  vec4 base = texture(u_tex, v_uv) * v_col * u_tint;\n"
    "  if ((u_flags & 2) != 0 && base.a < 0.5) discard;\n"
    "  vec3 N = normalize(v_nrm);\n"
    "  if ((u_flags & 4) != 0) N = normalize(cross(dFdx(v_wpos), dFdy(v_wpos)));\n"
    "  if ((u_flags & 512) != 0) {\n"
    "    vec3 sn = texture(u_ntex, v_uv).xyz * 2.0 - 1.0;\n"
    "    vec3 fwd = N;\n"
    "    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), fwd));\n"
    "    N = normalize(sn.x * right + sn.y * vec3(0.0, 1.0, 0.0) + sn.z * fwd);\n"
    "  }\n"
    "  vec3 light = u_ambient;\n"
    "  if (u_sun_on != 0) {\n"
    "    float ndl = max(dot(N, -u_sun_dir), 0.0);\n"
    "    float sh = 1.0;\n"
    "    vec3 sp = v_sunpos.xyz / v_sunpos.w * 0.5 + 0.5;\n"
    "    if (u_sun_on == 2 && sp.x > 0.0 && sp.x < 1.0 && sp.y > 0.0 && sp.y < 1.0 && sp.z < 1.0)\n"
    "      sh = texture(u_sun_shadow, vec3(sp.xy, sp.z - 0.002));\n"
    "    light += u_sun_col * ndl * sh;\n"
    "  }\n"
    "  for (int i = 0; i < 32; ++i) {\n"
    "    if (i >= u_nlights) break;\n"
    "    vec3 L = u_lpos[i].xyz - v_wpos;\n"
    "    float d = length(L);\n"
    "    float att = clamp(1.0 - d / u_lpos[i].w, 0.0, 1.0);\n"
    "    if (att <= 0.0) continue;\n"
    "    att *= att;\n"
    "    float ndl = (u_flags & 256) != 0 && (u_flags & 512) == 0 ? 0.8 : max(dot(N, L / d), 0.0);\n"
    "    float sh = 1.0;\n"
    "    int slot = int(u_lcol[i].w);\n"
    "    if (slot >= 0) sh = pshadow(slot, -L, u_lpos[i].w);\n"
    "    light += u_lcol[i].rgb * att * ndl * sh;\n"
    "  }\n"
    "  if (u_bands > 0.0) light = floor(light * u_bands + 0.5) / u_bands;\n"
    "  vec3 col = (u_flags & 1) != 0 ? base.rgb : base.rgb * light;\n"
    "  col += base.rgb * u_emissive;\n"
    "  if ((u_flags & 16) == 0 && u_fog.y > u_fog.x) {\n"
    "    float k = clamp((length(v_wpos - u_eye) - u_fog.x) / (u_fog.y - u_fog.x), 0.0, 1.0);\n"
    "    col = mix(col, u_fog_col, k);\n"
    "  }\n"
    "  o_color = vec4(col, base.a);\n"
    "}\n";

static const char k_depth_vs[] =
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 2) in vec2 a_uv;\n"
    "uniform mat4 u_model;\n"
    "uniform mat4 u_viewproj;\n"
    "out vec2 v_uv;\n"
    "void main() { v_uv = a_uv; gl_Position = u_viewproj * u_model * vec4(a_pos, 1.0); }\n";

static const char k_depth_fs[] =
    "uniform sampler2D u_tex;\n"
    "uniform int u_alpha;\n"
    "in vec2 v_uv;\n"
    "out vec4 o_color;\n"
    "void main() {\n"
    "  if (u_alpha != 0 && texture(u_tex, v_uv).a < 0.5) discard;\n"
    "  o_color = vec4(1.0);\n"
    "}\n";

static const char k_post_vs[] =
    "out vec2 v_uv;\n"
    "void main() {\n"
    "  vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n"
    "  v_uv = p;\n"
    "  gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n"
    "}\n";

static const char k_post_fs[] =
    "uniform sampler2D u_scene;\n"
    "uniform sampler3D u_lut;\n"
    "uniform float u_dither;\n"
    "uniform int u_palette;\n"
    "in vec2 v_uv;\n"
    "out vec4 o_color;\n"
    "const float bayer[16] = float[16](0.0, 8.0, 2.0, 10.0, 12.0, 4.0, 14.0, 6.0,\n"
    "                                  3.0, 11.0, 1.0, 9.0, 15.0, 7.0, 13.0, 5.0);\n"
    "void main() {\n"
    "  vec3 c = texture(u_scene, v_uv).rgb;\n"
    "  ivec2 p = ivec2(gl_FragCoord.xy) & 3;\n"
    "  c += (bayer[p.y * 4 + p.x] / 16.0 - 0.47) * u_dither / 16.0;\n"
    "  c = clamp(c, 0.0, 1.0);\n"
    "  if (u_palette != 0) c = texture(u_lut, c * (31.0 / 32.0) + 0.5 / 32.0).rgb;\n"
    "  o_color = vec4(c, 1.0);\n"
    "}\n";

static const char k_ui_vs[] =
    "layout(location = 0) in vec3 a_pos;\n"
    "layout(location = 2) in vec2 a_uv;\n"
    "layout(location = 3) in vec4 a_col;\n"
    "uniform vec2 u_size;\n"
    "out vec2 v_uv;\n"
    "out vec4 v_col;\n"
    "void main() {\n"
    "  v_uv = a_uv; v_col = a_col;\n"
    "  vec2 ndc = vec2(a_pos.x / u_size.x * 2.0 - 1.0, 1.0 - a_pos.y / u_size.y * 2.0);\n"
    "  gl_Position = vec4(ndc, 0.0, 1.0);\n"
    "}\n";

static const char k_ui_fs[] =
    "uniform sampler2D u_tex;\n"
    "in vec2 v_uv;\n"
    "in vec4 v_col;\n"
    "out vec4 o_color;\n"
    "void main() {\n"
    "  vec4 t = texture(u_tex, v_uv) * v_col;\n"
    "  if (t.a < 0.01) discard;\n"
    "  o_color = t;\n"
    "}\n";

/* ---- helpers --------------------------------------------------------------- */

#define U(prog, name) glGetUniformLocation((prog), (name))

enum { UNIT_TEX = 0, UNIT_NTEX = 1, UNIT_SUN = 2, UNIT_PS0 = 3, UNIT_LUT = 7 };

static unsigned make_white(void)
{
    cadv_texdata t;
    unsigned tex;
    cadv_texdata_alloc(&t, 1, 1);
    cadv_texdata_set(&t, 0, 0, 255, 255, 255, 255);
    tex = cadv_gl_texture(&t, true);
    cadv_texdata_free(&t);
    return tex;
}

static unsigned make_font(void)
{
    cadv_texdata t;
    unsigned tex;
    cadv_texdata_alloc(&t, CADV_FONT_ATLAS_W, CADV_FONT_ATLAS_H);
    cadv_font_build_atlas(t.rgba);
    tex = cadv_gl_texture(&t, false);
    cadv_texdata_free(&t);
    return tex;
}

static void set_lut(cadv_renderer *r)
{
    uint8_t *lut = (uint8_t *)malloc((size_t)CADV_LUT_DIM * CADV_LUT_DIM * CADV_LUT_DIM * 3u);
    if (!lut) {    /* GCOVR_EXCL_BR_LINE: allocation failure */
        return;    /* GCOVR_EXCL_LINE */
    }
    cadv_palette_build_lut(&r->palette, lut);
    glDeleteTextures(1, &r->tex_lut);
    r->tex_lut = cadv_gl_lut3d(lut, CADV_LUT_DIM);
    free(lut);
}

static void bind_samplers(unsigned prog)
{
    glUseProgram(prog);
    glUniform1i(U(prog, "u_tex"), UNIT_TEX);
    glUniform1i(U(prog, "u_ntex"), UNIT_NTEX);
    glUniform1i(U(prog, "u_sun_shadow"), UNIT_SUN);
    glUniform1i(U(prog, "u_ps0"), UNIT_PS0);
    glUniform1i(U(prog, "u_ps1"), UNIT_PS0 + 1);
    glUniform1i(U(prog, "u_ps2"), UNIT_PS0 + 2);
    glUniform1i(U(prog, "u_ps3"), UNIT_PS0 + 3);
}

bool cadv_renderer_init(cadv_renderer *r, cadv_gl_profile profile, int internal_height)
{
    int i;
    memset(r, 0, sizeof *r);
    r->profile = profile;
    r->internal_height = internal_height;
    r->prog_lit = cadv_gl_program(profile, k_lit_vs, k_lit_fs);
    r->prog_depth = cadv_gl_program(profile, k_depth_vs, k_depth_fs);
    r->prog_post = cadv_gl_program(profile, k_post_vs, k_post_fs);
    r->prog_ui = cadv_gl_program(profile, k_ui_vs, k_ui_fs);
    if (!r->prog_lit || !r->prog_depth || !r->prog_post || !r->prog_ui) { /* GCOVR_EXCL_BR_LINE */
        return false;                                                      /* GCOVR_EXCL_LINE */
    }
    bind_samplers(r->prog_lit);
    glUseProgram(r->prog_depth);
    glUniform1i(U(r->prog_depth, "u_tex"), UNIT_TEX);
    glUseProgram(r->prog_post);
    glUniform1i(U(r->prog_post, "u_scene"), UNIT_TEX);
    glUniform1i(U(r->prog_post, "u_lut"), UNIT_LUT);
    glUseProgram(r->prog_ui);
    glUniform1i(U(r->prog_ui, "u_tex"), UNIT_TEX);
    glUseProgram(0);
    r->tex_white = make_white();
    r->tex_font = make_font();
    cadv_shadow2d_init(&r->sun, 1024);
    for (i = 0; i < CADV_MAX_SHADOW_LIGHTS; ++i) {
        cadv_shadowcube_init(&r->cubes[i], 256);
    }
    cadv_palette_hall(&r->palette);
    set_lut(r);
    cadv_mesh_init(&r->scratch);
    glGenVertexArrays(1, &r->empty_vao);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    r->ready = true;
    CADV_INFO("renderer ready (%s): %s", cadv_gl_profile_name(profile),
              (const char *)glGetString(GL_RENDERER));
    return true;
}

void cadv_renderer_set_palette(cadv_renderer *r, const cadv_palette *p)
{
    r->palette = *p;
    set_lut(r);
}

void cadv_renderer_target_size(const cadv_renderer *r, cadv_rect viewport, int *w, int *h)
{
    cadv_view_internal_size(viewport.w, viewport.h, r->internal_height, w, h);
}

/* Builds camera-facing (Y-axis aligned) quads for all sprites of the scene. */
static void build_sprites(cadv_renderer *r, const cadv_scene *s, cadv_vec3 eye)
{
    int i;
    cadv_mesh_clear(&r->scratch);
    for (i = 0; i < s->nsprites; ++i) {
        const cadv_sprite *sp = &s->sprites[i];
        cadv_vec3 to_eye = cadv_v3(eye.x - sp->pos.x, 0.0f, eye.z - sp->pos.z);
        cadv_vec3 fwd = cadv_v3_len(to_eye) > 1e-4f ? cadv_v3_norm(to_eye) : cadv_v3(0, 0, 1);
        cadv_vec3 right = cadv_v3_scale(cadv_v3_cross(cadv_v3(0, 1, 0), fwd), 0.5f * sp->w);
        cadv_vec3 up = cadv_v3(0, sp->h, 0);
        cadv_rgba white = cadv_rgba_make(255, 255, 255, 255);
        uint32_t b0 = cadv_mesh_vertex(&r->scratch, cadv_v3_sub(sp->pos, right), fwd, sp->u0, sp->v1, white);
        uint32_t b1 = cadv_mesh_vertex(&r->scratch, cadv_v3_add(sp->pos, right), fwd, sp->u1, sp->v1, white);
        uint32_t t1 = cadv_mesh_vertex(&r->scratch, cadv_v3_add(cadv_v3_add(sp->pos, right), up), fwd, sp->u1, sp->v0, white);
        uint32_t t0 = cadv_mesh_vertex(&r->scratch, cadv_v3_add(cadv_v3_sub(sp->pos, right), up), fwd, sp->u0, sp->v0, white);
        cadv_mesh_tri(&r->scratch, b0, b1, t1);
        cadv_mesh_tri(&r->scratch, b0, t1, t0);
    }
    if (s->nsprites > 0) {
        cadv_gpu_mesh_upload(&r->sprite_mesh, &r->scratch, true);
    }
}

static void draw_sprite_range(cadv_renderer *r, int first)
{
    glBindVertexArray(r->sprite_mesh.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)(sizeof(uint32_t) * 6u * (size_t)first));
    glBindVertexArray(0);
    r->stats.draw_calls++;
    r->stats.triangles += 2;
}

static void depth_pass(cadv_renderer *r, const cadv_scene *s, cadv_mat4 viewproj, int size)
{
    int i;
    unsigned p = r->prog_depth;
    glViewport(0, 0, size, size);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(p);
    glUniformMatrix4fv(U(p, "u_viewproj"), 1, GL_FALSE, viewproj.m);
    glActiveTexture(GL_TEXTURE0 + UNIT_TEX);
    glUniform1i(U(p, "u_alpha"), 0);
    for (i = 0; i < s->ndraws; ++i) {
        const cadv_draw *d = &s->draws[i];
        if (d->mat.flags & (CADV_MAT_NO_SHADOW | CADV_MAT_UNLIT)) {
            continue;
        }
        glUniformMatrix4fv(U(p, "u_model"), 1, GL_FALSE, d->model.m);
        cadv_gpu_mesh_draw(d->mesh);
        r->stats.draw_calls++;
    }
    glUniform1i(U(p, "u_alpha"), 1);
    glUniformMatrix4fv(U(p, "u_model"), 1, GL_FALSE, cadv_m4_identity().m);
    for (i = 0; i < s->nsprites; ++i) {
        if (!s->sprites[i].casts_shadow) {
            continue;
        }
        glBindTexture(GL_TEXTURE_2D, s->sprites[i].tex ? s->sprites[i].tex : r->tex_white);
        draw_sprite_range(r, i);
    }
}

static cadv_mat4 sun_matrix(const cadv_scene *s)
{
    cadv_vec3 dir = cadv_v3_norm(s->sun_dir);
    cadv_vec3 up = cadv_absf(dir.y) > 0.99f ? cadv_v3(0, 0, 1) : cadv_v3(0, 1, 0);
    cadv_vec3 eye = cadv_v3_sub(s->shadow_center, cadv_v3_scale(dir, s->shadow_extent * 2.0f));
    float e = s->shadow_extent;
    return cadv_m4_mul(cadv_m4_ortho(-e, e, -e, e, 0.1f, e * 4.0f),
                       cadv_m4_look_at(eye, s->shadow_center, up));
}

#define POINT_NEAR 0.05f

static void shadow_passes(cadv_renderer *r, const cadv_scene *s, cadv_vec3 eye, int *slots,
                          cadv_mat4 *sun_mat)
{
    static const float dirs[6][6] = {
        {1, 0, 0, 0, -1, 0}, {-1, 0, 0, 0, -1, 0}, {0, 1, 0, 0, 0, 1},
        {0, -1, 0, 0, 0, -1}, {0, 0, 1, 0, -1, 0}, {0, 0, -1, 0, -1, 0},
    };
    cadv_light_ref refs[CADV_MAX_LIGHTS];
    int chosen[CADV_MAX_SHADOW_LIGHTS];
    int n;
    int i;
    int f;
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.5f, 2.0f);
    *sun_mat = cadv_m4_identity();
    if (s->sun_shadow && cadv_v3_len(s->sun_color) > 0.0f) {
        *sun_mat = sun_matrix(s);
        glBindFramebuffer(GL_FRAMEBUFFER, r->sun.fbo);
        depth_pass(r, s, *sun_mat, r->sun.size);
    }
    for (i = 0; i < s->nlights; ++i) {
        float lum = s->lights[i].color.x + s->lights[i].color.y + s->lights[i].color.z;
        refs[i].pos = s->lights[i].pos;
        refs[i].radius = s->lights[i].radius;
        refs[i].intensity = lum;
        refs[i].casts_shadow = s->lights[i].casts_shadow;
        slots[i] = -1;
    }
    n = cadv_select_shadow_lights(refs, s->nlights, eye, chosen, CADV_MAX_SHADOW_LIGHTS);
    r->stats.shadow_lights = n;
    for (i = 0; i < n; ++i) {
        const cadv_light *l = &s->lights[chosen[i]];
        cadv_mat4 proj = cadv_m4_perspective(0.5f * CADV_PI, 1.0f, POINT_NEAR, l->radius);
        slots[chosen[i]] = i;
        for (f = 0; f < 6; ++f) {
            cadv_vec3 target = cadv_v3_add(l->pos, cadv_v3(dirs[f][0], dirs[f][1], dirs[f][2]));
            cadv_mat4 view = cadv_m4_look_at(l->pos, target, cadv_v3(dirs[f][3], dirs[f][4], dirs[f][5]));
            cadv_shadowcube_bind_face(&r->cubes[i], f);
            depth_pass(r, s, cadv_m4_mul(proj, view), r->cubes[i].size);
        }
    }
    glDisable(GL_POLYGON_OFFSET_FILL);
}

static void lit_pass(cadv_renderer *r, const cadv_scene *s, const cadv_camera *cam, int w, int h,
                     const int *slots, cadv_mat4 sun_mat)
{
    unsigned p = r->prog_lit;
    float lpos[CADV_MAX_LIGHTS * 4];
    float lcol[CADV_MAX_LIGHTS * 4];
    cadv_mat4 vp = cadv_m4_mul(cadv_camera_proj(cam, (float)w / (float)h), cadv_camera_view(cam));
    int i;
    glViewport(0, 0, w, h);
    glClearColor(s->clear_color.x, s->clear_color.y, s->clear_color.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glUseProgram(p);
    glUniformMatrix4fv(U(p, "u_viewproj"), 1, GL_FALSE, vp.m);
    glUniformMatrix4fv(U(p, "u_sun_mat"), 1, GL_FALSE, sun_mat.m);
    glUniform3f(U(p, "u_ambient"), s->ambient.x, s->ambient.y, s->ambient.z);
    glUniform3f(U(p, "u_sun_dir"), s->sun_dir.x, s->sun_dir.y, s->sun_dir.z);
    glUniform3f(U(p, "u_sun_col"), s->sun_color.x, s->sun_color.y, s->sun_color.z);
    glUniform1i(U(p, "u_sun_on"), cadv_v3_len(s->sun_color) > 0.0f ? (s->sun_shadow ? 2 : 1) : 0);
    for (i = 0; i < s->nlights; ++i) {
        lpos[i * 4 + 0] = s->lights[i].pos.x;
        lpos[i * 4 + 1] = s->lights[i].pos.y;
        lpos[i * 4 + 2] = s->lights[i].pos.z;
        lpos[i * 4 + 3] = s->lights[i].radius;
        lcol[i * 4 + 0] = s->lights[i].color.x;
        lcol[i * 4 + 1] = s->lights[i].color.y;
        lcol[i * 4 + 2] = s->lights[i].color.z;
        lcol[i * 4 + 3] = (float)slots[i];
    }
    glUniform1i(U(p, "u_nlights"), s->nlights);
    if (s->nlights > 0) {
        glUniform4fv(U(p, "u_lpos"), s->nlights, lpos);
        glUniform4fv(U(p, "u_lcol"), s->nlights, lcol);
    }
    glUniform2f(U(p, "u_pnf"), POINT_NEAR, 1.0f);
    glUniform3f(U(p, "u_eye"), cam->pos.x, cam->pos.y, cam->pos.z);
    glUniform3f(U(p, "u_fog_col"), s->fog_color.x, s->fog_color.y, s->fog_color.z);
    glUniform2f(U(p, "u_fog"), s->fog_near, s->fog_far);
    glUniform1f(U(p, "u_bands"), (float)s->light_bands);
    glActiveTexture(GL_TEXTURE0 + UNIT_SUN);
    glBindTexture(GL_TEXTURE_2D, r->sun.tex);
    for (i = 0; i < CADV_MAX_SHADOW_LIGHTS; ++i) {
        glActiveTexture(GL_TEXTURE0 + UNIT_PS0 + (GLenum)i);
        glBindTexture(GL_TEXTURE_CUBE_MAP, r->cubes[i].tex);
    }
    glActiveTexture(GL_TEXTURE0 + UNIT_NTEX);
    glBindTexture(GL_TEXTURE_2D, r->tex_white);
    glActiveTexture(GL_TEXTURE0 + UNIT_TEX);
    for (i = 0; i < s->ndraws; ++i) {
        const cadv_draw *d = &s->draws[i];
        glUniformMatrix4fv(U(p, "u_model"), 1, GL_FALSE, d->model.m);
        glUniform4f(U(p, "u_tint"), d->mat.tint.x, d->mat.tint.y, d->mat.tint.z, d->mat.tint.w);
        glUniform1f(U(p, "u_emissive"), d->mat.emissive);
        glUniform1i(U(p, "u_flags"), (int)(d->mat.flags & 0xffu));
        glBindTexture(GL_TEXTURE_2D, d->mat.tex ? d->mat.tex : r->tex_white);
        cadv_gpu_mesh_draw(d->mesh);
        r->stats.draw_calls++;
        r->stats.triangles += d->mesh->index_count / 3;
    }
    /* sprites: alpha tested, never culled */
    glDisable(GL_CULL_FACE);
    glUniformMatrix4fv(U(p, "u_model"), 1, GL_FALSE, cadv_m4_identity().m);
    for (i = 0; i < s->nsprites; ++i) {
        const cadv_sprite *sp = &s->sprites[i];
        int flags = 2 | 256 | (sp->normal_tex ? 512 : 0);
        glUniform4f(U(p, "u_tint"), sp->tint.x, sp->tint.y, sp->tint.z, sp->tint.w);
        glUniform1f(U(p, "u_emissive"), sp->emissive);
        glUniform1i(U(p, "u_flags"), flags);
        glActiveTexture(GL_TEXTURE0 + UNIT_NTEX);
        glBindTexture(GL_TEXTURE_2D, sp->normal_tex ? sp->normal_tex : r->tex_white);
        glActiveTexture(GL_TEXTURE0 + UNIT_TEX);
        glBindTexture(GL_TEXTURE_2D, sp->tex ? sp->tex : r->tex_white);
        draw_sprite_range(r, i);
    }
}

static void post_pass(cadv_renderer *r, const cadv_scene *s, int view, int w, int h)
{
    unsigned p = r->prog_post;
    glBindFramebuffer(GL_FRAMEBUFFER, r->post_rt[view].fbo);
    glViewport(0, 0, w, h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glUseProgram(p);
    glUniform1f(U(p, "u_dither"), s->dither);
    glUniform1i(U(p, "u_palette"), s->use_palette ? 1 : 0);
    glActiveTexture(GL_TEXTURE0 + UNIT_LUT);
    glBindTexture(GL_TEXTURE_3D, r->tex_lut);
    glActiveTexture(GL_TEXTURE0 + UNIT_TEX);
    glBindTexture(GL_TEXTURE_2D, r->scene_rt[view].color);
    glBindVertexArray(r->empty_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    r->stats.draw_calls++;
}

static void ui_pass(cadv_renderer *r, const cadv_ui *ui, int w, int h)
{
    unsigned p = r->prog_ui;
    int i;
    cadv_mesh_clear(&r->scratch);
    for (i = 0; i < ui->count; ++i) {
        const cadv_ui_quad *q = &ui->q[i];
        cadv_rgba c = cadv_rgba_make((uint8_t)(cadv_clampf(q->color.x, 0, 1) * 255.0f),
                                     (uint8_t)(cadv_clampf(q->color.y, 0, 1) * 255.0f),
                                     (uint8_t)(cadv_clampf(q->color.z, 0, 1) * 255.0f),
                                     (uint8_t)(cadv_clampf(q->color.w, 0, 1) * 255.0f));
        cadv_vec3 n = cadv_v3(0, 0, 1);
        uint32_t a = cadv_mesh_vertex(&r->scratch, cadv_v3(q->x, q->y, 0), n, q->u0, q->v0, c);
        uint32_t b = cadv_mesh_vertex(&r->scratch, cadv_v3(q->x + q->w, q->y, 0), n, q->u1, q->v0, c);
        uint32_t cc = cadv_mesh_vertex(&r->scratch, cadv_v3(q->x + q->w, q->y + q->h, 0), n, q->u1, q->v1, c);
        uint32_t d = cadv_mesh_vertex(&r->scratch, cadv_v3(q->x, q->y + q->h, 0), n, q->u0, q->v1, c);
        cadv_mesh_tri(&r->scratch, a, b, cc);
        cadv_mesh_tri(&r->scratch, a, cc, d);
    }
    if (ui->count == 0) {
        return;
    }
    cadv_gpu_mesh_upload(&r->ui_mesh, &r->scratch, true);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(p);
    glUniform2f(U(p, "u_size"), (float)w, (float)h);
    glActiveTexture(GL_TEXTURE0 + UNIT_TEX);
    glBindTexture(GL_TEXTURE_2D, r->tex_font);
    cadv_gpu_mesh_draw(&r->ui_mesh);
    glDisable(GL_BLEND);
    r->stats.draw_calls++;
}

void cadv_renderer_render(cadv_renderer *r, int window_w, int window_h,
                          const cadv_render_view *views, int nviews)
{
    int v;
    int slots[CADV_MAX_LIGHTS];
    memset(&r->stats, 0, sizeof r->stats);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, window_w, window_h);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (nviews > CADV_MAX_VIEWS) {
        nviews = CADV_MAX_VIEWS;
    }
    for (v = 0; v < nviews; ++v) {
        const cadv_render_view *rv = &views[v];
        int w;
        int h;
        cadv_mat4 sun_mat;
        cadv_renderer_target_size(r, rv->viewport, &w, &h);
        cadv_rt_ensure(&r->scene_rt[v], w, h);
        cadv_rt_ensure(&r->post_rt[v], w, h);
        build_sprites(r, rv->scene, rv->camera->pos);
        shadow_passes(r, rv->scene, rv->camera->pos, slots, &sun_mat);
        glBindFramebuffer(GL_FRAMEBUFFER, r->scene_rt[v].fbo);
        lit_pass(r, rv->scene, rv->camera, w, h, slots, sun_mat);
        post_pass(r, rv->scene, v, w, h);
        if (rv->ui) {
            ui_pass(r, rv->ui, w, h);
        }
        glBindFramebuffer(GL_READ_FRAMEBUFFER, r->post_rt[v].fbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, w, h, rv->viewport.x, rv->viewport.y, rv->viewport.x + rv->viewport.w,
                          rv->viewport.y + rv->viewport.h, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

bool cadv_renderer_capture(cadv_renderer *r, int width, int height, cadv_image *img)
{
    unsigned char *rgba;
    bool ok;
    (void)r;
    rgba = (unsigned char *)malloc((size_t)width * (size_t)height * 4u);
    if (!rgba) {     /* GCOVR_EXCL_BR_LINE: allocation failure */
        return false; /* GCOVR_EXCL_LINE */
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    ok = cadv_image_from_rgba(img, width, height, rgba);
    free(rgba);
    if (ok) {        /* GCOVR_EXCL_BR_LINE: sizes come from the window */
        cadv_image_flip_rows(img);
    }
    return ok;
}

void cadv_renderer_shutdown(cadv_renderer *r)
{
    int i;
    if (!r->ready) {
        return;
    }
    glDeleteProgram(r->prog_lit);
    glDeleteProgram(r->prog_depth);
    glDeleteProgram(r->prog_post);
    glDeleteProgram(r->prog_ui);
    glDeleteTextures(1, &r->tex_white);
    glDeleteTextures(1, &r->tex_font);
    glDeleteTextures(1, &r->tex_lut);
    cadv_shadow2d_free(&r->sun);
    for (i = 0; i < CADV_MAX_SHADOW_LIGHTS; ++i) {
        cadv_shadowcube_free(&r->cubes[i]);
    }
    for (i = 0; i < CADV_MAX_VIEWS; ++i) {
        cadv_rt_free(&r->scene_rt[i]);
        cadv_rt_free(&r->post_rt[i]);
    }
    cadv_gpu_mesh_free(&r->sprite_mesh);
    cadv_gpu_mesh_free(&r->ui_mesh);
    cadv_mesh_free(&r->scratch);
    glDeleteVertexArrays(1, &r->empty_vao);
    memset(r, 0, sizeof *r);
}

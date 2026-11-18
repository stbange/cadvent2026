/* SPDX-License-Identifier: MIT */
#include "gfx/gl_util.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "core/log.h"
#include "gfx/gl_api.h"

static GLuint compile(cadv_gl_profile profile, cadv_shader_stage stage, const char *body)
{
    size_t need = cadv_shader_build(NULL, 0, profile, stage, body);
    char *src = (char *)malloc(need + 1);
    const char *p = src;
    GLint ok = 0;
    GLuint sh;
    if (!src) {      /* GCOVR_EXCL_BR_LINE: allocation failure */
        return 0;    /* GCOVR_EXCL_LINE */
    }
    cadv_shader_build(src, need + 1, profile, stage, body);
    sh = glCreateShader(stage == CADV_SHADER_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    glShaderSource(sh, 1, &p, NULL);
    glCompileShader(sh);
    free(src);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) { /* GCOVR_EXCL_START: only with a broken shader or driver */
        char log[1024];
        glGetShaderInfoLog(sh, (GLsizei)sizeof log, NULL, log);
        CADV_ERROR("shader compile failed: %s", log);
        glDeleteShader(sh);
        return 0;
    } /* GCOVR_EXCL_STOP */
    return sh;
}

unsigned cadv_gl_program(cadv_gl_profile profile, const char *vs_body, const char *fs_body)
{
    GLuint vs = compile(profile, CADV_SHADER_VERTEX, vs_body);
    GLuint fs = compile(profile, CADV_SHADER_FRAGMENT, fs_body);
    GLuint prog;
    GLint ok = 0;
    if (!vs || !fs) { /* GCOVR_EXCL_BR_LINE: broken shader */
        return 0;     /* GCOVR_EXCL_LINE */
    }
    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "a_pos");
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { /* GCOVR_EXCL_START */
        char log[1024];
        glGetProgramInfoLog(prog, (GLsizei)sizeof log, NULL, log);
        CADV_ERROR("shader link failed: %s", log);
        glDeleteProgram(prog);
        return 0;
    } /* GCOVR_EXCL_STOP */
    return prog;
}

unsigned cadv_gl_texture(const cadv_texdata *t, bool repeat)
{
    GLuint tex;
    GLint wrap = repeat ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, t->w, t->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, t->rgba);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    return tex;
}

unsigned cadv_gl_lut3d(const uint8_t *rgb, int dim)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_3D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB8, dim, dim, dim, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return tex;
}

void cadv_gl_delete_textures(int n, const unsigned *textures)
{
    glDeleteTextures((GLsizei)n, textures);
}

void cadv_rt_ensure(cadv_rt *rt, int w, int h)
{
    if (rt->fbo && rt->w == w && rt->h == h) {
        return;
    }
    cadv_rt_free(rt);
    rt->w = w;
    rt->h = h;
    glGenTextures(1, &rt->color);
    glBindTexture(GL_TEXTURE_2D, rt->color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenRenderbuffers(1, &rt->depth);
    glBindRenderbuffer(GL_RENDERBUFFER, rt->depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glGenFramebuffers(1, &rt->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, rt->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rt->color, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rt->depth);
}

void cadv_rt_free(cadv_rt *rt)
{
    glDeleteFramebuffers(1, &rt->fbo);
    glDeleteTextures(1, &rt->color);
    glDeleteRenderbuffers(1, &rt->depth);
    memset(rt, 0, sizeof *rt);
}

static void shadow_params(GLenum target)
{
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
}

void cadv_shadow2d_init(cadv_shadow2d *s, int size)
{
    s->size = size;
    glGenTextures(1, &s->tex);
    glBindTexture(GL_TEXTURE_2D, s->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, NULL);
    shadow_params(GL_TEXTURE_2D);
    glGenFramebuffers(1, &s->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, s->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, s->tex, 0);
    glDrawBuffers(0, NULL);
    glReadBuffer(GL_NONE);
}

void cadv_shadow2d_free(cadv_shadow2d *s)
{
    glDeleteFramebuffers(1, &s->fbo);
    glDeleteTextures(1, &s->tex);
    memset(s, 0, sizeof *s);
}

void cadv_shadowcube_init(cadv_shadowcube *s, int size)
{
    int f;
    s->size = size;
    glGenTextures(1, &s->tex);
    glBindTexture(GL_TEXTURE_CUBE_MAP, s->tex);
    for (f = 0; f < 6; ++f) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (GLenum)f, 0, GL_DEPTH_COMPONENT24, size, size,
                     0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    }
    shadow_params(GL_TEXTURE_CUBE_MAP);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &s->fbo);
    cadv_shadowcube_bind_face(s, 0);
    glDrawBuffers(0, NULL);
    glReadBuffer(GL_NONE);
}

void cadv_shadowcube_bind_face(const cadv_shadowcube *s, int face)
{
    glBindFramebuffer(GL_FRAMEBUFFER, s->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_CUBE_MAP_POSITIVE_X + (GLenum)face, s->tex, 0);
}

void cadv_shadowcube_free(cadv_shadowcube *s)
{
    glDeleteFramebuffers(1, &s->fbo);
    glDeleteTextures(1, &s->tex);
    memset(s, 0, sizeof *s);
}

void cadv_gpu_mesh_upload(cadv_gpu_mesh *g, const cadv_mesh *m, bool dynamic)
{
    GLenum usage = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    GLsizei stride = (GLsizei)sizeof(cadv_vertex);
    if (!g->vao) {
        glGenVertexArrays(1, &g->vao);
        glGenBuffers(1, &g->vbo);
        glGenBuffers(1, &g->ibo);
        glBindVertexArray(g->vao);
        glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g->ibo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(cadv_vertex, pos));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(cadv_vertex, normal));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void *)offsetof(cadv_vertex, uv));
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void *)offsetof(cadv_vertex, color));
    } else {
        glBindVertexArray(g->vao);
        glBindBuffer(GL_ARRAY_BUFFER, g->vbo);
    }
    if (dynamic && m->nv <= g->capacity_v && m->ni <= g->capacity_i) {
        glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(m->nv * sizeof(cadv_vertex)), m->v);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, (GLsizeiptr)(m->ni * sizeof(uint32_t)), m->idx);
    } else {
        glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(m->nv * sizeof(cadv_vertex)), m->v, usage);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(m->ni * sizeof(uint32_t)), m->idx, usage);
        g->capacity_v = m->nv;
        g->capacity_i = m->ni;
    }
    g->index_count = (int)m->ni;
    glBindVertexArray(0);
}

void cadv_gpu_mesh_draw(const cadv_gpu_mesh *g)
{
    if (g->index_count <= 0) {
        return;
    }
    glBindVertexArray(g->vao);
    glDrawElements(GL_TRIANGLES, g->index_count, GL_UNSIGNED_INT, NULL);
    glBindVertexArray(0);
}

void cadv_gpu_mesh_free(cadv_gpu_mesh *g)
{
    glDeleteVertexArrays(1, &g->vao);
    glDeleteBuffers(1, &g->vbo);
    glDeleteBuffers(1, &g->ibo);
    memset(g, 0, sizeof *g);
}

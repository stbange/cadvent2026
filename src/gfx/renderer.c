/* SPDX-License-Identifier: MIT */
#include "gfx/renderer.h"

#include <stdlib.h>
#include <string.h>

#include "core/log.h"
#include "gfx/gl_api.h"

static const char k_vs[] =
    "layout(location = 0) in vec2 a_pos;\n"
    "void main() { gl_Position = vec4(a_pos, 0.0, 1.0); }\n";

static const char k_fs[] =
    "uniform vec3 u_color;\n"
    "out vec4 o_color;\n"
    "void main() { o_color = vec4(u_color, 1.0); }\n";

static GLuint compile(cadv_gl_profile profile, cadv_shader_stage stage, const char *body)
{
    char src[1024];
    char log[512];
    const char *p = src;
    GLint ok = 0;
    GLuint sh;
    cadv_shader_build(src, sizeof src, profile, stage, body);
    sh = glCreateShader(stage == CADV_SHADER_VERTEX ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
    glShaderSource(sh, 1, &p, NULL);
    glCompileShader(sh);
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        /* GCOVR_EXCL_START: only reachable with a broken driver or shader */
        glGetShaderInfoLog(sh, (GLsizei)sizeof log, NULL, log);
        CADV_ERROR("shader compile failed: %s", log);
        glDeleteShader(sh);
        return 0;
        /* GCOVR_EXCL_STOP */
    }
    return sh;
}

bool cadv_renderer_init(cadv_renderer *r, cadv_gl_profile profile)
{
    static const float verts[] = {0.0f, 0.7f, -0.6f, -0.6f, 0.6f, -0.6f};
    GLuint vs;
    GLuint fs;
    GLint ok = 0;
    memset(r, 0, sizeof *r);
    r->profile = profile;
    vs = compile(profile, CADV_SHADER_VERTEX, k_vs);
    fs = compile(profile, CADV_SHADER_FRAGMENT, k_fs);
    if (!vs || !fs) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return false; /* GCOVR_EXCL_LINE */
    }
    r->program = glCreateProgram();
    glAttachShader(r->program, vs);
    glAttachShader(r->program, fs);
    glLinkProgram(r->program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    glGetProgramiv(r->program, GL_LINK_STATUS, &ok);
    if (!ok) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        CADV_ERROR("shader link failed"); /* GCOVR_EXCL_LINE */
        return false;                     /* GCOVR_EXCL_LINE */
    }
    r->u_color = glGetUniformLocation(r->program, "u_color");
    glGenVertexArrays(1, &r->vao);
    glGenBuffers(1, &r->vbo);
    glBindVertexArray(r->vao);
    glBindBuffer(GL_ARRAY_BUFFER, r->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof verts, verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * (GLsizei)sizeof(float), (void *)0);
    glBindVertexArray(0);
    r->ready = true;
    CADV_INFO("renderer ready (%s): %s", cadv_gl_profile_name(profile),
              (const char *)glGetString(GL_RENDERER));
    return true;
}

void cadv_renderer_frame(cadv_renderer *r, int width, int height)
{
    glViewport(0, 0, width, height);
    glClearColor(CADV_CLEAR_R / 255.0f, CADV_CLEAR_G / 255.0f, CADV_CLEAR_B / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(r->program);
    glUniform3f(r->u_color, CADV_TREE_R / 255.0f, CADV_TREE_G / 255.0f, CADV_TREE_B / 255.0f);
    glBindVertexArray(r->vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

bool cadv_renderer_capture(cadv_renderer *r, int width, int height, cadv_image *img)
{
    unsigned char *rgba;
    bool ok;
    (void)r;
    rgba = (unsigned char *)malloc((size_t)width * (size_t)height * 4u);
    if (!rgba) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return false; /* GCOVR_EXCL_LINE */
    }
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
    ok = cadv_image_from_rgba(img, width, height, rgba);
    free(rgba);
    if (ok) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        cadv_image_flip_rows(img);
    }
    return ok;
}

void cadv_renderer_shutdown(cadv_renderer *r)
{
    /* Deleting name 0 is a no-op in GL, so this is safe after a failed init. */
    glDeleteBuffers(1, &r->vbo);
    glDeleteVertexArrays(1, &r->vao);
    glDeleteProgram(r->program);
    memset(r, 0, sizeof *r);
}

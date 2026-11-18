/* SPDX-License-Identifier: MIT */
#include "gfx/shader_src.h"

#include <stdio.h>

cadv_gl_profile cadv_gl_native_profile(void)
{
#if defined(__EMSCRIPTEN__) || defined(CADV_FORCE_GLES)
    return CADV_GL_ES_30;
#else
    return CADV_GL_DESKTOP_33;
#endif
}

const char *cadv_gl_profile_name(cadv_gl_profile p)
{
    return p == CADV_GL_ES_30 ? "OpenGL ES 3.0" : "OpenGL 3.3 Core";
}

size_t cadv_shader_build(char *out, size_t cap, cadv_gl_profile profile,
                         cadv_shader_stage stage, const char *body)
{
    static const char desktop[] = "#version 330 core\n#define CADV_GLES 0\n";
    static const char gles[] = "#version 300 es\n"
                               "precision highp float;\n"
                               "precision highp int;\n"
                               "#define CADV_GLES 1\n";
    const char *pre = profile == CADV_GL_ES_30 ? gles : desktop;
    const char *st = stage == CADV_SHADER_VERTEX ? "#define CADV_VERTEX 1\n"
                                                 : "#define CADV_FRAGMENT 1\n";
    int n;
    if (!body) {
        if (out && cap > 0) {
            out[0] = '\0';
        }
        return 0;
    }
    n = snprintf(out, cap, "%s%s#line 1\n%s", pre, st, body);
    return n < 0 ? 0 : (size_t)n;
}

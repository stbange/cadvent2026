/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_SHADER_SRC_H
#define CADV_GFX_SHADER_SRC_H

#include <stddef.h>

/* One GLSL body is shared by desktop GL 3.3 core and GLES 3.0 / WebGL 2; only
 * the preamble differs. Bodies may test CADV_GLES and CADV_VERTEX/CADV_FRAGMENT. */
typedef enum cadv_gl_profile {
    CADV_GL_DESKTOP_33 = 0,
    CADV_GL_ES_30 = 1
} cadv_gl_profile;

typedef enum cadv_shader_stage {
    CADV_SHADER_VERTEX = 0,
    CADV_SHADER_FRAGMENT = 1
} cadv_shader_stage;

/* The compile-time profile of this build. */
cadv_gl_profile cadv_gl_native_profile(void);
const char *cadv_gl_profile_name(cadv_gl_profile p);

/* Writes preamble + body into out (always NUL-terminated if cap > 0).
 * Returns the full length needed, excluding the NUL, like snprintf; a return
 * value >= cap means the output was truncated. Returns 0 if body is NULL. */
size_t cadv_shader_build(char *out, size_t cap, cadv_gl_profile profile,
                         cadv_shader_stage stage, const char *body);

#endif

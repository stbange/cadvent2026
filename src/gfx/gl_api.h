/* SPDX-License-Identifier: MIT */
#ifndef CADV_GFX_GL_API_H
#define CADV_GFX_GL_API_H

/* Single include point for OpenGL. Desktop uses the vendored glad loader
 * (GL 3.3 core), the web uses the GLES 3.0 headers provided by Emscripten. */
#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/gl.h>
#endif

#endif

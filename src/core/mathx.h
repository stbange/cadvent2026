/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_MATHX_H
#define CADV_CORE_MATHX_H

/* Small vector/matrix library. Trigonometry is implemented here (not libm) so
 * simulation results are bit-identical on every platform and compiler. */

#define CADV_PI 3.14159265358979323846f
#define CADV_TAU 6.28318530717958647692f

typedef struct cadv_vec2 { float x, y; } cadv_vec2;
typedef struct cadv_vec3 { float x, y, z; } cadv_vec3;
typedef struct cadv_vec4 { float x, y, z, w; } cadv_vec4;
/* Column-major 4x4 matrix (OpenGL convention): m[col * 4 + row]. */
typedef struct cadv_mat4 { float m[16]; } cadv_mat4;

float cadv_sinf(float x);
float cadv_cosf(float x);
float cadv_atan2f(float y, float x);
float cadv_sqrtf(float x); /* IEEE sqrt is exact, so this is deterministic */
float cadv_clampf(float v, float lo, float hi);
float cadv_lerpf(float a, float b, float t);
float cadv_wrap_angle(float a); /* to (-pi, pi] */
int cadv_mini(int a, int b);
int cadv_maxi(int a, int b);
float cadv_minf(float a, float b);
float cadv_maxf(float a, float b);
float cadv_absf(float a);

cadv_vec3 cadv_v3(float x, float y, float z);
cadv_vec3 cadv_v3_add(cadv_vec3 a, cadv_vec3 b);
cadv_vec3 cadv_v3_sub(cadv_vec3 a, cadv_vec3 b);
cadv_vec3 cadv_v3_scale(cadv_vec3 a, float s);
cadv_vec3 cadv_v3_mul(cadv_vec3 a, cadv_vec3 b);
float cadv_v3_dot(cadv_vec3 a, cadv_vec3 b);
cadv_vec3 cadv_v3_cross(cadv_vec3 a, cadv_vec3 b);
float cadv_v3_len(cadv_vec3 a);
float cadv_v3_dist(cadv_vec3 a, cadv_vec3 b);
cadv_vec3 cadv_v3_norm(cadv_vec3 a); /* zero vector stays zero */
cadv_vec3 cadv_v3_lerp(cadv_vec3 a, cadv_vec3 b, float t);
/* Unit direction from yaw (around +Y, 0 = -Z) and pitch (up positive). */
cadv_vec3 cadv_v3_from_yaw_pitch(float yaw, float pitch);

cadv_mat4 cadv_m4_identity(void);
cadv_mat4 cadv_m4_mul(cadv_mat4 a, cadv_mat4 b);
cadv_mat4 cadv_m4_translate(cadv_vec3 t);
cadv_mat4 cadv_m4_scale(cadv_vec3 s);
cadv_mat4 cadv_m4_rotate_y(float a);
cadv_mat4 cadv_m4_rotate_x(float a);
cadv_mat4 cadv_m4_perspective(float fovy, float aspect, float znear, float zfar);
cadv_mat4 cadv_m4_ortho(float l, float r, float b, float t, float n, float f);
cadv_mat4 cadv_m4_look_at(cadv_vec3 eye, cadv_vec3 target, cadv_vec3 up);
cadv_vec4 cadv_m4_mul_v4(cadv_mat4 m, cadv_vec4 v);
cadv_vec3 cadv_m4_mul_point(cadv_mat4 m, cadv_vec3 p); /* with perspective divide */

#endif

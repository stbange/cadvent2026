/* SPDX-License-Identifier: MIT */
#include "core/mathx.h"

#include <math.h>

float cadv_wrap_angle(float a)
{
    /* Bring a into (-pi, pi] with a deterministic float-only reduction. */
    if (a > CADV_PI || a <= -CADV_PI) {
        float k = (float)(int)(a / CADV_TAU);
        a -= k * CADV_TAU;
        if (a > CADV_PI) {
            a -= CADV_TAU;
        } else if (a <= -CADV_PI) {
            a += CADV_TAU;
        }
    }
    return a;
}

/* sin on [-pi/2, pi/2] by an odd polynomial (max error ~3e-6 after reduction). */
static float sin_core(float x)
{
    float x2 = x * x;
    return x * (1.0f + x2 * (-0.16666667f + x2 * (0.0083333310f + x2 * (-0.00019840874f +
                                                                         x2 * 2.7525562e-6f))));
}

float cadv_sinf(float x)
{
    x = cadv_wrap_angle(x);
    if (x > 0.5f * CADV_PI) {
        x = CADV_PI - x;
    } else if (x < -0.5f * CADV_PI) {
        x = -CADV_PI - x;
    }
    return sin_core(x);
}

float cadv_cosf(float x) { return cadv_sinf(x + 0.5f * CADV_PI); }

float cadv_atan2f(float y, float x)
{
    float ax = cadv_absf(x);
    float ay = cadv_absf(y);
    float mx = cadv_maxf(ax, ay);
    float a;
    float s;
    float r;
    if (mx == 0.0f) {
        return 0.0f;
    }
    a = cadv_minf(ax, ay) / mx;
    s = a * a;
    /* atan on [0,1] by a degree-11 odd polynomial, max error ~1e-5 rad */
    r = a * (0.99997726f +
             s * (-0.33262347f +
                  s * (0.19354346f + s * (-0.11643287f + s * (0.05265332f + s * -0.01172120f)))));
    if (ay > ax) {
        r = 0.5f * CADV_PI - r;
    }
    if (x < 0.0f) {
        r = CADV_PI - r;
    }
    return y < 0.0f ? -r : r;
}

float cadv_sqrtf(float x) { return x <= 0.0f ? 0.0f : sqrtf(x); }
float cadv_clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
float cadv_lerpf(float a, float b, float t) { return a + (b - a) * t; }
int cadv_mini(int a, int b) { return a < b ? a : b; }
int cadv_maxi(int a, int b) { return a > b ? a : b; }
float cadv_minf(float a, float b) { return a < b ? a : b; }
float cadv_maxf(float a, float b) { return a > b ? a : b; }
float cadv_absf(float a) { return a < 0.0f ? -a : a; }

cadv_vec3 cadv_v3(float x, float y, float z)
{
    cadv_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}
cadv_vec3 cadv_v3_add(cadv_vec3 a, cadv_vec3 b) { return cadv_v3(a.x + b.x, a.y + b.y, a.z + b.z); }
cadv_vec3 cadv_v3_sub(cadv_vec3 a, cadv_vec3 b) { return cadv_v3(a.x - b.x, a.y - b.y, a.z - b.z); }
cadv_vec3 cadv_v3_scale(cadv_vec3 a, float s) { return cadv_v3(a.x * s, a.y * s, a.z * s); }
cadv_vec3 cadv_v3_mul(cadv_vec3 a, cadv_vec3 b) { return cadv_v3(a.x * b.x, a.y * b.y, a.z * b.z); }
float cadv_v3_dot(cadv_vec3 a, cadv_vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
cadv_vec3 cadv_v3_cross(cadv_vec3 a, cadv_vec3 b)
{
    return cadv_v3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}
float cadv_v3_len(cadv_vec3 a) { return cadv_sqrtf(cadv_v3_dot(a, a)); }
float cadv_v3_dist(cadv_vec3 a, cadv_vec3 b) { return cadv_v3_len(cadv_v3_sub(a, b)); }
cadv_vec3 cadv_v3_norm(cadv_vec3 a)
{
    float l = cadv_v3_len(a);
    return l > 0.0f ? cadv_v3_scale(a, 1.0f / l) : a;
}
cadv_vec3 cadv_v3_lerp(cadv_vec3 a, cadv_vec3 b, float t)
{
    return cadv_v3_add(a, cadv_v3_scale(cadv_v3_sub(b, a), t));
}
cadv_vec3 cadv_v3_from_yaw_pitch(float yaw, float pitch)
{
    float cp = cadv_cosf(pitch);
    return cadv_v3(-cadv_sinf(yaw) * cp, cadv_sinf(pitch), -cadv_cosf(yaw) * cp);
}

cadv_mat4 cadv_m4_identity(void)
{
    cadv_mat4 r = {{0}};
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
}

cadv_mat4 cadv_m4_mul(cadv_mat4 a, cadv_mat4 b)
{
    cadv_mat4 r;
    int c;
    int k;
    for (c = 0; c < 4; ++c) {
        for (k = 0; k < 4; ++k) {
            r.m[c * 4 + k] = a.m[0 * 4 + k] * b.m[c * 4 + 0] + a.m[1 * 4 + k] * b.m[c * 4 + 1] +
                             a.m[2 * 4 + k] * b.m[c * 4 + 2] + a.m[3 * 4 + k] * b.m[c * 4 + 3];
        }
    }
    return r;
}

cadv_mat4 cadv_m4_translate(cadv_vec3 t)
{
    cadv_mat4 r = cadv_m4_identity();
    r.m[12] = t.x;
    r.m[13] = t.y;
    r.m[14] = t.z;
    return r;
}

cadv_mat4 cadv_m4_scale(cadv_vec3 s)
{
    cadv_mat4 r = cadv_m4_identity();
    r.m[0] = s.x;
    r.m[5] = s.y;
    r.m[10] = s.z;
    return r;
}

cadv_mat4 cadv_m4_rotate_y(float a)
{
    cadv_mat4 r = cadv_m4_identity();
    float c = cadv_cosf(a);
    float s = cadv_sinf(a);
    r.m[0] = c;
    r.m[2] = -s;
    r.m[8] = s;
    r.m[10] = c;
    return r;
}

cadv_mat4 cadv_m4_rotate_x(float a)
{
    cadv_mat4 r = cadv_m4_identity();
    float c = cadv_cosf(a);
    float s = cadv_sinf(a);
    r.m[5] = c;
    r.m[6] = s;
    r.m[9] = -s;
    r.m[10] = c;
    return r;
}

cadv_mat4 cadv_m4_perspective(float fovy, float aspect, float znear, float zfar)
{
    cadv_mat4 r = {{0}};
    float f = cadv_cosf(0.5f * fovy) / cadv_sinf(0.5f * fovy);
    r.m[0] = f / aspect;
    r.m[5] = f;
    r.m[10] = (zfar + znear) / (znear - zfar);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zfar * znear) / (znear - zfar);
    return r;
}

cadv_mat4 cadv_m4_ortho(float l, float r, float b, float t, float n, float f)
{
    cadv_mat4 o = cadv_m4_identity();
    o.m[0] = 2.0f / (r - l);
    o.m[5] = 2.0f / (t - b);
    o.m[10] = -2.0f / (f - n);
    o.m[12] = -(r + l) / (r - l);
    o.m[13] = -(t + b) / (t - b);
    o.m[14] = -(f + n) / (f - n);
    return o;
}

cadv_mat4 cadv_m4_look_at(cadv_vec3 eye, cadv_vec3 target, cadv_vec3 up)
{
    cadv_vec3 f = cadv_v3_norm(cadv_v3_sub(target, eye));
    cadv_vec3 s = cadv_v3_norm(cadv_v3_cross(f, up));
    cadv_vec3 u = cadv_v3_cross(s, f);
    cadv_mat4 r = cadv_m4_identity();
    r.m[0] = s.x;
    r.m[4] = s.y;
    r.m[8] = s.z;
    r.m[1] = u.x;
    r.m[5] = u.y;
    r.m[9] = u.z;
    r.m[2] = -f.x;
    r.m[6] = -f.y;
    r.m[10] = -f.z;
    r.m[12] = -cadv_v3_dot(s, eye);
    r.m[13] = -cadv_v3_dot(u, eye);
    r.m[14] = cadv_v3_dot(f, eye);
    return r;
}

cadv_vec4 cadv_m4_mul_v4(cadv_mat4 m, cadv_vec4 v)
{
    cadv_vec4 r;
    r.x = m.m[0] * v.x + m.m[4] * v.y + m.m[8] * v.z + m.m[12] * v.w;
    r.y = m.m[1] * v.x + m.m[5] * v.y + m.m[9] * v.z + m.m[13] * v.w;
    r.z = m.m[2] * v.x + m.m[6] * v.y + m.m[10] * v.z + m.m[14] * v.w;
    r.w = m.m[3] * v.x + m.m[7] * v.y + m.m[11] * v.z + m.m[15] * v.w;
    return r;
}

cadv_vec3 cadv_m4_mul_point(cadv_mat4 m, cadv_vec3 p)
{
    cadv_vec4 v;
    v.x = p.x;
    v.y = p.y;
    v.z = p.z;
    v.w = 1.0f;
    v = cadv_m4_mul_v4(m, v);
    if (v.w != 0.0f) {
        return cadv_v3(v.x / v.w, v.y / v.w, v.z / v.w);
    }
    return cadv_v3(v.x, v.y, v.z);
}

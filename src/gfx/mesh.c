/* SPDX-License-Identifier: MIT */
#include "gfx/mesh.h"

#include <stdlib.h>
#include <string.h>

cadv_rgba cadv_rgba_make(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    cadv_rgba c;
    c.r = r;
    c.g = g;
    c.b = b;
    c.a = a;
    return c;
}

void cadv_mesh_init(cadv_mesh *m) { memset(m, 0, sizeof *m); }

void cadv_mesh_free(cadv_mesh *m)
{
    free(m->v);
    free(m->idx);
    memset(m, 0, sizeof *m);
}

void cadv_mesh_clear(cadv_mesh *m)
{
    m->nv = 0;
    m->ni = 0;
    m->oom = false;
}

static bool grow(void **p, uint32_t *cap, uint32_t need, size_t elem)
{
    uint32_t nc;
    void *np;
    if (need <= *cap) {
        return true;
    }
    nc = *cap ? *cap : 64;
    while (nc < need) {
        nc *= 2;
    }
    np = realloc(*p, (size_t)nc * elem);
    if (!np) {           /* GCOVR_EXCL_BR_LINE: allocation failure */
        return false;    /* GCOVR_EXCL_LINE */
    }
    *p = np;
    *cap = nc;
    return true;
}

uint32_t cadv_mesh_vertex(cadv_mesh *m, cadv_vec3 p, cadv_vec3 n, float u, float v, cadv_rgba c)
{
    cadv_vertex *vx;
    if (!grow((void **)&m->v, &m->cap_v, m->nv + 1, sizeof(cadv_vertex))) { /* GCOVR_EXCL_BR_LINE */
        m->oom = true;  /* GCOVR_EXCL_LINE */
        return 0;       /* GCOVR_EXCL_LINE */
    }
    vx = &m->v[m->nv];
    vx->pos[0] = p.x;
    vx->pos[1] = p.y;
    vx->pos[2] = p.z;
    vx->normal[0] = n.x;
    vx->normal[1] = n.y;
    vx->normal[2] = n.z;
    vx->uv[0] = u;
    vx->uv[1] = v;
    vx->color[0] = c.r;
    vx->color[1] = c.g;
    vx->color[2] = c.b;
    vx->color[3] = c.a;
    return m->nv++;
}

void cadv_mesh_tri(cadv_mesh *m, uint32_t a, uint32_t b, uint32_t c)
{
    if (!grow((void **)&m->idx, &m->cap_i, m->ni + 3, sizeof(uint32_t))) { /* GCOVR_EXCL_BR_LINE */
        m->oom = true; /* GCOVR_EXCL_LINE */
        return;        /* GCOVR_EXCL_LINE */
    }
    m->idx[m->ni++] = a;
    m->idx[m->ni++] = b;
    m->idx[m->ni++] = c;
}

void cadv_mesh_quad(cadv_mesh *m, cadv_vec3 a, cadv_vec3 b, cadv_vec3 c, cadv_vec3 d, float us,
                    float vs, cadv_rgba col)
{
    cadv_vec3 n = cadv_v3_norm(cadv_v3_cross(cadv_v3_sub(b, a), cadv_v3_sub(d, a)));
    uint32_t i0 = cadv_mesh_vertex(m, a, n, 0.0f, vs, col);
    uint32_t i1 = cadv_mesh_vertex(m, b, n, us, vs, col);
    uint32_t i2 = cadv_mesh_vertex(m, c, n, us, 0.0f, col);
    uint32_t i3 = cadv_mesh_vertex(m, d, n, 0.0f, 0.0f, col);
    cadv_mesh_tri(m, i0, i1, i2);
    cadv_mesh_tri(m, i0, i2, i3);
}

void cadv_mesh_box(cadv_mesh *m, cadv_vec3 lo, cadv_vec3 hi, float uv_scale, cadv_rgba col,
                   bool inward)
{
    /* 8 corners: bit0 = x, bit1 = y, bit2 = z */
    cadv_vec3 p[8];
    /* corner order per face gives an outward normal via cadv_mesh_quad */
    static const int faces[6][4] = {
        {5, 1, 3, 7}, /* +X */
        {0, 4, 6, 2}, /* -X */
        {6, 7, 3, 2}, /* +Y */
        {0, 1, 5, 4}, /* -Y */
        {4, 5, 7, 6}, /* +Z */
        {1, 0, 2, 3}, /* -Z */
    };
    float sx = hi.x - lo.x;
    float sy = hi.y - lo.y;
    float sz = hi.z - lo.z;
    float fu[6];
    float fv[6];
    int i;
    for (i = 0; i < 8; ++i) {
        p[i] = cadv_v3((i & 1) ? hi.x : lo.x, (i & 2) ? hi.y : lo.y, (i & 4) ? hi.z : lo.z);
    }
    fu[0] = fu[1] = sz;
    fv[0] = fv[1] = sy;
    fu[2] = fu[3] = sx;
    fv[2] = fv[3] = sz;
    fu[4] = fu[5] = sx;
    fv[4] = fv[5] = sy;
    for (i = 0; i < 6; ++i) {
        const int *f = faces[i];
        float us = uv_scale > 0.0f ? fu[i] * uv_scale : 1.0f;
        float vs = uv_scale > 0.0f ? fv[i] * uv_scale : 1.0f;
        if (inward) {
            cadv_mesh_quad(m, p[f[3]], p[f[2]], p[f[1]], p[f[0]], us, vs, col);
        } else {
            cadv_mesh_quad(m, p[f[0]], p[f[1]], p[f[2]], p[f[3]], us, vs, col);
        }
    }
}

void cadv_mesh_prism(cadv_mesh *m, cadv_vec3 base, float radius, float height, int sides,
                     float uv_scale, cadv_rgba col)
{
    int i;
    float circumference = CADV_TAU * radius;
    if (sides < 3) {
        sides = 3;
    }
    for (i = 0; i < sides; ++i) {
        float a0 = CADV_TAU * (float)i / (float)sides;
        float a1 = CADV_TAU * (float)(i + 1) / (float)sides;
        cadv_vec3 p0 = cadv_v3(base.x + cadv_cosf(a0) * radius, base.y, base.z - cadv_sinf(a0) * radius);
        cadv_vec3 p1 = cadv_v3(base.x + cadv_cosf(a1) * radius, base.y, base.z - cadv_sinf(a1) * radius);
        cadv_vec3 p2 = cadv_v3(p1.x, base.y + height, p1.z);
        cadv_vec3 p3 = cadv_v3(p0.x, base.y + height, p0.z);
        float us = uv_scale > 0.0f ? circumference * uv_scale / (float)sides : 1.0f;
        float vs = uv_scale > 0.0f ? height * uv_scale : 1.0f;
        cadv_mesh_quad(m, p0, p1, p2, p3, us, vs, col);
    }
}

void cadv_mesh_arch(cadv_mesh *m, cadv_vec3 center, float width, float height, float frame,
                    float depth, int segments, cadv_rgba col)
{
    float r = 0.5f * width;
    float leg = height - r; /* straight part below the half circle */
    float hd = 0.5f * depth;
    int i;
    /* two legs */
    cadv_mesh_box(m, cadv_v3(center.x - r - frame, center.y, center.z - hd),
                  cadv_v3(center.x - r, center.y + leg, center.z + hd), 1.0f, col, false);
    cadv_mesh_box(m, cadv_v3(center.x + r, center.y, center.z - hd),
                  cadv_v3(center.x + r + frame, center.y + leg, center.z + hd), 1.0f, col, false);
    if (segments < 2) {
        segments = 2;
    }
    /* half ring made of quads (front, back, inner, outer) */
    for (i = 0; i < segments; ++i) {
        float a0 = CADV_PI * (float)i / (float)segments;
        float a1 = CADV_PI * (float)(i + 1) / (float)segments;
        float c0 = cadv_cosf(a0);
        float s0 = cadv_sinf(a0);
        float c1 = cadv_cosf(a1);
        float s1 = cadv_sinf(a1);
        float cy = center.y + leg;
        cadv_vec3 i0 = cadv_v3(center.x + c0 * r, cy + s0 * r, 0);
        cadv_vec3 i1 = cadv_v3(center.x + c1 * r, cy + s1 * r, 0);
        cadv_vec3 o0 = cadv_v3(center.x + c0 * (r + frame), cy + s0 * (r + frame), 0);
        cadv_vec3 o1 = cadv_v3(center.x + c1 * (r + frame), cy + s1 * (r + frame), 0);
        cadv_vec3 fi0 = cadv_v3(i0.x, i0.y, center.z + hd);
        cadv_vec3 fi1 = cadv_v3(i1.x, i1.y, center.z + hd);
        cadv_vec3 fo0 = cadv_v3(o0.x, o0.y, center.z + hd);
        cadv_vec3 fo1 = cadv_v3(o1.x, o1.y, center.z + hd);
        cadv_vec3 bi0 = cadv_v3(i0.x, i0.y, center.z - hd);
        cadv_vec3 bi1 = cadv_v3(i1.x, i1.y, center.z - hd);
        cadv_vec3 bo0 = cadv_v3(o0.x, o0.y, center.z - hd);
        cadv_vec3 bo1 = cadv_v3(o1.x, o1.y, center.z - hd);
        cadv_mesh_quad(m, fi0, fo0, fo1, fi1, 1.0f, 1.0f, col); /* front */
        cadv_mesh_quad(m, bi1, bo1, bo0, bi0, 1.0f, 1.0f, col); /* back */
        cadv_mesh_quad(m, fo1, fo0, bo0, bo1, 1.0f, 1.0f, col); /* outer */
        cadv_mesh_quad(m, fi0, fi1, bi1, bi0, 1.0f, 1.0f, col); /* inner */
    }
}

void cadv_mesh_append(cadv_mesh *m, const cadv_mesh *src, cadv_mat4 xf)
{
    uint32_t base = m->nv;
    uint32_t i;
    for (i = 0; i < src->nv; ++i) {
        const cadv_vertex *s = &src->v[i];
        cadv_vec3 p = cadv_m4_mul_point(xf, cadv_v3(s->pos[0], s->pos[1], s->pos[2]));
        cadv_vec4 n4;
        cadv_vec3 n;
        n4.x = s->normal[0];
        n4.y = s->normal[1];
        n4.z = s->normal[2];
        n4.w = 0.0f;
        n4 = cadv_m4_mul_v4(xf, n4);
        n = cadv_v3_norm(cadv_v3(n4.x, n4.y, n4.z));
        cadv_mesh_vertex(m, p, n, s->uv[0], s->uv[1],
                         cadv_rgba_make(s->color[0], s->color[1], s->color[2], s->color[3]));
    }
    for (i = 0; i + 2 < src->ni; i += 3) {
        cadv_mesh_tri(m, base + src->idx[i], base + src->idx[i + 1], base + src->idx[i + 2]);
    }
}

bool cadv_mesh_bounds(const cadv_mesh *m, cadv_vec3 *lo, cadv_vec3 *hi)
{
    uint32_t i;
    if (m->nv == 0) {
        return false;
    }
    *lo = cadv_v3(m->v[0].pos[0], m->v[0].pos[1], m->v[0].pos[2]);
    *hi = *lo;
    for (i = 1; i < m->nv; ++i) {
        const float *p = m->v[i].pos;
        lo->x = cadv_minf(lo->x, p[0]);
        lo->y = cadv_minf(lo->y, p[1]);
        lo->z = cadv_minf(lo->z, p[2]);
        hi->x = cadv_maxf(hi->x, p[0]);
        hi->y = cadv_maxf(hi->y, p[1]);
        hi->z = cadv_maxf(hi->z, p[2]);
    }
    return true;
}

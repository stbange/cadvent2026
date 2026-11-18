/* SPDX-License-Identifier: MIT */
#include "gfx/voxel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/mathx.h"

bool cadv_voxels_alloc(cadv_voxels *v, int w, int h, int d)
{
    int i;
    memset(v, 0, sizeof *v);
    if (w <= 0 || h <= 0 || d <= 0 || w > 256 || h > 256 || d > 256) {
        return false;
    }
    v->cells = (uint8_t *)calloc((size_t)w * (size_t)h * (size_t)d, 1u);
    if (!v->cells) {   /* GCOVR_EXCL_BR_LINE: allocation failure */
        return false;  /* GCOVR_EXCL_LINE */
    }
    v->w = w;
    v->h = h;
    v->d = d;
    for (i = 0; i < 256; ++i) {
        uint8_t g = (uint8_t)i;
        v->pal[i] = cadv_rgba_make(g, g, g, 255);
    }
    return true;
}

void cadv_voxels_free(cadv_voxels *v)
{
    free(v->cells);
    memset(v, 0, sizeof *v);
}

uint8_t cadv_voxels_get(const cadv_voxels *v, int x, int y, int z)
{
    if (x < 0 || y < 0 || z < 0 || x >= v->w || y >= v->h || z >= v->d) {
        return 0;
    }
    return v->cells[((size_t)z * (size_t)v->h + (size_t)y) * (size_t)v->w + (size_t)x];
}

void cadv_voxels_set(cadv_voxels *v, int x, int y, int z, uint8_t c)
{
    if (x < 0 || y < 0 || z < 0 || x >= v->w || y >= v->h || z >= v->d) {
        return;
    }
    v->cells[((size_t)z * (size_t)v->h + (size_t)y) * (size_t)v->w + (size_t)x] = c;
}

int cadv_voxels_count(const cadv_voxels *v)
{
    size_t n = (size_t)v->w * (size_t)v->h * (size_t)v->d;
    size_t i;
    int c = 0;
    for (i = 0; i < n; ++i) {
        c += v->cells[i] != 0;
    }
    return c;
}

/* ---- .vox reading ------------------------------------------------------- */

static uint32_t rd32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool cadv_vox_parse(cadv_voxels *v, const uint8_t *data, size_t len)
{
    size_t pos;
    size_t main_end;
    bool have_size = false;
    bool have_xyzi = false;
    int sx = 0, sy = 0, sz = 0;
    memset(v, 0, sizeof *v);
    if (len < 20 || memcmp(data, "VOX ", 4) != 0 || memcmp(data + 8, "MAIN", 4) != 0) {
        return false;
    }
    main_end = 20 + (size_t)rd32(data + 12) + (size_t)rd32(data + 16);
    if (main_end > len) {
        return false;
    }
    pos = 20 + (size_t)rd32(data + 12);
    while (pos + 12 <= main_end) {
        const uint8_t *id = data + pos;
        size_t content = rd32(data + pos + 4);
        size_t children = rd32(data + pos + 8);
        const uint8_t *c = data + pos + 12;
        if (pos + 12 + content + children > main_end) {
            cadv_voxels_free(v);
            return false;
        }
        if (memcmp(id, "SIZE", 4) == 0 && content >= 12 && !have_size) {
            sx = (int)rd32(c);
            sy = (int)rd32(c + 4);
            sz = (int)rd32(c + 8);
            /* .vox is Z-up: our (w, h, d) = (x, z, y) */
            if (!cadv_voxels_alloc(v, sx, sz, sy)) {
                return false;
            }
            have_size = true;
        } else if (memcmp(id, "XYZI", 4) == 0 && have_size && !have_xyzi && content >= 4) {
            uint32_t n = rd32(c);
            uint32_t i;
            if ((size_t)n * 4u + 4u > content) {
                cadv_voxels_free(v);
                return false;
            }
            for (i = 0; i < n; ++i) {
                const uint8_t *e = c + 4 + i * 4;
                cadv_voxels_set(v, e[0], e[2], e[1], e[3]);
            }
            have_xyzi = true;
        } else if (memcmp(id, "RGBA", 4) == 0 && have_size && content >= 1024) {
            int i;
            for (i = 0; i < 255; ++i) {
                v->pal[i + 1] = cadv_rgba_make(c[i * 4], c[i * 4 + 1], c[i * 4 + 2], c[i * 4 + 3]);
            }
        }
        pos += 12 + content + children;
    }
    if (!have_xyzi) {
        cadv_voxels_free(v);
        return false;
    }
    return true;
}

bool cadv_vox_load(cadv_voxels *v, const char *path)
{
    FILE *f = fopen(path, "rb");
    uint8_t *buf;
    long n;
    bool ok;
    memset(v, 0, sizeof *v);
    if (!f) {
        return false;
    }
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n <= 0 || n > 64L * 1024L * 1024L) {
        fclose(f);
        return false;
    }
    buf = (uint8_t *)malloc((size_t)n);
    if (!buf) {          /* GCOVR_EXCL_BR_LINE: allocation failure */
        fclose(f);       /* GCOVR_EXCL_LINE */
        return false;    /* GCOVR_EXCL_LINE */
    }
    ok = fread(buf, 1, (size_t)n, f) == (size_t)n;
    fclose(f);
    ok = ok && cadv_vox_parse(v, buf, (size_t)n);
    free(buf);
    return ok;
}

/* ---- .vox writing ------------------------------------------------------- */

static size_t put32(uint8_t *out, size_t cap, size_t at, uint32_t x)
{
    if (out && at + 4 <= cap) {
        out[at] = (uint8_t)(x & 0xffu);
        out[at + 1] = (uint8_t)((x >> 8) & 0xffu);
        out[at + 2] = (uint8_t)((x >> 16) & 0xffu);
        out[at + 3] = (uint8_t)(x >> 24);
    }
    return at + 4;
}

static size_t put_tag(uint8_t *out, size_t cap, size_t at, const char *tag)
{
    if (out && at + 4 <= cap) {
        memcpy(out + at, tag, 4);
    }
    return at + 4;
}

static size_t put8(uint8_t *out, size_t cap, size_t at, uint8_t x)
{
    if (out && at < cap) {
        out[at] = x;
    }
    return at + 1;
}

size_t cadv_vox_write(const cadv_voxels *v, uint8_t *out, size_t cap)
{
    uint32_t n = (uint32_t)cadv_voxels_count(v);
    uint32_t size_chunk = 12 + 12;
    uint32_t xyzi_chunk = 12 + 4 + n * 4;
    uint32_t rgba_chunk = 12 + 1024;
    size_t at = 0;
    int x, y, z, i;
    at = put_tag(out, cap, at, "VOX ");
    at = put32(out, cap, at, 150);
    at = put_tag(out, cap, at, "MAIN");
    at = put32(out, cap, at, 0);
    at = put32(out, cap, at, size_chunk + xyzi_chunk + rgba_chunk);
    at = put_tag(out, cap, at, "SIZE");
    at = put32(out, cap, at, 12);
    at = put32(out, cap, at, 0);
    at = put32(out, cap, at, (uint32_t)v->w);
    at = put32(out, cap, at, (uint32_t)v->d);
    at = put32(out, cap, at, (uint32_t)v->h);
    at = put_tag(out, cap, at, "XYZI");
    at = put32(out, cap, at, 4 + n * 4);
    at = put32(out, cap, at, 0);
    at = put32(out, cap, at, n);
    for (z = 0; z < v->d; ++z) {
        for (y = 0; y < v->h; ++y) {
            for (x = 0; x < v->w; ++x) {
                uint8_t c = cadv_voxels_get(v, x, y, z);
                if (c) {
                    at = put8(out, cap, at, (uint8_t)x);
                    at = put8(out, cap, at, (uint8_t)z);
                    at = put8(out, cap, at, (uint8_t)y);
                    at = put8(out, cap, at, c);
                }
            }
        }
    }
    at = put_tag(out, cap, at, "RGBA");
    at = put32(out, cap, at, 1024);
    at = put32(out, cap, at, 0);
    for (i = 1; i <= 256; ++i) {
        cadv_rgba c = v->pal[i & 255];
        at = put8(out, cap, at, c.r);
        at = put8(out, cap, at, c.g);
        at = put8(out, cap, at, c.b);
        at = put8(out, cap, at, c.a);
    }
    return at;
}

/* ---- greedy meshing ------------------------------------------------------ */

static uint8_t cell(const cadv_voxels *v, const int p[3]) { return cadv_voxels_get(v, p[0], p[1], p[2]); }

void cadv_voxels_mesh(const cadv_voxels *v, float scale, cadv_mesh *out)
{
    int dims[3];
    int d;
    int *mask;
    dims[0] = v->w;
    dims[1] = v->h;
    dims[2] = v->d;
    mask = (int *)malloc(sizeof(int) * 256u * 256u);
    if (!mask) {    /* GCOVR_EXCL_BR_LINE: allocation failure */
        return;     /* GCOVR_EXCL_LINE */
    }
    for (d = 0; d < 3; ++d) {
        int u = (d + 1) % 3;
        int w = (d + 2) % 3;
        int x[3] = {0, 0, 0};
        int q[3] = {0, 0, 0};
        q[d] = 1;
        for (x[d] = -1; x[d] < dims[d];) {
            int n = 0;
            int i, j;
            for (x[w] = 0; x[w] < dims[w]; ++x[w]) {
                for (x[u] = 0; x[u] < dims[u]; ++x[u]) {
                    int nb[3];
                    uint8_t a;
                    uint8_t b;
                    nb[0] = x[0] + q[0];
                    nb[1] = x[1] + q[1];
                    nb[2] = x[2] + q[2];
                    a = x[d] >= 0 ? cell(v, x) : 0;
                    b = x[d] < dims[d] - 1 ? cell(v, nb) : 0;
                    mask[n++] = (a != 0) == (b != 0) ? 0 : (a ? (int)a : -(int)b);
                }
            }
            ++x[d];
            n = 0;
            for (j = 0; j < dims[w]; ++j) {
                for (i = 0; i < dims[u];) {
                    int c = mask[n];
                    int wd;
                    int ht;
                    int k, l;
                    bool done = false;
                    if (!c) {
                        ++i;
                        ++n;
                        continue;
                    }
                    for (wd = 1; i + wd < dims[u] && mask[n + wd] == c; ++wd) {
                    }
                    for (ht = 1; j + ht < dims[w]; ++ht) {
                        for (k = 0; k < wd; ++k) {
                            if (mask[n + k + ht * dims[u]] != c) {
                                done = true;
                                break;
                            }
                        }
                        if (done) {
                            break;
                        }
                    }
                    {
                        float base[3];
                        float du[3] = {0, 0, 0};
                        float dv[3] = {0, 0, 0};
                        cadv_vec3 p[4];
                        cadv_rgba col = v->pal[c > 0 ? c : -c];
                        int m;
                        base[d] = (float)x[d];
                        base[u] = (float)i;
                        base[w] = (float)j;
                        du[u] = (float)wd;
                        dv[w] = (float)ht;
                        for (m = 0; m < 4; ++m) {
                            float px = base[0] + ((m == 1 || m == 2) ? du[0] : 0) + ((m >= 2) ? dv[0] : 0);
                            float py = base[1] + ((m == 1 || m == 2) ? du[1] : 0) + ((m >= 2) ? dv[1] : 0);
                            float pz = base[2] + ((m == 1 || m == 2) ? du[2] : 0) + ((m >= 2) ? dv[2] : 0);
                            p[m] = cadv_v3((px - 0.5f * (float)v->w) * scale, py * scale,
                                           (pz - 0.5f * (float)v->d) * scale);
                        }
                        if (c > 0) {
                            cadv_mesh_quad(out, p[0], p[1], p[2], p[3], (float)wd, (float)ht, col);
                        } else {
                            cadv_mesh_quad(out, p[3], p[2], p[1], p[0], (float)wd, (float)ht, col);
                        }
                    }
                    for (l = 0; l < ht; ++l) {
                        for (k = 0; k < wd; ++k) {
                            mask[n + k + l * dims[u]] = 0;
                        }
                    }
                    i += wd;
                    n += wd;
                }
            }
        }
    }
    free(mask);
}

/* ---- sprite baking ----------------------------------------------------------- */

bool cadv_voxels_bake_sprites(const cadv_voxels *v, int dirs, int ppv, cadv_texdata *color,
                              cadv_texdata *normals, int *fw, int *fh)
{
    int ext;
    int f;
    float diag;
    if (dirs < 1 || dirs > 16 || ppv < 1 || ppv > 8) {
        return false;
    }
    diag = cadv_sqrtf((float)(v->w * v->w + v->d * v->d));
    ext = (int)diag + 1;
    *fw = ext * ppv;
    *fh = v->h * ppv;
    if (!cadv_texdata_alloc(color, *fw * dirs, *fh)) {
        return false;
    }
    if (normals && !cadv_texdata_alloc(normals, *fw * dirs, *fh)) { /* GCOVR_EXCL_BR_LINE */
        cadv_texdata_free(color);                                   /* GCOVR_EXCL_LINE */
        return false;                                               /* GCOVR_EXCL_LINE */
    }
    for (f = 0; f < dirs; ++f) {
        float th = CADV_TAU * (float)f / (float)dirs;
        cadv_vec3 view = cadv_v3(-cadv_sinf(th), 0.0f, -cadv_cosf(th)); /* ray direction */
        cadv_vec3 right = cadv_v3(cadv_cosf(th), 0.0f, -cadv_sinf(th));
        cadv_vec3 centre = cadv_v3(0.5f * (float)v->w, 0.0f, 0.5f * (float)v->d);
        int px, py;
        for (py = 0; py < *fh; ++py) {
            for (px = 0; px < *fw; ++px) {
                float s = ((float)px + 0.5f) / (float)ppv - 0.5f * (float)ext;
                float t = (float)v->h - ((float)py + 0.5f) / (float)ppv;
                cadv_vec3 o = cadv_v3_add(cadv_v3_add(centre, cadv_v3_scale(right, s)),
                                          cadv_v3_add(cadv_v3(0, t, 0), cadv_v3_scale(view, -(float)ext)));
                int prev[3] = {(int)(o.x + 1000.0f) - 1000, (int)(o.y + 1000.0f) - 1000,
                               (int)(o.z + 1000.0f) - 1000};
                float step = 0.05f;
                float dist;
                for (dist = 0.0f; dist < 2.0f * (float)ext; dist += step) {
                    cadv_vec3 p = cadv_v3_add(o, cadv_v3_scale(view, dist));
                    int c[3] = {(int)(p.x + 1000.0f) - 1000, (int)(p.y + 1000.0f) - 1000,
                                (int)(p.z + 1000.0f) - 1000};
                    uint8_t idx = cadv_voxels_get(v, c[0], c[1], c[2]);
                    if (idx) {
                        cadv_rgba col = v->pal[idx];
                        cadv_vec3 n = cadv_v3(0, 0, 0);
                        int ox = *fw * f + px;
                        if (c[0] != prev[0]) {
                            n.x = c[0] > prev[0] ? -1.0f : 1.0f;
                        } else if (c[1] != prev[1]) {
                            n.y = c[1] > prev[1] ? -1.0f : 1.0f;
                        } else {
                            n.z = c[2] > prev[2] ? -1.0f : 1.0f;
                        }
                        cadv_texdata_set(color, ox, py, col.r, col.g, col.b, 255);
                        if (normals) {
                            float nx = cadv_v3_dot(n, right);
                            float nz = -cadv_v3_dot(n, view);
                            cadv_texdata_set(normals, ox, py, (uint8_t)((nx * 0.5f + 0.5f) * 255.0f),
                                             (uint8_t)((n.y * 0.5f + 0.5f) * 255.0f),
                                             (uint8_t)((nz * 0.5f + 0.5f) * 255.0f), 255);
                        }
                        break;
                    }
                    prev[0] = c[0];
                    prev[1] = c[1];
                    prev[2] = c[2];
                }
            }
        }
    }
    return true;
}

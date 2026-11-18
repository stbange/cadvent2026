/* SPDX-License-Identifier: MIT */
#include "gfx/procgen.h"

#include "core/mathx.h"

static uint32_t hash3(uint32_t seed, int x, int y)
{
    uint32_t h = seed * 0x9E3779B1u ^ (uint32_t)x * 0x85EBCA77u ^ (uint32_t)y * 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x2C1B3C6Du;
    h ^= h >> 12;
    h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}

static float lattice(uint32_t seed, int x, int y, int period)
{
    x %= period;
    y %= period;
    if (x < 0) {
        x += period;
    }
    if (y < 0) {
        y += period;
    }
    return (float)(hash3(seed, x, y) & 0xFFFFu) / 65535.0f;
}

static float smooth(float t) { return t * t * (3.0f - 2.0f * t); }

float cadv_noise2(uint32_t seed, float x, float y, int period)
{
    int xi = (int)x;
    int yi = (int)y;
    float fx;
    float fy;
    float a, b, c, d;
    if (x < 0.0f && (float)xi != x) {
        --xi;
    }
    if (y < 0.0f && (float)yi != y) {
        --yi;
    }
    fx = smooth(x - (float)xi);
    fy = smooth(y - (float)yi);
    if (period < 1) {
        period = 1;
    }
    a = lattice(seed, xi, yi, period);
    b = lattice(seed, xi + 1, yi, period);
    c = lattice(seed, xi, yi + 1, period);
    d = lattice(seed, xi + 1, yi + 1, period);
    return cadv_lerpf(cadv_lerpf(a, b, fx), cadv_lerpf(c, d, fx), fy);
}

float cadv_fbm2(uint32_t seed, float x, float y, int period, int octaves)
{
    float sum = 0.0f;
    float amp = 0.5f;
    float norm = 0.0f;
    int i;
    for (i = 0; i < octaves; ++i) {
        sum += amp * cadv_noise2(seed + (uint32_t)i * 101u, x, y, period);
        norm += amp;
        amp *= 0.5f;
        x *= 2.0f;
        y *= 2.0f;
        period *= 2;
    }
    return norm > 0.0f ? sum / norm : 0.0f;
}

static uint8_t shade(uint8_t c, float k)
{
    float v = (float)c * k;
    return (uint8_t)cadv_clampf(v, 0.0f, 255.0f);
}

static void put(cadv_texdata *t, int x, int y, cadv_rgba c, float k)
{
    cadv_texdata_set(t, x, y, shade(c.r, k), shade(c.g, k), shade(c.b, k), c.a);
}

static cadv_rgba mix(cadv_rgba a, cadv_rgba b, float t)
{
    return cadv_rgba_make((uint8_t)cadv_lerpf(a.r, b.r, t), (uint8_t)cadv_lerpf(a.g, b.g, t),
                          (uint8_t)cadv_lerpf(a.b, b.b, t), (uint8_t)cadv_lerpf(a.a, b.a, t));
}

bool cadv_procgen_texture(cadv_texdata *out, cadv_tex_kind kind, int size, uint32_t seed,
                          cadv_rgba base, cadv_rgba accent)
{
    int x;
    int y;
    if (size < 8 || size > 256 || (size & (size - 1)) != 0 || kind < 0 ||
        kind >= CADV_TEX_KIND_COUNT || !cadv_texdata_alloc(out, size, size)) {
        return false;
    }
    for (y = 0; y < size; ++y) {
        for (x = 0; x < size; ++x) {
            float u = (float)x / (float)size;
            float v = (float)y / (float)size;
            float n = cadv_fbm2(seed, u * 8.0f, v * 8.0f, 8, 3);
            switch (kind) {
            case CADV_TEX_STONE_BRICKS: {
                /* 4 rows of bricks, odd rows offset by half a brick */
                int row = y / (size / 4);
                int bx = (x + (row & 1) * (size / 4)) % (size / 2);
                int by = y % (size / 4);
                bool mortar = bx < 1 || by < 1;
                float var = 0.75f + 0.35f * cadv_noise2(seed + 7u, (float)((x + (row & 1) * (size / 4)) / (size / 2)), (float)row, 64);
                put(out, x, y, mortar ? accent : base, mortar ? 0.8f + 0.2f * n : var * (0.8f + 0.4f * n));
                break;
            }
            case CADV_TEX_PLANKS: {
                int plank = x / (size / 4);
                bool gap = x % (size / 4) == 0;
                float grain = cadv_noise2(seed + (uint32_t)plank, u * 2.0f, v * 24.0f, 24);
                put(out, x, y, gap ? accent : base, gap ? 0.6f : 0.75f + 0.4f * grain);
                break;
            }
            case CADV_TEX_FLOOR_TILES: {
                bool checker = ((x / (size / 2)) + (y / (size / 2))) & 1;
                bool seam = x % (size / 2) == 0 || y % (size / 2) == 0;
                float vein = cadv_fbm2(seed + 3u, u * 4.0f, v * 4.0f, 4, 4);
                cadv_rgba c = checker ? accent : base;
                put(out, x, y, c, seam ? 0.55f : 0.8f + 0.3f * vein);
                break;
            }
            case CADV_TEX_ICE: {
                float crack = cadv_absf(cadv_fbm2(seed + 9u, u * 4.0f, v * 4.0f, 4, 4) - 0.5f);
                put(out, x, y, mix(base, accent, n), crack < 0.02f ? 1.35f : 0.9f + 0.2f * n);
                break;
            }
            case CADV_TEX_CLOTH: {
                bool weave = ((x + y) & 1) != 0;
                bool border = x < 3 || x >= size - 3;
                put(out, x, y, border ? accent : base, (weave ? 0.92f : 1.0f) * (0.85f + 0.2f * n));
                break;
            }
            case CADV_TEX_METAL: {
                float brushed = cadv_noise2(seed, u * 64.0f, v * 2.0f, 64);
                bool rivet = (x % (size / 2) == size / 8) && (y % (size / 2) == size / 8);
                put(out, x, y, rivet ? accent : base, 0.8f + 0.3f * brushed);
                break;
            }
            case CADV_TEX_PLASTER:
                put(out, x, y, mix(base, accent, n * n), 0.9f + 0.15f * n);
                break;
            default:
                put(out, x, y, mix(base, accent, n), 1.0f);
                break;
            }
        }
    }
    return true;
}

/* SPDX-License-Identifier: MIT */
#include "core/rng.h"

void cadv_rng_seed(cadv_rng *r, uint64_t seed, uint64_t stream)
{
    r->state = 0u;
    r->inc = (stream << 1u) | 1u;
    (void)cadv_rng_next(r);
    r->state += seed;
    (void)cadv_rng_next(r);
}

uint32_t cadv_rng_next(cadv_rng *r)
{
    uint64_t old = r->state;
    uint32_t xorshifted;
    uint32_t rot;
    r->state = old * 6364136223846793005ULL + r->inc;
    xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
    rot = (uint32_t)(old >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
}

uint32_t cadv_rng_below(cadv_rng *r, uint32_t bound)
{
    uint32_t threshold;
    if (bound == 0u) {
        return 0u;
    }
    threshold = (0u - bound) % bound; /* == 2^32 mod bound */
    for (;;) {
        uint32_t v = cadv_rng_next(r);
        if (v >= threshold) {
            return v % bound;
        }
    }
}

int32_t cadv_rng_range(cadv_rng *r, int32_t lo, int32_t hi)
{
    uint32_t span;
    if (lo > hi) {
        int32_t t = lo;
        lo = hi;
        hi = t;
    }
    span = (uint32_t)((int64_t)hi - (int64_t)lo);
    if (span == UINT32_MAX) {
        return (int32_t)((int64_t)lo + (int64_t)cadv_rng_next(r));
    }
    return (int32_t)((int64_t)lo + (int64_t)cadv_rng_below(r, span + 1u));
}

float cadv_rng_float01(cadv_rng *r)
{
    return (float)(cadv_rng_next(r) >> 8) * (1.0f / 16777216.0f);
}

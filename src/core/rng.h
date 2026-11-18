/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_RNG_H
#define CADV_CORE_RNG_H

#include <stdint.h>

/* PCG32 (O'Neill, pcg-random.org, Apache-2.0/MIT algorithm description).
 * Deterministic on every platform: integer only. */
typedef struct cadv_rng {
    uint64_t state;
    uint64_t inc;
} cadv_rng;

void cadv_rng_seed(cadv_rng *r, uint64_t seed, uint64_t stream);
uint32_t cadv_rng_next(cadv_rng *r);
/* Uniform in [0, bound) without modulo bias; bound 0 returns 0. */
uint32_t cadv_rng_below(cadv_rng *r, uint32_t bound);
/* Uniform in [lo, hi] inclusive; if lo > hi the bounds are swapped. */
int32_t cadv_rng_range(cadv_rng *r, int32_t lo, int32_t hi);
/* Uniform float in [0, 1) built from 24 random bits. */
float cadv_rng_float01(cadv_rng *r);

#endif

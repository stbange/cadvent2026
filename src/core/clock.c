/* SPDX-License-Identifier: MIT */
#include "core/clock.h"

void cadv_clock_init(cadv_clock *c)
{
    c->accumulator = 0.0;
    c->tick = 0;
}

int cadv_clock_advance(cadv_clock *c, double elapsed_seconds, double speed)
{
    int n = 0;
    if (elapsed_seconds < 0.0) {
        elapsed_seconds = 0.0;
    }
    if (elapsed_seconds > 0.25) {
        elapsed_seconds = 0.25; /* e.g. after a breakpoint or tab switch */
    }
    c->accumulator += elapsed_seconds * speed;
    while (c->accumulator >= CADV_TICK_DT && n < CADV_MAX_TICKS_PER_FRAME) {
        c->accumulator -= CADV_TICK_DT;
        ++n;
    }
    if (n == CADV_MAX_TICKS_PER_FRAME && c->accumulator > CADV_TICK_DT) {
        c->accumulator = 0.0; /* drop the backlog instead of falling behind */
    }
    c->tick += (unsigned long long)n;
    return n;
}

float cadv_clock_alpha(const cadv_clock *c) { return (float)(c->accumulator / CADV_TICK_DT); }

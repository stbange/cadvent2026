/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_CLOCK_H
#define CADV_CORE_CLOCK_H

/* Fixed-timestep accumulator: the simulation always advances in 1/60 s ticks;
 * rendering interpolates with 'alpha'. */
#define CADV_TICK_HZ 60
#define CADV_TICK_DT (1.0 / (double)CADV_TICK_HZ)
#define CADV_MAX_TICKS_PER_FRAME 8

typedef struct cadv_clock {
    double accumulator;
    unsigned long long tick; /* total ticks simulated */
} cadv_clock;

void cadv_clock_init(cadv_clock *c);
/* Adds real elapsed seconds (scaled by 'speed') and returns how many ticks to
 * simulate now (capped to avoid a spiral of death). */
int cadv_clock_advance(cadv_clock *c, double elapsed_seconds, double speed);
/* Fraction of the next tick already elapsed, for render interpolation. */
float cadv_clock_alpha(const cadv_clock *c);

#endif

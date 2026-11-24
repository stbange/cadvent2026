/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_RPG_PERSUADE_H
#define CADV_KITS_RPG_PERSUADE_H

#include <stdbool.h>

/* RPG / tactics kit, persuasion: a beacon converts people near you over a
 * moment of exposure; how many may follow grows with the crowd you already
 * have; tougher minds (guards) need a bigger crowd first and take longer.
 * Also the hacking mini-game: a timing ring with a moving window. */

typedef struct cadv_charm {
    float range;        /* metres */
    int base_limit;     /* followers allowed with nobody yet */
    float growth;       /* extra allowance per follower */
    float time;         /* seconds of exposure for an ordinary mind */
} cadv_charm;

int cadv_charm_limit(const cadv_charm *c, int followers);
/* Minimum crowd before a mind of this resistance can be swayed at all
 * (0 = civilians, 1 = guards, ...). */
int cadv_charm_needs(int resist);
/* Exposes one person for dt; progress decays when out of range or not
 * allowed. True on the tick they convert. */
bool cadv_charm_tick(const cadv_charm *c, float *progress, float dist, int followers, int resist, float dt);

typedef struct cadv_hack {
    float angle;        /* the marker, 0..2pi */
    float speed;        /* radians per second */
    float window;       /* centre of the window */
    float width;        /* half width */
    int hits, need, misses;
    bool done;
    unsigned seed;
} cadv_hack;

void cadv_hack_start(cadv_hack *h, int need, float speed, float width, unsigned seed);
void cadv_hack_tick(cadv_hack *h, float dt);
/* Presses: +1 a hit (the window jumps on), -1 a miss, 0 when already done. */
int cadv_hack_press(cadv_hack *h);

#endif

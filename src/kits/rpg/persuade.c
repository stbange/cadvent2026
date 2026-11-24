/* SPDX-License-Identifier: MIT */
#include "kits/rpg/persuade.h"

#include <string.h>

#include "core/mathx.h"

int cadv_charm_limit(const cadv_charm *c, int followers) { return c->base_limit + (int)((float)followers * c->growth); }

int cadv_charm_needs(int resist) { return resist <= 0 ? 0 : resist * 4; }

bool cadv_charm_tick(const cadv_charm *c, float *progress, float dist, int followers, int resist, float dt)
{
    bool allowed = dist <= c->range && followers < cadv_charm_limit(c, followers) && followers >= cadv_charm_needs(resist);
    if (!allowed) {
        *progress = cadv_maxf(0.0f, *progress - dt);
        return false;
    }
    /* closer is quicker */
    *progress += dt * (1.5f - dist / c->range) / (float)(1 + resist);
    if (*progress >= c->time) {
        *progress = 0.0f;
        return true;
    }
    return false;
}

static float next_window(cadv_hack *h)
{
    h->seed = h->seed * 1103515245u + 12345u;
    return (float)((h->seed >> 8) % 1000u) / 1000.0f * 2.0f * CADV_PI;
}

void cadv_hack_start(cadv_hack *h, int need, float speed, float width, unsigned seed)
{
    memset(h, 0, sizeof *h);
    h->need = need;
    h->speed = speed;
    h->width = width;
    h->seed = seed | 1u;
    h->window = next_window(h);
}

void cadv_hack_tick(cadv_hack *h, float dt)
{
    if (h->done) {
        return;
    }
    h->angle += h->speed * dt;
    while (h->angle >= 2.0f * CADV_PI) {
        h->angle -= 2.0f * CADV_PI;
    }
}

int cadv_hack_press(cadv_hack *h)
{
    float d;
    if (h->done) {
        return 0;
    }
    d = cadv_absf(cadv_wrap_angle(h->angle - h->window));
    if (d <= h->width) {
        ++h->hits;
        h->done = h->hits >= h->need;
        h->window = cadv_wrap_angle(h->window + 1.5f + next_window(h) * 0.5f);
        if (h->window < 0.0f) {
            h->window += 2.0f * CADV_PI;
        }
        return 1;
    }
    ++h->misses;
    return -1;
}

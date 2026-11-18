/* SPDX-License-Identifier: MIT */
#include "hub/progress.h"

#include <stdio.h>
#include <string.h>

void cadv_progress_reset(cadv_progress *pr) { memset(pr, 0, sizeof *pr); }

static bool valid_day(int day) { return day >= 1 && day <= CADV_PORTAL_COUNT; }

bool cadv_progress_record(cadv_progress *pr, int day, const cadv_portal_result *r)
{
    cadv_portal_progress *p;
    bool changed = false;
    if (!valid_day(day)) {
        return false;
    }
    p = &pr->p[day - 1];
    p->entered = true;
    if (r->checkpoint > p->checkpoint) {
        p->checkpoint = r->checkpoint;
        changed = true;
    }
    if ((p->marks_found | r->marks_found) != p->marks_found) {
        p->marks_found |= r->marks_found;
        changed = true;
    }
    if (r->star_collected && !p->star_collected) {
        p->star_collected = true;
        changed = true;
    }
    if (r->outcome == CADV_OUTCOME_WON) {
        if (!p->completed) {
            p->completed = true;
            changed = true;
        }
        if (r->snowflakes > p->snowflakes) {
            p->snowflakes = r->snowflakes > 3 ? 3 : r->snowflakes;
            changed = true;
        }
        if (r->ticks > 0 && (p->best_ticks == 0 || r->ticks < p->best_ticks)) {
            p->best_ticks = r->ticks;
            changed = true;
        }
        p->checkpoint = 0; /* a finished run starts fresh next time */
    }
    return changed;
}

int cadv_progress_total_snowflakes(const cadv_progress *pr)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_PORTAL_COUNT; ++i) {
        n += pr->p[i].snowflakes;
    }
    return n;
}

int cadv_progress_candles(const cadv_progress *pr)
{
    int c = cadv_progress_total_snowflakes(pr) / 6;
    return c > 4 ? 4 : c;
}

int cadv_progress_stars_carried(const cadv_progress *pr)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_PORTAL_COUNT - 1; ++i) {
        n += pr->p[i].star_collected && !pr->p[i].star_socketed;
    }
    return n;
}

int cadv_progress_stars_socketed(const cadv_progress *pr)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_PORTAL_COUNT - 1; ++i) {
        n += pr->p[i].star_socketed;
    }
    return n;
}

bool cadv_progress_arch_complete(const cadv_progress *pr)
{
    return cadv_progress_stars_socketed(pr) == CADV_PORTAL_COUNT - 1;
}

int cadv_progress_socket_all(cadv_progress *pr)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_PORTAL_COUNT - 1; ++i) {
        if (pr->p[i].star_collected && !pr->p[i].star_socketed) {
            pr->p[i].star_socketed = true;
            ++n;
        }
    }
    return n;
}

int cadv_progress_missing_sockets(const cadv_progress *pr, int *out, int max)
{
    int i;
    int n = 0;
    for (i = 0; i < CADV_PORTAL_COUNT - 1; ++i) {
        if (!pr->p[i].star_socketed) {
            if (n < max) {
                out[n] = i + 1;
            }
            ++n;
        }
    }
    return n;
}

bool cadv_progress_can_crown(const cadv_progress *pr)
{
    return pr->p[CADV_PORTAL_COUNT - 1].star_collected && !pr->tree_crowned;
}

void cadv_progress_dev_fill(cadv_progress *pr)
{
    int i;
    for (i = 0; i < CADV_PORTAL_COUNT - 1; ++i) {
        pr->p[i].star_collected = true;
        pr->p[i].star_socketed = true;
    }
}

void cadv_progress_to_kv(const cadv_progress *pr, cadv_kvstore *kv)
{
    int i;
    char key[32];
    for (i = 0; i < CADV_PORTAL_COUNT; ++i) {
        const cadv_portal_progress *p = &pr->p[i];
        unsigned flags = (p->entered ? 1u : 0u) | (p->completed ? 2u : 0u) |
                         (p->star_collected ? 4u : 0u) | (p->star_socketed ? 8u : 0u);
        snprintf(key, sizeof key, "p%02d.flags", i + 1);
        cadv_kv_set_int(kv, key, (long long)flags);
        snprintf(key, sizeof key, "p%02d.snowflakes", i + 1);
        cadv_kv_set_int(kv, key, p->snowflakes);
        snprintf(key, sizeof key, "p%02d.checkpoint", i + 1);
        cadv_kv_set_int(kv, key, p->checkpoint);
        snprintf(key, sizeof key, "p%02d.best_ticks", i + 1);
        cadv_kv_set_int(kv, key, p->best_ticks);
        snprintf(key, sizeof key, "p%02d.marks", i + 1);
        cadv_kv_set_int(kv, key, (long long)p->marks_found);
    }
    cadv_kv_set_int(kv, "tree_crowned", pr->tree_crowned ? 1 : 0);
    cadv_kv_set_int(kv, "epilogue_seen", pr->epilogue_seen ? 1 : 0);
}

static int clampi(long long v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : (int)v); }

void cadv_progress_from_kv(cadv_progress *pr, const cadv_kvstore *kv)
{
    int i;
    char key[32];
    cadv_progress_reset(pr);
    for (i = 0; i < CADV_PORTAL_COUNT; ++i) {
        cadv_portal_progress *p = &pr->p[i];
        long long flags;
        snprintf(key, sizeof key, "p%02d.flags", i + 1);
        flags = cadv_kv_get_int(kv, key, 0);
        p->entered = (flags & 1) != 0;
        p->completed = (flags & 2) != 0;
        p->star_collected = (flags & 4) != 0;
        p->star_socketed = (flags & 8) != 0 && p->star_collected;
        snprintf(key, sizeof key, "p%02d.snowflakes", i + 1);
        p->snowflakes = clampi(cadv_kv_get_int(kv, key, 0), 0, 3);
        snprintf(key, sizeof key, "p%02d.checkpoint", i + 1);
        p->checkpoint = clampi(cadv_kv_get_int(kv, key, 0), 0, 1000);
        snprintf(key, sizeof key, "p%02d.best_ticks", i + 1);
        p->best_ticks = clampi(cadv_kv_get_int(kv, key, 0), 0, 0x7fffffff);
        snprintf(key, sizeof key, "p%02d.marks", i + 1);
        p->marks_found = (uint32_t)cadv_kv_get_int(kv, key, 0);
    }
    pr->tree_crowned = cadv_kv_get_int(kv, "tree_crowned", 0) != 0;
    pr->epilogue_seen = cadv_kv_get_int(kv, "epilogue_seen", 0) != 0;
}

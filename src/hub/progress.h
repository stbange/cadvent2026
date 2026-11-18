/* SPDX-License-Identifier: MIT */
#ifndef CADV_HUB_PROGRESS_H
#define CADV_HUB_PROGRESS_H

#include <stdbool.h>
#include <stdint.h>

#include "core/calendar.h"
#include "core/kvstore.h"
#include "portal/portal.h"

/* Everything the player has achieved; persisted in the save file. */

typedef struct cadv_portal_progress {
    bool entered;        /* the thawing reveal has been seen */
    bool completed;
    int snowflakes;      /* best rating 0..3 */
    bool star_collected; /* carried out of the portal */
    bool star_socketed;  /* set into the Star Arch */
    int checkpoint;
    int best_ticks;      /* 0 = none */
    uint32_t marks_found;
} cadv_portal_progress;

typedef struct cadv_progress {
    cadv_portal_progress p[CADV_PORTAL_COUNT];
    bool tree_crowned;   /* 24th star placed on the tree */
    bool epilogue_seen;
} cadv_progress;

void cadv_progress_reset(cadv_progress *pr);
/* Merges a finished session; returns true if the stored best changed. */
bool cadv_progress_record(cadv_progress *pr, int day, const cadv_portal_result *r);
int cadv_progress_total_snowflakes(const cadv_progress *pr);
int cadv_progress_candles(const cadv_progress *pr); /* one per 6 snowflakes, max 4 */
int cadv_progress_stars_carried(const cadv_progress *pr); /* collected, not yet socketed (1..23) */
int cadv_progress_stars_socketed(const cadv_progress *pr);
bool cadv_progress_arch_complete(const cadv_progress *pr);
/* Moves every carried star of days 1..23 into its socket; returns how many. */
int cadv_progress_socket_all(cadv_progress *pr);
/* Lists empty sockets (days) into out; returns the count. */
int cadv_progress_missing_sockets(const cadv_progress *pr, int *out, int max);
/* Day 24 star collected and the tree not yet crowned. */
bool cadv_progress_can_crown(const cadv_progress *pr);
/* Fills all 23 sockets (development override). */
void cadv_progress_dev_fill(cadv_progress *pr);

void cadv_progress_to_kv(const cadv_progress *pr, cadv_kvstore *kv);
void cadv_progress_from_kv(cadv_progress *pr, const cadv_kvstore *kv);

#define CADV_SAVE_MAGIC "cadvent-save"

#endif

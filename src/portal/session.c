/* SPDX-License-Identifier: MIT */
#include "portal/portal.h"

#include <string.h>

bool cadv_session_start(cadv_session *s, const cadv_portal_desc *d, const cadv_portal_cfg *cfg)
{
    memset(s, 0, sizeof *s);
    if (!d || !cadv_arena_init(&s->arena, d->state_size + 64u)) { /* GCOVR_EXCL_BR_LINE */
        return false;
    }
    s->state = cadv_arena_alloc(&s->arena, d->state_size, 16);
    s->desc = d;
    if (!s->state || !d->init(s->state, cfg)) {
        cadv_arena_free(&s->arena);
        memset(s, 0, sizeof *s);
        return false;
    }
    return true;
}

void cadv_session_tick(cadv_session *s, const cadv_input_frame *in)
{
    if (s->desc) {
        s->desc->tick(s->state, in);
    }
}

cadv_portal_result cadv_session_result(const cadv_session *s)
{
    cadv_portal_result r;
    if (!s->desc) {
        memset(&r, 0, sizeof r);
        r.outcome = CADV_OUTCOME_QUIT;
        return r;
    }
    return s->desc->result(s->state);
}

void cadv_session_end(cadv_session *s)
{
    if (!s->desc) {
        return;
    }
    if (s->gpu_free) {
        s->gpu_free(s->state);
    }
    s->arena_peak = s->arena.peak;
    cadv_arena_free(&s->arena);
    s->desc = NULL;
    s->state = NULL;
    s->gpu_free = NULL;
}

bool cadv_session_active(const cadv_session *s) { return s->desc != NULL; }

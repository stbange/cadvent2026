/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P07_TINSEL_STRIKE_H
#define CADV_PORTALS_P07_TINSEL_STRIKE_H

#include "portal/portal.h"

/* Day 7 - Tinsel Strike. Round-based 5v5 in the snowy market town of
 * Glühstadt: the Humbug Syndicate tries to set off a glitter bomb at one of
 * two market sites, the Nutcracker Guard defends. Non-lethal tagging: the
 * defeated pop into confetti. Original setting, teams and weapons. */

#define CADV_P07_STATE_SIZE (512 * 1024)

bool cadv_p07_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p07_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p07_result(const void *state);
bool cadv_p07_query(const void *state, const char *key, double *out);
void cadv_p07_bot(const void *state, cadv_input_frame *out);
void cadv_p07_teleport(void *state, float x, float y, float z, float yaw);

#endif

/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P02_AURORA_CARPET_H
#define CADV_PORTALS_P02_AURORA_CARPET_H

#include "portal/portal.h"

/* Day 2 - Aurora Carpet. An apprentice skyweaver flies her grandmother's
 * carpet over the Shattered Fjords, reshapes the land with spells and wins the
 * starlight mana back from the Frost Sorcerer Vael. Original setting and cast. */

#define CADV_P02_STATE_SIZE (640 * 1024)
#define CADV_P02_WORLD_MANA 1000
#define CADV_P02_WIN_SHARE 0.75f

bool cadv_p02_init(void *state, const cadv_portal_cfg *cfg);
void cadv_p02_tick(void *state, const cadv_input_frame *in);
cadv_portal_result cadv_p02_result(const void *state);
bool cadv_p02_query(const void *state, const char *key, double *out);
void cadv_p02_bot(const void *state, cadv_input_frame *out);
void cadv_p02_teleport(void *state, float x, float y, float z, float yaw);

#endif

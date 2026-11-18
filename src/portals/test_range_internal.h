/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_TEST_RANGE_INTERNAL_H
#define CADV_PORTALS_TEST_RANGE_INTERNAL_H

#include "gfx/gl_util.h"
#include "gfx/mesh.h"
#include "kits/fps/combat.h"
#include "portal/portal.h"

#define DT (1.0f / 60.0f)
#define MAX_PLAQUES 4
#define KEY_BIT 1u

/* Layout (metres): main room x -8..8, z -8..8; north wall z=-8 has a door
 * (x -1..1) to the star room z -14..-8. Spawn at the south end. */
typedef struct tr_gpu {
    cadv_gpu_mesh room, floor, door, star, key, pedestal, plaque;
    unsigned tex_wall, tex_floor, tex_door, tex_imp, tex_imp_n;
    unsigned tex_plaque[MAX_PLAQUES];
    int imp_fw, imp_fh, imp_dirs;
} tr_gpu;

typedef struct tr_state {
    cadv_fps fps;
    cadv_portal_cfg cfg;
    int day;
    int ticks;
    int checkpoint;
    int win_timer;       /* ticks of victory banner left */
    int death_timer;
    cadv_outcome outcome;
    uint32_t marks_found;
    int plaque_count;
    int plaque_mark[MAX_PLAQUES];
    cadv_vec3 plaque_pos[MAX_PLAQUES];
    float plaque_yaw[MAX_PLAQUES];
    char plaque_text[MAX_PLAQUES][64];
    char message[64];
    int message_timer;
    int star_pickup;
    bool gpu_ready;
    tr_gpu g;
} tr_state;

#endif

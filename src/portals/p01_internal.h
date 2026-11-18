/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTALS_P01_INTERNAL_H
#define CADV_PORTALS_P01_INTERNAL_H

#include "gfx/gl_util.h"
#include "kits/fps/combat.h"
#include "kits/fps/map.h"
#include "portals/p01_sleighed.h"

#define P01_DT (1.0f / 60.0f)
#define P01_KEYS_ALL 7u
#define P01_MAX_MATS 32

enum { P01_WEAPON_CANE, P01_WEAPON_PISTOL, P01_WEAPON_SCATTER, P01_WEAPON_GIFTBOMB, P01_WEAPON_SHRINK, P01_WEAPON_CHAINGUN, P01_WEAPONS };
enum { P01_HOG, P01_BOAR, P01_DRONE, P01_GREMLIN, P01_LOBBER, P01_BOSS, P01_ENEMY_TYPES };

typedef struct p01_gpu {
    cadv_gpu_mesh mat_mesh[P01_MAX_MATS];
    unsigned mat_tex[P01_MAX_MATS];
    char mat_name[P01_MAX_MATS][CADV_MAP_NAME];
    int mat_count;
    cadv_gpu_mesh glass[CADV_FPS_MAX_BREAKABLES];
    cadv_gpu_mesh door[CADV_FPS_MAX_DOORS];
    cadv_gpu_mesh cube, prop, plaque, star, key, beam;
    unsigned tex_sprites[P01_ENEMY_TYPES], tex_normals[P01_ENEMY_TYPES];
    int fw[P01_ENEMY_TYPES], fh[P01_ENEMY_TYPES];
    unsigned tex_sign[32], tex_mark[16];
    int sign_w[32], mark_w[16];
    unsigned tex_glass, tex_target;
} p01_gpu;

typedef struct p01_state {
    cadv_fps fps;
    cadv_map map;
    cadv_map_links links;
    cadv_portal_cfg cfg;
    int day;
    int ticks;
    int checkpoint;
    cadv_outcome outcome;
    uint32_t marks_found;
    uint32_t secrets_found;     /* bit 0..4 = map secret volumes, bit 5 = arcade */
    uint32_t zones_seen;
    int prop_uses[CADV_MAP_MAX_ITEMS];
    int boss_index;
    bool boss_awake;
    int siphon_base_box;        /* map box index of the siphon base */
    bool siphon_down;
    int collapse_ticks;
    int star_pickup;
    int win_timer;
    int death_timer;
    int route;                  /* bot progress through map waypoints */
    int kill_streak;
    bool low_health_quipped;
    int shake;                  /* ticks of screen shake left */
    char message[72];
    int message_timer;
    char quip[72];
    int quip_timer;
    char mark_text[16][64];
    bool gpu_ready;
    p01_gpu g;
} p01_state;

const cadv_weapon_def *cadv_p01_weapons(void);
const cadv_enemy_def *cadv_p01_enemies(void);

#endif

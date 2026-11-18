/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_FPS_PLAYER_H
#define CADV_KITS_FPS_PLAYER_H

#include <stdbool.h>
#include <stdint.h>

#include "input/input.h"
#include "sim/collide.h"

/* First-person body shared by the hall and the FPS-style portals. */

#define CADV_PLAYER_RADIUS 0.3f
#define CADV_PLAYER_HEIGHT 1.7f
#define CADV_PLAYER_CROUCH_HEIGHT 1.0f
#define CADV_PLAYER_EYE 1.55f
#define CADV_PLAYER_STEP 0.45f

typedef struct cadv_player_tuning {
    float walk_speed, run_speed, crouch_speed; /* m/s */
    float accel, air_accel, friction;          /* 1/s */
    float jump_speed, gravity;                 /* m/s, m/s^2 */
    float max_pitch;                           /* radians */
} cadv_player_tuning;

typedef enum cadv_player_event {
    CADV_PEV_JUMP = 1 << 0,
    CADV_PEV_LAND = 1 << 1,
    CADV_PEV_STEP = 1 << 2 /* footstep sound cue */
} cadv_player_event;

typedef struct cadv_player {
    cadv_vec3 pos;  /* feet */
    cadv_vec3 vel;
    float yaw, pitch;
    bool on_ground;
    bool crouching;
    float stride;   /* distance walked since last footstep */
    int health, armor;
    uint32_t keys;  /* key/card bitmask */
    cadv_player_tuning tune;
} cadv_player;

cadv_player_tuning cadv_player_default_tuning(void);
void cadv_player_spawn(cadv_player *p, cadv_vec3 feet, float yaw);
/* Advances one tick; returns cadv_player_event flags. */
uint32_t cadv_player_tick(cadv_player *p, const cadv_world *w, const cadv_input_frame *in, float dt);
cadv_vec3 cadv_player_eye(const cadv_player *p);
cadv_vec3 cadv_player_forward(const cadv_player *p);
float cadv_player_height(const cadv_player *p);
/* Applies damage (armor absorbs half); returns true if the player died. */
bool cadv_player_damage(cadv_player *p, int amount);

#endif

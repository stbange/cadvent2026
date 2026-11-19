/* SPDX-License-Identifier: MIT */
#ifndef CADV_KITS_PLATFORMER_CLIMBER_H
#define CADV_KITS_PLATFORMER_CLIMBER_H

#include <stdint.h>

#include "input/input.h"
#include "kits/platformer/tiles.h"

/* Grid-aware first-person traversal in the spirit of classic tomb explorers:
 *  - a standing jump (walking pace) clears a 1-tile gap, a running jump (RUN)
 *    a 2-tile gap, nothing clears 3; jumps snap to the grid axis when facing
 *    within 25 degrees of it;
 *  - holding USE in the air catches ledges whose top passes the hands within
 *    a tolerance window; walls up to 2 tiles are climbed by jumping and
 *    grabbing; hanging, the player shimmies sideways and pulls up with JUMP
 *    or forward; JUMP while holding USE in front of a 1-tile step climbs it;
 *  - USE + forward/back pushes or pulls a block one tile;
 *  - in water the player swims along the view, JUMP rises, CROUCH dives,
 *    breath runs out while the head is under water.
 * Support is tested under the centre of the feet, so jump distances are
 * exact in tiles. */

typedef enum cadv_climb_state {
    CADV_CLIMB_GROUND = 0,
    CADV_CLIMB_AIR,
    CADV_CLIMB_HANG,
    CADV_CLIMB_PULLUP,
    CADV_CLIMB_PUSH,
    CADV_CLIMB_SWIM
} cadv_climb_state;

enum {
    CADV_CLIMB_EV_JUMP = 1 << 0,
    CADV_CLIMB_EV_LAND = 1 << 1,     /* last_fall holds the drop in metres */
    CADV_CLIMB_EV_GRAB = 1 << 2,
    CADV_CLIMB_EV_UP = 1 << 3,       /* pull-up / step climb finished */
    CADV_CLIMB_EV_PUSHED = 1 << 4,   /* a block moved */
    CADV_CLIMB_EV_SPLASH = 1 << 5,
    CADV_CLIMB_EV_DROWNING = 1 << 6, /* no breath left this tick */
    CADV_CLIMB_EV_ABYSS = 1 << 7     /* fell out of the world */
};

typedef struct cadv_climber_def {
    float radius, height, eye;
    float walk, run;            /* m/s */
    float standing_jump;        /* horizontal m/s of a jump at walking pace */
    float running_jump;
    float jump_vy;
    float gravity;
    float swim;
    float air_max;              /* seconds of breath */
} cadv_climber_def;

typedef struct cadv_climber {
    cadv_vec3 pos;              /* centre of the feet */
    cadv_vec3 vel;
    float yaw, pitch;
    cadv_climb_state state;
    float timer, duration;
    cadv_vec3 from, to;
    int dirx, dirz;             /* grid direction while hanging / pushing */
    int ledge_y;
    float peak_y;
    float last_fall;
    float air;
    int block_x, block_y, block_z; /* block moved at the end of a pull */
    int block_tx, block_tz;
    bool pulling;
} cadv_climber;

void cadv_climber_default_def(cadv_climber_def *d);
void cadv_climber_spawn(cadv_climber *c, const cadv_climber_def *d, cadv_vec3 pos, float yaw);
uint32_t cadv_climber_tick(cadv_climber *c, const cadv_climber_def *d, const cadv_input_frame *in, cadv_tiles *t, float dt);
cadv_vec3 cadv_climber_eye(const cadv_climber *c, const cadv_climber_def *d);
cadv_vec3 cadv_climber_forward(const cadv_climber *c);
/* Grid direction the player faces (one of dx, dz is +-1, the other 0). */
void cadv_climber_facing(const cadv_climber *c, int *dx, int *dz);

#endif

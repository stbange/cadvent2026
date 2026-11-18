/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTAL_SCRIPT_H
#define CADV_PORTAL_SCRIPT_H

#include <stdbool.h>

#include "input/input.h"

/* Scenario scripts (.gts) drive the real game loop headlessly in tests:
 *
 *   # comment
 *   wait 30                      hold still for 30 ticks
 *   hold use 10                  hold an action for N ticks
 *   press jump                   one-tick press
 *   move 0 1 60                  move_x move_y for N ticks
 *   turn 0.5 0                   look delta (radians) for one tick
 *   face 3.14                    turn to an absolute yaw (needs player.yaw)
 *   teleport 0 0 5 3.14          x y z yaw
 *   walk_to 2.0 -4.0 600         steer to x z within N ticks (fails on timeout)
 *   bot 7200                     let the current mode's bot play until it ends
 *                                (or until the mode changes, e.g. back to the hall)
 *   bot_for 600                  let the bot play 600 ticks (no need to finish)
 *   expect hub.stars_socketed == 23
 *   wait_until mode == 1 600     wait for a condition up to N ticks
 *   expect_event portal_won
 *   screenshot out.ppm
 *   log any text
 *   print player.x               log a value (debugging)
 */

#define CADV_SCRIPT_MAX_CMDS 512

typedef enum cadv_script_op {
    CADV_SOP_WAIT, CADV_SOP_HOLD, CADV_SOP_PRESS, CADV_SOP_MOVE, CADV_SOP_TURN, CADV_SOP_FACE,
    CADV_SOP_TELEPORT, CADV_SOP_WALK_TO, CADV_SOP_BOT, CADV_SOP_EXPECT, CADV_SOP_WAIT_UNTIL,
    CADV_SOP_EXPECT_EVENT, CADV_SOP_SCREENSHOT, CADV_SOP_PRINT, CADV_SOP_LOG
} cadv_script_op;

typedef enum cadv_cmp { CADV_CMP_EQ, CADV_CMP_NE, CADV_CMP_LT, CADV_CMP_LE, CADV_CMP_GT, CADV_CMP_GE } cadv_cmp;

typedef struct cadv_script_cmd {
    cadv_script_op op;
    int line;
    int ticks;
    int action;
    float a, b, c, d;
    cadv_cmp cmp;
    char text[256]; /* key / event / path / log text */
} cadv_script_cmd;

typedef struct cadv_script {
    cadv_script_cmd cmd[CADV_SCRIPT_MAX_CMDS];
    int count;
    int pc;        /* current command */
    int elapsed;   /* ticks spent in the current command */
    bool failed;
    bool done;
    char error[160];
} cadv_script;

typedef struct cadv_script_host {
    void *user;
    bool (*query)(void *user, const char *key, double *out);
    void (*teleport)(void *user, float x, float y, float z, float yaw);
    void (*bot)(void *user, cadv_input_frame *out);
    bool (*event)(void *user, const char *name);
    bool (*screenshot)(void *user, const char *path);
    void (*log)(void *user, const char *text);
} cadv_script_host;

/* Parses script text; on error returns false with s->error set. */
bool cadv_script_parse(cadv_script *s, const char *text);
bool cadv_script_load(cadv_script *s, const char *path);
/* Produces the input for one tick. Returns false once the script has finished
 * (check s->failed). */
bool cadv_script_step(cadv_script *s, const cadv_script_host *host, cadv_input_frame *out);
int cadv_script_action_from_name(const char *name); /* -1 if unknown */
bool cadv_script_compare(double lhs, cadv_cmp cmp, double rhs);

#endif

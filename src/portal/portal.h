/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTAL_PORTAL_H
#define CADV_PORTAL_PORTAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "core/arena.h"
#include "core/calendar.h"
#include "gfx/scene.h"
#include "gfx/view.h"
#include "input/input.h"

/* The contract every portal game (and the hall itself) implements. */

struct cadv_audio;

enum {
    CADV_CAP_SOLO = 1 << 0,
    CADV_CAP_COOP = 1 << 1,
    CADV_CAP_VERSUS = 1 << 2,
    CADV_CAP_XR_FULL = 1 << 3,
    CADV_CAP_XR_SEATED = 1 << 4,
    CADV_CAP_XR_TABLETOP = 1 << 5
};

typedef enum cadv_difficulty {
    CADV_DIFF_STORY = 0,
    CADV_DIFF_DEFAULT = 1,
    CADV_DIFF_HARD = 2
} cadv_difficulty;

typedef enum cadv_outcome {
    CADV_OUTCOME_RUNNING = 0,
    CADV_OUTCOME_WON = 1,
    CADV_OUTCOME_LOST = 2,
    CADV_OUTCOME_QUIT = 3
} cadv_outcome;

#define CADV_MAX_MARKS 16

typedef struct cadv_portal_result {
    cadv_outcome outcome;
    int snowflakes;     /* 0..3 */
    bool star_collected;
    int checkpoint;     /* highest checkpoint reached (0 = start) */
    uint32_t marks_found; /* bit i = this portal's mark i was registered */
    int ticks;          /* simulated play time */
} cadv_portal_result;

typedef struct cadv_portal_cfg {
    int day;
    uint64_t seed;
    cadv_difficulty difficulty;
    int checkpoint;     /* resume from this checkpoint */
    struct cadv_audio *audio; /* may be NULL (headless) */
    const char *data_dir;       /* where assets/ lives (with trailing slash), may be "" */
    const char *const *friends; /* friend display names */
    int friend_count;
} cadv_portal_cfg;

/* A friend's mark hidden in a portal: graffiti, a sign, a name tag ...
 * 'text' is a template where %s is replaced by the friend's display name. */
typedef struct cadv_mark_def {
    int friend_index;   /* index into the friends list */
    const char *text;   /* e.g. "%s WAS HERE" */
} cadv_mark_def;

typedef struct cadv_portal_desc {
    int day;                 /* 1..24 */
    const char *id;          /* "p01_sleighed" */
    const char *title;
    const char *inspired_by; /* nominative reference for docs/notice board */
    uint32_t caps;
    bool implemented;        /* false = "still thawing", only enterable with --unlock-all */
    size_t state_size;
    bool (*init)(void *state, const cadv_portal_cfg *cfg);
    void (*tick)(void *state, const cadv_input_frame *in);
    cadv_portal_result (*result)(const void *state);
    /* Numeric state for scenario scripts; returns false for unknown keys. */
    bool (*query)(const void *state, const char *key, double *out);
    /* Automated player for tests and pace estimates. */
    void (*bot)(const void *state, cadv_input_frame *out);
    /* Teleport for scenario scripts. */
    void (*teleport)(void *state, float x, float y, float z, float yaw);
    /* Friend marks hidden in this portal (static data). */
    const cadv_mark_def *marks;
    int mark_count;
} cadv_portal_desc;

/* The 24 portals in calendar order (index 0 = day 1). */
const cadv_portal_desc *cadv_portal_get(int day);

/* GPU side of a portal, kept apart so the simulation never links against GL.
 * Implemented by the graphics library (src/portals/views.c). */
typedef struct cadv_portal_view {
    void (*gpu_init)(void *state);
    void (*gpu_free)(void *state);
    void (*draw)(void *state, cadv_scene *scene, cadv_camera *cam, cadv_ui *ui, float alpha);
} cadv_portal_view;

const cadv_portal_view *cadv_portal_view_get(int day);

/* A running portal: owns the arena that holds all of its state. */
typedef struct cadv_session {
    const cadv_portal_desc *desc;
    cadv_arena arena;
    void *state;
    void (*gpu_free)(void *state); /* set once GPU resources exist */
    size_t arena_peak;
} cadv_session;

bool cadv_session_start(cadv_session *s, const cadv_portal_desc *d, const cadv_portal_cfg *cfg);
void cadv_session_tick(cadv_session *s, const cadv_input_frame *in);
cadv_portal_result cadv_session_result(const cadv_session *s);
void cadv_session_end(cadv_session *s);
bool cadv_session_active(const cadv_session *s);

#endif

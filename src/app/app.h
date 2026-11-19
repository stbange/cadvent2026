/* SPDX-License-Identifier: MIT */
#ifndef CADV_APP_APP_H
#define CADV_APP_APP_H

#include <stdbool.h>

#include "audio/audio.h"
#include "core/args.h"
#include "core/calendar.h"
#include "core/clock.h"
#include "gfx/renderer.h"
#include "gfx/scene.h"
#include "hub/friends.h"
#include "hub/hub.h"
#include "hub/hub_gfx.h"
#include "hub/progress.h"
#include "input/input.h"
#include "portal/portal.h"
#include "portal/script.h"

/* Platform-independent application: the hall, portal sessions, saving,
 * scenario scripts and rendering. The SDL layer only forwards window size,
 * time, raw input and audio pulls. */

typedef enum cadv_mode { CADV_MODE_HUB = 0, CADV_MODE_PORTAL = 1 } cadv_mode;

typedef enum cadv_app_status {
    CADV_APP_CONTINUE = 0,
    CADV_APP_DONE_OK = 1,
    CADV_APP_DONE_FAIL = 2
} cadv_app_status;

#define CADV_APP_MAX_EVENTS 128

typedef struct cadv_app {
    cadv_args args;
    bool gpu;                  /* rendering available (false = headless) */
    cadv_gl_profile profile;
    cadv_renderer renderer;
    cadv_hub_gfx hub_gfx;
    cadv_scene scene;
    cadv_ui ui;
    cadv_camera cam;
    cadv_input input;
    cadv_audio audio;
    cadv_clock clock;
    cadv_progress progress;
    cadv_friends friends;
    cadv_hub hub;
    cadv_session session;
    cadv_mode mode;
    int portal_day;
    cadv_script script;
    bool has_script;
    char save_path[512];
    char data_dir[512];
    char events[CADV_APP_MAX_EVENTS][40];
    int event_count;
    int frame;
    unsigned long long ticks;
    cadv_datetime now;
    int width, height;         /* last window size */
    bool screenshot_pending;
    char screenshot_path[256];
    bool screenshot_ok;
    bool touch_visible;        /* last reported touch-overlay state */
    bool help_open;            /* on-screen help shown (simulation paused) */
    int help_anim;             /* 0..CADV_APP_HELP_ANIM opening slide */
    int help_hint;             /* ticks left of the "F1 HELP" reminder */
    int help_page;             /* 0 or 1 (small screens get two pages) */
} cadv_app;

#define CADV_APP_HELP_ANIM 10
#define CADV_APP_HELP_HINT 420

/* data_dir: where assets/ live (with trailing slash, may be ""); save_dir: a
 * writable directory for the save file (with trailing slash, may be ""). */
bool cadv_app_init(cadv_app *app, const cadv_args *args, bool gpu, cadv_gl_profile profile,
                   const char *data_dir, const char *save_dir, cadv_datetime now);
/* Wall-clock local time (ignored when --date was given). */
void cadv_app_set_now(cadv_app *app, cadv_datetime now);
/* One simulation tick (scripts, hall, portal). */
void cadv_app_tick(cadv_app *app);
/* Advances real time 'elapsed' seconds, renders a frame into the window. */
cadv_app_status cadv_app_frame(cadv_app *app, double elapsed, int width, int height);
/* Headless driver: runs ticks until the script ends or limits are hit. */
cadv_app_status cadv_app_run_headless(cadv_app *app);
/* Status of scripted runs after a tick (CONTINUE while running). */
cadv_app_status cadv_app_script_status(const cadv_app *app);
bool cadv_app_query(cadv_app *app, const char *key, double *out);
bool cadv_app_event_happened(const cadv_app *app, const char *name);
void cadv_app_save(cadv_app *app);
void cadv_app_mix_audio(cadv_app *app, float *out, int frames);
void cadv_app_shutdown(cadv_app *app);

#endif

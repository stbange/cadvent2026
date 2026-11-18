/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_ARGS_H
#define CADV_CORE_ARGS_H

#include <stdbool.h>

#include "core/calendar.h"
#include "core/log.h"

typedef enum cadv_args_status {
    CADV_ARGS_RUN = 0,     /* start the game */
    CADV_ARGS_EXIT_OK = 1, /* --help / --version handled, exit 0 */
    CADV_ARGS_ERROR = 2    /* invalid arguments, exit non-zero */
} cadv_args_status;

typedef struct cadv_args {
    int width;              /* window width in pixels */
    int height;             /* window height in pixels */
    int internal_height;    /* low-res render height (width follows aspect) */
    int frames;             /* quit after this many frames, 0 = run until closed */
    const char *screenshot; /* write a PPM of the last frame, NULL = off */
    const char *script;     /* scenario script (.gts) to run, NULL = off */
    const char *save_path;  /* override the save file location */
    int portal;             /* 0 = start in the hall, 1..24 = start in that portal */
    int max_ticks;          /* headless safety limit, 0 = none */
    unsigned seed;          /* world seed */
    bool headless;          /* simulate without a window or GPU */
    bool unlock_all;        /* development override: all portals + all sockets */
    bool has_date;          /* use 'date' instead of the system clock */
    cadv_datetime date;
    cadv_log_level log_level;
    bool show_help;
    bool show_version;
    char error[128];
} cadv_args;

void cadv_args_defaults(cadv_args *a);
/* argv[0] is skipped. On error, a->error holds a human-readable message. */
cadv_args_status cadv_args_parse(cadv_args *a, int argc, char **argv);
/* Applies web URL query parameters (e.g. "?dev=unlockall&date=2026-12-05"). */
void cadv_args_apply_query(cadv_args *a, const char *query);
const char *cadv_args_usage(void);

#endif

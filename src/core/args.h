/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_ARGS_H
#define CADV_CORE_ARGS_H

#include <stdbool.h>

#include "core/log.h"

typedef enum cadv_args_status {
    CADV_ARGS_RUN = 0,     /* start the game */
    CADV_ARGS_EXIT_OK = 1, /* --help / --version handled, exit 0 */
    CADV_ARGS_ERROR = 2    /* invalid arguments, exit non-zero */
} cadv_args_status;

typedef struct cadv_args {
    int width;              /* window width in pixels */
    int height;             /* window height in pixels */
    int frames;             /* quit after this many frames, 0 = run until closed */
    const char *screenshot; /* write a PPM of the last frame, NULL = off */
    cadv_log_level log_level;
    bool show_help;
    bool show_version;
    char error[128];
} cadv_args;

void cadv_args_defaults(cadv_args *a);
/* argv[0] is skipped. On error, a->error holds a human-readable message. */
cadv_args_status cadv_args_parse(cadv_args *a, int argc, char **argv);
const char *cadv_args_usage(void);

#endif

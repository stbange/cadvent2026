/* SPDX-License-Identifier: MIT */
#include "core/args.h"

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cadv_args_defaults(cadv_args *a)
{
    memset(a, 0, sizeof *a);
    a->width = 1280;
    a->height = 720;
    a->log_level = CADV_LOG_INFO;
}

static bool parse_int(const char *s, int lo, int hi, int *out)
{
    char *end = NULL;
    long v;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v < lo || v > hi) {
        return false;
    }
    *out = (int)v;
    return true;
}

static bool parse_level(const char *s, cadv_log_level *out)
{
    static const char *names[] = {"debug", "info", "warn", "error", "off"};
    int i;
    for (i = 0; i < 5; ++i) {
        if (strcmp(s, names[i]) == 0) {
            *out = (cadv_log_level)i;
            return true;
        }
    }
    return false;
}

static cadv_args_status fail(cadv_args *a, const char *what, const char *opt)
{
    snprintf(a->error, sizeof a->error, "%s: %s", what, opt);
    return CADV_ARGS_ERROR;
}

cadv_args_status cadv_args_parse(cadv_args *a, int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; ++i) {
        const char *opt = argv[i];
        const char *val = (i + 1 < argc) ? argv[i + 1] : NULL;
        if (strcmp(opt, "--help") == 0 || strcmp(opt, "-h") == 0) {
            a->show_help = true;
            continue;
        }
        if (strcmp(opt, "--version") == 0) {
            a->show_version = true;
            continue;
        }
        if (strcmp(opt, "--width") == 0 || strcmp(opt, "--height") == 0 ||
            strcmp(opt, "--frames") == 0 || strcmp(opt, "--screenshot") == 0 ||
            strcmp(opt, "--log-level") == 0) {
            if (!val) {
                return fail(a, "missing value for option", opt);
            }
            ++i;
            if (strcmp(opt, "--width") == 0) {
                if (!parse_int(val, 64, 16384, &a->width)) {
                    return fail(a, "invalid width", val);
                }
            } else if (strcmp(opt, "--height") == 0) {
                if (!parse_int(val, 64, 16384, &a->height)) {
                    return fail(a, "invalid height", val);
                }
            } else if (strcmp(opt, "--frames") == 0) {
                if (!parse_int(val, 0, INT_MAX, &a->frames)) {
                    return fail(a, "invalid frame count", val);
                }
            } else if (strcmp(opt, "--screenshot") == 0) {
                a->screenshot = val;
            } else if (!parse_level(val, &a->log_level)) {
                return fail(a, "invalid log level", val);
            }
            continue;
        }
        return fail(a, "unknown option", opt);
    }
    if (a->show_help || a->show_version) {
        return CADV_ARGS_EXIT_OK;
    }
    return CADV_ARGS_RUN;
}

const char *cadv_args_usage(void)
{
    return "usage: cadvent [options]\n"
           "  --width N          window width (64..16384, default 1280)\n"
           "  --height N         window height (64..16384, default 720)\n"
           "  --frames N         quit after N frames (0 = run until closed)\n"
           "  --screenshot FILE  write the last frame as binary PPM\n"
           "  --log-level LEVEL  debug|info|warn|error|off (default info)\n"
           "  --version          print version and exit\n"
           "  -h, --help         print this help and exit\n";
}

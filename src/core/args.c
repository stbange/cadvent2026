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
    a->internal_height = 225;
    a->seed = 2026u;
    a->log_level = CADV_LOG_INFO;
}

static bool parse_int(const char *s, long lo, long hi, long *out)
{
    char *end = NULL;
    long v;
    errno = 0;
    v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0' || v < lo || v > hi) {
        return false;
    }
    *out = v;
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

typedef enum opt_kind { OPT_FLAG, OPT_INT, OPT_STR, OPT_LEVEL, OPT_DATE } opt_kind;

typedef struct opt_def {
    const char *name;
    opt_kind kind;
    long lo, hi;
    const char *what; /* error text prefix */
} opt_def;

static const opt_def k_opts[] = {
    {"--width", OPT_INT, 64, 16384, "invalid width"},
    {"--height", OPT_INT, 64, 16384, "invalid height"},
    {"--internal-height", OPT_INT, 90, 720, "invalid internal height"},
    {"--frames", OPT_INT, 0, INT_MAX, "invalid frame count"},
    {"--portal", OPT_INT, 0, CADV_PORTAL_COUNT, "invalid portal"},
    {"--max-ticks", OPT_INT, 0, INT_MAX, "invalid tick limit"},
    {"--seed", OPT_INT, 0, INT_MAX, "invalid seed"},
    {"--screenshot", OPT_STR, 0, 0, NULL},
    {"--script", OPT_STR, 0, 0, NULL},
    {"--save", OPT_STR, 0, 0, NULL},
    {"--log-level", OPT_LEVEL, 0, 0, "invalid log level"},
    {"--date", OPT_DATE, 0, 0, "invalid date"},
    {"--headless", OPT_FLAG, 0, 0, NULL},
    {"--unlock-all", OPT_FLAG, 0, 0, NULL},
    {"--help", OPT_FLAG, 0, 0, NULL},
    {"-h", OPT_FLAG, 0, 0, NULL},
    {"--version", OPT_FLAG, 0, 0, NULL},
};

static cadv_args_status fail(cadv_args *a, const char *what, const char *opt)
{
    snprintf(a->error, sizeof a->error, "%s: %s", what, opt);
    return CADV_ARGS_ERROR;
}

static void store_int(cadv_args *a, const char *name, long v)
{
    if (strcmp(name, "--width") == 0) {
        a->width = (int)v;
    } else if (strcmp(name, "--height") == 0) {
        a->height = (int)v;
    } else if (strcmp(name, "--internal-height") == 0) {
        a->internal_height = (int)v;
    } else if (strcmp(name, "--frames") == 0) {
        a->frames = (int)v;
    } else if (strcmp(name, "--portal") == 0) {
        a->portal = (int)v;
    } else if (strcmp(name, "--max-ticks") == 0) {
        a->max_ticks = (int)v;
    } else {
        a->seed = (unsigned)v;
    }
}

static void store_str(cadv_args *a, const char *name, const char *v)
{
    if (strcmp(name, "--screenshot") == 0) {
        a->screenshot = v;
    } else if (strcmp(name, "--script") == 0) {
        a->script = v;
    } else {
        a->save_path = v;
    }
}

static void store_flag(cadv_args *a, const char *name)
{
    if (strcmp(name, "--headless") == 0) {
        a->headless = true;
    } else if (strcmp(name, "--unlock-all") == 0) {
        a->unlock_all = true;
    } else if (strcmp(name, "--version") == 0) {
        a->show_version = true;
    } else {
        a->show_help = true;
    }
}

cadv_args_status cadv_args_parse(cadv_args *a, int argc, char **argv)
{
    int i;
    for (i = 1; i < argc; ++i) {
        const char *opt = argv[i];
        const opt_def *d = NULL;
        size_t k;
        const char *val;
        long iv = 0;
        for (k = 0; k < sizeof k_opts / sizeof k_opts[0]; ++k) {
            if (strcmp(opt, k_opts[k].name) == 0) {
                d = &k_opts[k];
                break;
            }
        }
        if (!d) {
            return fail(a, "unknown option", opt);
        }
        if (d->kind == OPT_FLAG) {
            store_flag(a, d->name);
            continue;
        }
        if (i + 1 >= argc) {
            return fail(a, "missing value for option", opt);
        }
        val = argv[++i];
        switch (d->kind) {
        case OPT_INT:
            if (!parse_int(val, d->lo, d->hi, &iv)) {
                return fail(a, d->what, val);
            }
            store_int(a, d->name, iv);
            break;
        case OPT_LEVEL:
            if (!parse_level(val, &a->log_level)) {
                return fail(a, d->what, val);
            }
            break;
        case OPT_DATE:
            if (!cadv_datetime_parse(val, &a->date)) {
                return fail(a, d->what, val);
            }
            a->has_date = true;
            break;
        default:
            store_str(a, d->name, val);
            break;
        }
    }
    if (a->show_help || a->show_version) {
        return CADV_ARGS_EXIT_OK;
    }
    return CADV_ARGS_RUN;
}

void cadv_args_apply_query(cadv_args *a, const char *query)
{
    char tok[128];
    if (!query) {
        return;
    }
    if (*query == '?') {
        ++query;
    }
    while (*query) {
        const char *amp = strchr(query, '&');
        size_t len = amp ? (size_t)(amp - query) : strlen(query);
        long v;
        if (len < sizeof tok) {
            memcpy(tok, query, len);
            tok[len] = '\0';
            if (strcmp(tok, "dev=unlockall") == 0) {
                a->unlock_all = true;
            } else if (strncmp(tok, "date=", 5) == 0 && cadv_datetime_parse(tok + 5, &a->date)) {
                a->has_date = true;
            } else if (strncmp(tok, "portal=", 7) == 0 &&
                       parse_int(tok + 7, 0, CADV_PORTAL_COUNT, &v)) {
                a->portal = (int)v;
            }
        }
        query += len;
        if (*query == '&') {
            ++query;
        }
    }
}

const char *cadv_args_usage(void)
{
    return "usage: cadvent [options]\n"
           "  --width N            window width (64..16384, default 1280)\n"
           "  --height N           window height (64..16384, default 720)\n"
           "  --internal-height N  low-res render height (90..720, default 225)\n"
           "  --portal N           start in portal N (1..24), 0 = the hall\n"
           "  --date DATE          pretend the local date is YYYY-MM-DD[THH:MM]\n"
           "  --unlock-all         development override: open everything\n"
           "  --headless           simulate without window or GPU (tests, bots)\n"
           "  --script FILE        run a scenario script (.gts)\n"
           "  --max-ticks N        fail if the run takes more ticks (headless)\n"
           "  --save FILE          save file location\n"
           "  --seed N             world seed (default 2026)\n"
           "  --frames N           quit after N frames (0 = run until closed)\n"
           "  --screenshot FILE    write the last frame as binary PPM\n"
           "  --log-level LEVEL    debug|info|warn|error|off (default info)\n"
           "  --version            print version and exit\n"
           "  -h, --help           print this help and exit\n";
}

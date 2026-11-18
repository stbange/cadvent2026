/* SPDX-License-Identifier: MIT */
#include "core/log.h"

#include <stdarg.h>
#include <stdio.h>

static cadv_log_level g_level = CADV_LOG_INFO;
static cadv_log_sink g_sink = NULL;
static void *g_user = NULL;

void cadv_log_set_level(cadv_log_level level) { g_level = level; }

cadv_log_level cadv_log_get_level(void) { return g_level; }

void cadv_log_set_sink(cadv_log_sink sink, void *user)
{
    g_sink = sink;
    g_user = user;
}

const char *cadv_log_level_name(cadv_log_level level)
{
    switch (level) {
    case CADV_LOG_DEBUG: return "debug";
    case CADV_LOG_INFO: return "info";
    case CADV_LOG_WARN: return "warn";
    case CADV_LOG_ERROR: return "error";
    default: return "off";
    }
}

void cadv_log(cadv_log_level level, const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    if (level < g_level || level >= CADV_LOG_OFF) {
        return;
    }
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (g_sink) {
        g_sink(level, buf, g_user);
    } else {
        fprintf(stderr, "[%s] %s\n", cadv_log_level_name(level), buf);
    }
}

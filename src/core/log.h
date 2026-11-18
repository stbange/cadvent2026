/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_LOG_H
#define CADV_CORE_LOG_H

typedef enum cadv_log_level {
    CADV_LOG_DEBUG = 0,
    CADV_LOG_INFO = 1,
    CADV_LOG_WARN = 2,
    CADV_LOG_ERROR = 3,
    CADV_LOG_OFF = 4
} cadv_log_level;

/* A sink receives every message at or above the current level. */
typedef void (*cadv_log_sink)(cadv_log_level level, const char *msg, void *user);

void cadv_log_set_level(cadv_log_level level);
cadv_log_level cadv_log_get_level(void);
/* Pass NULL to restore the default sink (stderr). */
void cadv_log_set_sink(cadv_log_sink sink, void *user);
const char *cadv_log_level_name(cadv_log_level level);
void cadv_log(cadv_log_level level, const char *fmt, ...)
#if defined(__GNUC__) || defined(__clang__)
    __attribute__((format(printf, 2, 3)))
#endif
    ;

#define CADV_DEBUG(...) cadv_log(CADV_LOG_DEBUG, __VA_ARGS__)
#define CADV_INFO(...) cadv_log(CADV_LOG_INFO, __VA_ARGS__)
#define CADV_WARN(...) cadv_log(CADV_LOG_WARN, __VA_ARGS__)
#define CADV_ERROR(...) cadv_log(CADV_LOG_ERROR, __VA_ARGS__)

#endif

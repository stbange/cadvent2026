/* SPDX-License-Identifier: MIT */
#ifndef CADV_CORE_CALENDAR_H
#define CADV_CORE_CALENDAR_H

#include <stdbool.h>
#include <stddef.h>

#define CADV_PORTAL_COUNT 24

typedef struct cadv_datetime {
    int year, month, day; /* local calendar date */
    int hour, minute;     /* local time */
} cadv_datetime;

typedef enum cadv_season {
    CADV_SEASON_TEASER = 0, /* before 1 December */
    CADV_SEASON_ADVENT = 1, /* 1..24 December */
    CADV_SEASON_POST = 2    /* from 25 December on (portals stay open) */
} cadv_season;

typedef enum cadv_portal_access {
    CADV_ACCESS_SEALED_DATE = 0,  /* its day has not come yet */
    CADV_ACCESS_SEALED_STARS = 1, /* portal 24 only: Star Arch incomplete */
    CADV_ACCESS_OPEN = 2
} cadv_portal_access;

cadv_datetime cadv_datetime_make(int year, int month, int day, int hour, int minute);
/* Parses "YYYY-MM-DD" or "YYYY-MM-DDTHH:MM". */
bool cadv_datetime_parse(const char *s, cadv_datetime *out);
bool cadv_datetime_valid(const cadv_datetime *dt);
/* Minutes since 1970-01-01 00:00 of this local wall-clock time. */
long long cadv_datetime_minutes(const cadv_datetime *dt);
int cadv_datetime_cmp(const cadv_datetime *a, const cadv_datetime *b);

cadv_season cadv_calendar_season(const cadv_datetime *now, int season_year);
/* True when portal 'day' (1..24) has reached its date (or unlock_all). */
bool cadv_calendar_date_eligible(int day, const cadv_datetime *now, int season_year, bool unlock_all);
cadv_portal_access cadv_calendar_access(int day, const cadv_datetime *now, int season_year,
                                        bool arch_complete, bool unlock_all);
/* Minutes until portal 'day' becomes date-eligible (0 if already). */
long long cadv_calendar_minutes_until(int day, const cadv_datetime *now, int season_year);
/* "opens in 2 d 4 h" / "opens in 3 h 5 min" / "opens in 12 min" / "open". */
void cadv_calendar_countdown_text(long long minutes, char *out, size_t cap);

#endif

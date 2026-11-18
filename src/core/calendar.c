/* SPDX-License-Identifier: MIT */
#include "core/calendar.h"

#include <stdio.h>
#include <string.h>

cadv_datetime cadv_datetime_make(int year, int month, int day, int hour, int minute)
{
    cadv_datetime dt;
    dt.year = year;
    dt.month = month;
    dt.day = day;
    dt.hour = hour;
    dt.minute = minute;
    return dt;
}

static int days_in_month(int y, int m)
{
    static const int d[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    return (m == 2 && leap) ? 29 : d[m - 1];
}

bool cadv_datetime_valid(const cadv_datetime *dt)
{
    return dt->year >= 1970 && dt->year <= 9999 && dt->month >= 1 && dt->month <= 12 &&
           dt->day >= 1 && dt->day <= days_in_month(dt->year, dt->month) && dt->hour >= 0 &&
           dt->hour <= 23 && dt->minute >= 0 && dt->minute <= 59;
}

bool cadv_datetime_parse(const char *s, cadv_datetime *out)
{
    cadv_datetime dt = cadv_datetime_make(0, 0, 0, 0, 0);
    char tail = 0;
    int n;
    size_t len = strlen(s);
    if (len == 10) {
        n = sscanf(s, "%4d-%2d-%2d%c", &dt.year, &dt.month, &dt.day, &tail);
        if (n != 3) {
            return false;
        }
    } else if (len == 16 && s[10] == 'T') {
        n = sscanf(s, "%4d-%2d-%2dT%2d:%2d%c", &dt.year, &dt.month, &dt.day, &dt.hour, &dt.minute,
                   &tail);
        if (n != 5) {
            return false;
        }
    } else {
        return false;
    }
    if (s[4] != '-' || s[7] != '-' || !cadv_datetime_valid(&dt)) {
        return false;
    }
    *out = dt;
    return true;
}

/* Howard Hinnant's days_from_civil. */
static long long days_from_civil(int y, int m, int d)
{
    long long yy = (long long)y - (m <= 2 ? 1 : 0);
    long long era = (yy >= 0 ? yy : yy - 399) / 400;
    long long yoe = yy - era * 400;
    long long mp = (m + 9) % 12;
    long long doy = (153 * mp + 2) / 5 + d - 1;
    long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

long long cadv_datetime_minutes(const cadv_datetime *dt)
{
    return days_from_civil(dt->year, dt->month, dt->day) * 1440LL + dt->hour * 60LL + dt->minute;
}

int cadv_datetime_cmp(const cadv_datetime *a, const cadv_datetime *b)
{
    long long x = cadv_datetime_minutes(a);
    long long y = cadv_datetime_minutes(b);
    return x < y ? -1 : (x > y ? 1 : 0);
}

cadv_season cadv_calendar_season(const cadv_datetime *now, int season_year)
{
    cadv_datetime start = cadv_datetime_make(season_year, 12, 1, 0, 0);
    cadv_datetime post = cadv_datetime_make(season_year, 12, 25, 0, 0);
    if (cadv_datetime_cmp(now, &start) < 0) {
        return CADV_SEASON_TEASER;
    }
    return cadv_datetime_cmp(now, &post) < 0 ? CADV_SEASON_ADVENT : CADV_SEASON_POST;
}

bool cadv_calendar_date_eligible(int day, const cadv_datetime *now, int season_year, bool unlock_all)
{
    cadv_datetime opens;
    if (day < 1 || day > CADV_PORTAL_COUNT) {
        return false;
    }
    if (unlock_all) {
        return true;
    }
    opens = cadv_datetime_make(season_year, 12, day, 0, 0);
    return cadv_datetime_cmp(now, &opens) >= 0;
}

cadv_portal_access cadv_calendar_access(int day, const cadv_datetime *now, int season_year,
                                        bool arch_complete, bool unlock_all)
{
    if (!cadv_calendar_date_eligible(day, now, season_year, unlock_all)) {
        return CADV_ACCESS_SEALED_DATE;
    }
    if (day == CADV_PORTAL_COUNT && !arch_complete && !unlock_all) {
        return CADV_ACCESS_SEALED_STARS;
    }
    return CADV_ACCESS_OPEN;
}

long long cadv_calendar_minutes_until(int day, const cadv_datetime *now, int season_year)
{
    cadv_datetime opens = cadv_datetime_make(season_year, 12, day, 0, 0);
    long long d = cadv_datetime_minutes(&opens) - cadv_datetime_minutes(now);
    return d > 0 ? d : 0;
}

void cadv_calendar_countdown_text(long long minutes, char *out, size_t cap)
{
    if (minutes <= 0) {
        snprintf(out, cap, "open");
    } else if (minutes >= 1440) {
        snprintf(out, cap, "opens in %lld d %lld h", minutes / 1440, (minutes % 1440) / 60);
    } else if (minutes >= 60) {
        snprintf(out, cap, "opens in %lld h %lld min", minutes / 60, minutes % 60);
    } else {
        snprintf(out, cap, "opens in %lld min", minutes);
    }
}

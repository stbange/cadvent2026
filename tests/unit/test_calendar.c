/* SPDX-License-Identifier: MIT */
/* Test names reference OpenSpec scenarios (capability__scenario). */
#include "core/calendar.h"
#include "unity.h"

#define Y 2026

void setUp(void) {}
void tearDown(void) {}

static cadv_datetime dt(int y, int mo, int d, int h, int mi) { return cadv_datetime_make(y, mo, d, h, mi); }

static void test_advent_calendar__portal_sealed_before_its_day(void)
{
    cadv_datetime now = dt(Y, 12, 4, 23, 59);
    int d;
    TEST_ASSERT_EQUAL_INT(CADV_ACCESS_SEALED_DATE, cadv_calendar_access(5, &now, Y, false, false));
    for (d = 1; d <= 4; ++d) {
        TEST_ASSERT_EQUAL_INT(CADV_ACCESS_OPEN, cadv_calendar_access(d, &now, Y, false, false));
    }
}

static void test_advent_calendar__portal_opens_at_local_midnight(void)
{
    cadv_datetime before = dt(Y, 12, 4, 23, 59);
    cadv_datetime after = dt(Y, 12, 5, 0, 0);
    TEST_ASSERT_FALSE(cadv_calendar_date_eligible(5, &before, Y, false));
    TEST_ASSERT_TRUE(cadv_calendar_date_eligible(5, &after, Y, false));
}

static void test_advent_calendar__time_zone_independence(void)
{
    /* The rule only looks at local wall-clock time, so every player unlocks
     * at their own local midnight regardless of UTC offset. */
    cadv_datetime berlin = dt(Y, 12, 10, 0, 0);
    cadv_datetime tokyo = dt(Y, 12, 10, 0, 0);
    TEST_ASSERT_TRUE(cadv_calendar_date_eligible(10, &berlin, Y, false));
    TEST_ASSERT_TRUE(cadv_calendar_date_eligible(10, &tokyo, Y, false));
}

static void test_advent_calendar__teaser_before_december(void)
{
    cadv_datetime now = dt(Y, 11, 30, 12, 0);
    int d;
    char text[32];
    TEST_ASSERT_EQUAL_INT(CADV_SEASON_TEASER, cadv_calendar_season(&now, Y));
    for (d = 1; d <= CADV_PORTAL_COUNT; ++d) {
        TEST_ASSERT_EQUAL_INT(CADV_ACCESS_SEALED_DATE, cadv_calendar_access(d, &now, Y, true, false));
    }
    cadv_calendar_countdown_text(cadv_calendar_minutes_until(1, &now, Y), text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("opens in 12 h 0 min", text);
}

static void test_advent_calendar__after_christmas_eve(void)
{
    cadv_datetime now = dt(2027, 1, 15, 9, 0);
    int d;
    TEST_ASSERT_EQUAL_INT(CADV_SEASON_POST, cadv_calendar_season(&now, Y));
    for (d = 1; d <= CADV_PORTAL_COUNT; ++d) {
        TEST_ASSERT_EQUAL_INT(CADV_ACCESS_OPEN, cadv_calendar_access(d, &now, Y, true, false));
    }
}

static void test_advent_calendar__portals_stay_open_after_the_season(void)
{
    cadv_datetime now = dt(2027, 1, 15, 9, 0);
    TEST_ASSERT_EQUAL_INT(CADV_ACCESS_OPEN, cadv_calendar_access(23, &now, Y, false, false));
    TEST_ASSERT_EQUAL_INT(CADV_ACCESS_SEALED_STARS, cadv_calendar_access(24, &now, Y, false, false));
}

static void test_advent_calendar__override_via_url_parameter(void)
{
    cadv_datetime now = dt(Y, 11, 20, 10, 0);
    TEST_ASSERT_EQUAL_INT(CADV_ACCESS_OPEN, cadv_calendar_access(24, &now, Y, false, true));
}

static void test_advent_calendar__countdown_on_a_sealed_portal(void)
{
    cadv_datetime now = dt(Y, 12, 5, 20, 0);
    char text[32];
    cadv_calendar_countdown_text(cadv_calendar_minutes_until(8, &now, Y), text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("opens in 2 d 4 h", text);
    cadv_calendar_countdown_text(12, text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("opens in 12 min", text);
    cadv_calendar_countdown_text(cadv_calendar_minutes_until(3, &now, Y), text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("open", text);
}

static void test_season_during_advent_and_invalid_days(void)
{
    cadv_datetime now = dt(Y, 12, 24, 23, 59);
    TEST_ASSERT_EQUAL_INT(CADV_SEASON_ADVENT, cadv_calendar_season(&now, Y));
    TEST_ASSERT_FALSE(cadv_calendar_date_eligible(0, &now, Y, true));
    TEST_ASSERT_FALSE(cadv_calendar_date_eligible(25, &now, Y, true));
}

static void test_parse_dates(void)
{
    cadv_datetime d;
    TEST_ASSERT_TRUE(cadv_datetime_parse("2026-12-05", &d));
    TEST_ASSERT_EQUAL_INT(5, d.day);
    TEST_ASSERT_EQUAL_INT(0, d.hour);
    TEST_ASSERT_TRUE(cadv_datetime_parse("2026-12-05T20:30", &d));
    TEST_ASSERT_EQUAL_INT(30, d.minute);
    TEST_ASSERT_TRUE(cadv_datetime_parse("2024-02-29", &d)); /* leap year */
    TEST_ASSERT_TRUE(cadv_datetime_parse("2000-02-29", &d)); /* leap century */
    TEST_ASSERT_FALSE(cadv_datetime_parse("1900-02-29", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("2026-02-29", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("2026-13-01", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("2026/12/05", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("2026-12-5", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("2026-12-05X20:30", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("2026-12-05T25:00", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("abcd-ef-gh", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("2026-12-05Tab:cd", &d));
    TEST_ASSERT_FALSE(cadv_datetime_parse("1969-12-31", &d));
}

static void test_minutes_and_compare(void)
{
    cadv_datetime a = dt(1970, 1, 1, 0, 0);
    cadv_datetime b = dt(1970, 1, 2, 1, 1);
    cadv_datetime c = dt(2026, 12, 1, 0, 0);
    TEST_ASSERT_EQUAL_INT64(0, cadv_datetime_minutes(&a));
    TEST_ASSERT_EQUAL_INT64(1440 + 61, cadv_datetime_minutes(&b));
    TEST_ASSERT_EQUAL_INT(-1, cadv_datetime_cmp(&a, &b));
    TEST_ASSERT_EQUAL_INT(1, cadv_datetime_cmp(&b, &a));
    TEST_ASSERT_EQUAL_INT(0, cadv_datetime_cmp(&c, &c));
    TEST_ASSERT_EQUAL_INT64(20788LL * 1440LL, cadv_datetime_minutes(&c));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_advent_calendar__portal_sealed_before_its_day);
    RUN_TEST(test_advent_calendar__portal_opens_at_local_midnight);
    RUN_TEST(test_advent_calendar__time_zone_independence);
    RUN_TEST(test_advent_calendar__teaser_before_december);
    RUN_TEST(test_advent_calendar__after_christmas_eve);
    RUN_TEST(test_advent_calendar__portals_stay_open_after_the_season);
    RUN_TEST(test_advent_calendar__override_via_url_parameter);
    RUN_TEST(test_advent_calendar__countdown_on_a_sealed_portal);
    RUN_TEST(test_season_during_advent_and_invalid_days);
    RUN_TEST(test_parse_dates);
    RUN_TEST(test_minutes_and_compare);
    return UNITY_END();
}

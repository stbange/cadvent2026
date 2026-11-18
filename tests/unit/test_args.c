/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "core/args.h"
#include "unity.h"

static cadv_args g_args;

void setUp(void) { cadv_args_defaults(&g_args); }
void tearDown(void) {}

#define PARSE(...)                                                                     \
    do {                                                                               \
        char *argv_[] = {"cadvent", __VA_ARGS__};                                      \
        g_status = cadv_args_parse(&g_args, (int)(sizeof argv_ / sizeof argv_[0]), argv_); \
    } while (0)

static cadv_args_status g_status;

static void test_defaults(void)
{
    char *argv[] = {"cadvent"};
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_RUN, cadv_args_parse(&g_args, 1, argv));
    TEST_ASSERT_EQUAL_INT(1280, g_args.width);
    TEST_ASSERT_EQUAL_INT(720, g_args.height);
    TEST_ASSERT_EQUAL_INT(0, g_args.frames);
    TEST_ASSERT_NULL(g_args.screenshot);
    TEST_ASSERT_EQUAL_INT(CADV_LOG_INFO, g_args.log_level);
    TEST_ASSERT_EQUAL_INT(225, g_args.internal_height);
    TEST_ASSERT_EQUAL_INT(0, g_args.portal);
    TEST_ASSERT_FALSE(g_args.headless);
}

static void test_all_options(void)
{
    PARSE("--width", "320", "--height", "180", "--frames", "3", "--screenshot", "out.ppm",
          "--log-level", "debug");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_RUN, g_status);
    TEST_ASSERT_EQUAL_INT(320, g_args.width);
    TEST_ASSERT_EQUAL_INT(180, g_args.height);
    TEST_ASSERT_EQUAL_INT(3, g_args.frames);
    TEST_ASSERT_EQUAL_STRING("out.ppm", g_args.screenshot);
    TEST_ASSERT_EQUAL_INT(CADV_LOG_DEBUG, g_args.log_level);
}

static void test_log_levels(void)
{
    PARSE("--log-level", "off");
    TEST_ASSERT_EQUAL_INT(CADV_LOG_OFF, g_args.log_level);
    PARSE("--log-level", "loud");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_ERROR, g_status);
    TEST_ASSERT_EQUAL_STRING("invalid log level: loud", g_args.error);
}

static void test_help_and_version_exit_ok(void)
{
    PARSE("--help");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_EXIT_OK, g_status);
    TEST_ASSERT_TRUE(g_args.show_help);
    cadv_args_defaults(&g_args);
    PARSE("-h");
    TEST_ASSERT_TRUE(g_args.show_help);
    cadv_args_defaults(&g_args);
    PARSE("--version");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_EXIT_OK, g_status);
    TEST_ASSERT_TRUE(g_args.show_version);
    TEST_ASSERT_NOT_NULL(strstr(cadv_args_usage(), "--screenshot"));
}

static void test_rejects_bad_values(void)
{
    PARSE("--width", "abc");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_ERROR, g_status);
    TEST_ASSERT_EQUAL_STRING("invalid width: abc", g_args.error);
    PARSE("--width", "10");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_ERROR, g_status);
    PARSE("--height", "99999");
    TEST_ASSERT_EQUAL_STRING("invalid height: 99999", g_args.error);
    PARSE("--frames", "-1");
    TEST_ASSERT_EQUAL_STRING("invalid frame count: -1", g_args.error);
    PARSE("--frames", "12x");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_ERROR, g_status);
    PARSE("--frames", "99999999999999999999");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_ERROR, g_status);
}

static void test_new_options(void)
{
    PARSE("--headless", "--unlock-all", "--portal", "7", "--script", "s.gts", "--save", "x.sav",
          "--seed", "42", "--max-ticks", "1000", "--internal-height", "180", "--date",
          "2026-12-05T20:00");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_RUN, g_status);
    TEST_ASSERT_TRUE(g_args.headless);
    TEST_ASSERT_TRUE(g_args.unlock_all);
    TEST_ASSERT_EQUAL_INT(7, g_args.portal);
    TEST_ASSERT_EQUAL_STRING("s.gts", g_args.script);
    TEST_ASSERT_EQUAL_STRING("x.sav", g_args.save_path);
    TEST_ASSERT_EQUAL_UINT(42, g_args.seed);
    TEST_ASSERT_EQUAL_INT(1000, g_args.max_ticks);
    TEST_ASSERT_EQUAL_INT(180, g_args.internal_height);
    TEST_ASSERT_TRUE(g_args.has_date);
    TEST_ASSERT_EQUAL_INT(20, g_args.date.hour);
    PARSE("--portal", "25");
    TEST_ASSERT_EQUAL_STRING("invalid portal: 25", g_args.error);
    PARSE("--date", "2026-13-01");
    TEST_ASSERT_EQUAL_STRING("invalid date: 2026-13-01", g_args.error);
}

static void test_url_query(void)
{
    cadv_args_apply_query(&g_args, NULL);
    TEST_ASSERT_FALSE(g_args.unlock_all);
    cadv_args_apply_query(&g_args, "?foo=1&dev=unlockall&date=2026-12-24&portal=3&portal=99&date=bad");
    TEST_ASSERT_TRUE(g_args.unlock_all);
    TEST_ASSERT_TRUE(g_args.has_date);
    TEST_ASSERT_EQUAL_INT(24, g_args.date.day);
    TEST_ASSERT_EQUAL_INT(3, g_args.portal);
    cadv_args_defaults(&g_args);
    cadv_args_apply_query(&g_args, "portal=2&&xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx");
    TEST_ASSERT_EQUAL_INT(2, g_args.portal);
}

static void test_rejects_missing_value_and_unknown(void)
{
    PARSE("--width");
    TEST_ASSERT_EQUAL_INT(CADV_ARGS_ERROR, g_status);
    TEST_ASSERT_EQUAL_STRING("missing value for option: --width", g_args.error);
    PARSE("--unlock-everything");
    TEST_ASSERT_EQUAL_STRING("unknown option: --unlock-everything", g_args.error);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_defaults);
    RUN_TEST(test_all_options);
    RUN_TEST(test_log_levels);
    RUN_TEST(test_help_and_version_exit_ok);
    RUN_TEST(test_rejects_bad_values);
    RUN_TEST(test_new_options);
    RUN_TEST(test_url_query);
    RUN_TEST(test_rejects_missing_value_and_unknown);
    return UNITY_END();
}

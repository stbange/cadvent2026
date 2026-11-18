/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "core/log.h"
#include "unity.h"

static int g_calls;
static cadv_log_level g_last_level;
static char g_last_msg[256];

static void capture(cadv_log_level level, const char *msg, void *user)
{
    int *counter = (int *)user;
    ++*counter;
    g_last_level = level;
    strncpy(g_last_msg, msg, sizeof g_last_msg - 1);
}

void setUp(void)
{
    g_calls = 0;
    g_last_msg[0] = '\0';
    cadv_log_set_level(CADV_LOG_DEBUG);
    cadv_log_set_sink(capture, &g_calls);
}

void tearDown(void)
{
    cadv_log_set_sink(NULL, NULL);
    cadv_log_set_level(CADV_LOG_INFO);
}

static void test_formats_and_forwards_to_sink(void)
{
    CADV_WARN("portal %d of %d", 5, 24);
    TEST_ASSERT_EQUAL_INT(1, g_calls);
    TEST_ASSERT_EQUAL_INT(CADV_LOG_WARN, g_last_level);
    TEST_ASSERT_EQUAL_STRING("portal 5 of 24", g_last_msg);
}

static void test_filters_below_level(void)
{
    cadv_log_set_level(CADV_LOG_ERROR);
    TEST_ASSERT_EQUAL_INT(CADV_LOG_ERROR, cadv_log_get_level());
    CADV_DEBUG("x");
    CADV_INFO("x");
    CADV_WARN("x");
    TEST_ASSERT_EQUAL_INT(0, g_calls);
    CADV_ERROR("boom");
    TEST_ASSERT_EQUAL_INT(1, g_calls);
}

static void test_off_level_is_never_emitted(void)
{
    cadv_log(CADV_LOG_OFF, "never");
    TEST_ASSERT_EQUAL_INT(0, g_calls);
    cadv_log_set_level(CADV_LOG_OFF);
    CADV_ERROR("silenced");
    TEST_ASSERT_EQUAL_INT(0, g_calls);
}

static void test_level_names(void)
{
    TEST_ASSERT_EQUAL_STRING("debug", cadv_log_level_name(CADV_LOG_DEBUG));
    TEST_ASSERT_EQUAL_STRING("info", cadv_log_level_name(CADV_LOG_INFO));
    TEST_ASSERT_EQUAL_STRING("warn", cadv_log_level_name(CADV_LOG_WARN));
    TEST_ASSERT_EQUAL_STRING("error", cadv_log_level_name(CADV_LOG_ERROR));
    TEST_ASSERT_EQUAL_STRING("off", cadv_log_level_name(CADV_LOG_OFF));
}

static void test_default_sink_writes_to_stderr(void)
{
    cadv_log_set_sink(NULL, NULL);
    CADV_INFO("default sink smoke test (expected on stderr)");
    TEST_ASSERT_EQUAL_INT(0, g_calls);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_formats_and_forwards_to_sink);
    RUN_TEST(test_filters_below_level);
    RUN_TEST(test_off_level_is_never_emitted);
    RUN_TEST(test_level_names);
    RUN_TEST(test_default_sink_writes_to_stderr);
    return UNITY_END();
}

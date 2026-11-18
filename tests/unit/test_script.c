/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <string.h>

#include "portal/script.h"
#include "unity.h"

static cadv_script s;
static double g_x, g_z, g_yaw, g_mode, g_outcome;
static int g_teleports, g_bot_calls, g_logs, g_shots;
static bool g_shot_ok;
static bool g_query_yaw;

static bool query(void *u, const char *k, double *out)
{
    (void)u;
    if (strcmp(k, "player.x") == 0) { *out = g_x; return true; }
    if (strcmp(k, "player.z") == 0) { *out = g_z; return true; }
    if (strcmp(k, "player.yaw") == 0 && g_query_yaw) { *out = g_yaw; return true; }
    if (strcmp(k, "mode") == 0) { *out = g_mode; return true; }
    if (strcmp(k, "outcome") == 0) { *out = g_outcome; return true; }
    return false;
}
static void teleport(void *u, float x, float y, float z, float yaw) { (void)u; (void)y; g_x = x; g_z = z; g_yaw = yaw; g_teleports++; }
static void bot(void *u, cadv_input_frame *out) { (void)u; out->move_y = 1.0f; g_bot_calls++; }
static bool event(void *u, const char *name) { (void)u; return strcmp(name, "portal_won") == 0; }
static bool shot(void *u, const char *p) { (void)u; (void)p; g_shots++; return g_shot_ok; }
static void logf_(void *u, const char *t) { (void)u; (void)t; g_logs++; }

static cadv_script_host host;

void setUp(void)
{
    g_x = g_z = g_yaw = g_mode = g_outcome = 0.0;
    g_teleports = g_bot_calls = g_logs = g_shots = 0;
    g_shot_ok = true;
    g_query_yaw = true;
    host.user = NULL;
    host.query = query;
    host.teleport = teleport;
    host.bot = bot;
    host.event = event;
    host.screenshot = shot;
    host.log = logf_;
}
void tearDown(void) {}

static int run(int max)
{
    cadv_input_frame f;
    int n = 0;
    while (n < max && cadv_script_step(&s, &host, &f)) {
        ++n;
    }
    return n;
}

static void test_parse_errors(void)
{
    char longline[600];
    TEST_ASSERT_FALSE(cadv_script_parse(&s, "wait"));
    TEST_ASSERT_EQUAL_STRING("line 1: unknown command or wrong argument count", s.error);
    TEST_ASSERT_FALSE(cadv_script_parse(&s, "# ok\nhold fly 3"));
    TEST_ASSERT_EQUAL_STRING("line 2: unknown action", s.error);
    TEST_ASSERT_FALSE(cadv_script_parse(&s, "expect a ~ 3"));
    TEST_ASSERT_EQUAL_STRING("line 1: unknown comparison", s.error);
    memset(longline, 'x', sizeof longline - 1);
    longline[sizeof longline - 1] = '\0';
    TEST_ASSERT_FALSE(cadv_script_parse(&s, longline));
    TEST_ASSERT_FALSE(cadv_script_load(&s, "missing.gts"));
}

static void test_too_many_commands(void)
{
    static char text[CADV_SCRIPT_MAX_CMDS * 8 + 32];
    int i;
    text[0] = '\0';
    for (i = 0; i <= CADV_SCRIPT_MAX_CMDS; ++i) {
        strcat(text, "wait 1\n");
    }
    TEST_ASSERT_FALSE(cadv_script_parse(&s, text));
}

static void test_wait_hold_press_move_turn(void)
{
    cadv_input_frame f;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "wait 2\r\nhold fire 2\npress use\nmove 0.5 1 1\nturn 0.1 -0.2\n\n"));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f)); /* hold tick 1 */
    TEST_ASSERT_TRUE(cadv_input_pressed(&f, CADV_ACT_FIRE));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f)); /* hold tick 2 */
    TEST_ASSERT_TRUE(cadv_input_down(&f, CADV_ACT_FIRE));
    TEST_ASSERT_FALSE(cadv_input_pressed(&f, CADV_ACT_FIRE));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_TRUE(cadv_input_pressed(&f, CADV_ACT_USE));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_EQUAL_FLOAT(0.5f, f.move_x);
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_EQUAL_FLOAT(-0.2f, f.look_y);
    TEST_ASSERT_FALSE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_TRUE(s.done);
    TEST_ASSERT_FALSE(s.failed);
    TEST_ASSERT_FALSE(cadv_script_step(&s, &host, &f)); /* stays done */
}

static void test_face_and_teleport(void)
{
    cadv_input_frame f;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "teleport 1 0 2 0.5\nface 1.0"));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_EQUAL_INT(1, g_teleports);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.5f, f.look_x);
    g_query_yaw = false;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "face 1.0"));
    TEST_ASSERT_FALSE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_TRUE(s.failed);
}

static void test_walk_to(void)
{
    cadv_input_frame f;
    int i;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "walk_to 0 -5 100"));
    for (i = 0; i < 200; ++i) {
        if (!cadv_script_step(&s, &host, &f)) {
            break;
        }
        g_z -= 0.2 * f.move_y; /* fake movement towards -Z */
    }
    TEST_ASSERT_TRUE(s.done);
    TEST_ASSERT_FALSE(s.failed);
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "walk_to 50 50 3"));
    run(10);
    TEST_ASSERT_TRUE(s.failed);
    TEST_ASSERT_EQUAL_STRING("line 1: walk_to: timed out", s.error);
    g_query_yaw = false;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "walk_to 1 1 3"));
    run(10);
    TEST_ASSERT_TRUE(s.failed);
}

static void test_bot_until_outcome_or_mode_change(void)
{
    cadv_input_frame f;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "bot 100"));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_EQUAL_FLOAT(1.0f, f.move_y);
    g_outcome = 1.0;
    TEST_ASSERT_FALSE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_FALSE(s.failed);
    g_outcome = 0.0;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "bot 100"));
    TEST_ASSERT_TRUE(cadv_script_step(&s, &host, &f));
    g_mode = 1.0;
    TEST_ASSERT_FALSE(cadv_script_step(&s, &host, &f));
    TEST_ASSERT_FALSE(s.failed);
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "bot 2"));
    run(10);
    TEST_ASSERT_TRUE(s.failed);
}

static void test_expect_wait_until_events(void)
{
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect mode == 0\nexpect_event portal_won\nwait_until mode >= 1 5"));
    TEST_ASSERT_EQUAL_INT(5, run(100));
    TEST_ASSERT_TRUE(s.failed);
    TEST_ASSERT_NOT_NULL(strstr(s.error, "expected mode (0) to be >= 1"));
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect nope == 1"));
    run(5);
    TEST_ASSERT_EQUAL_STRING("line 1: unknown key 'nope'", s.error);
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect_event portal_lost"));
    run(5);
    TEST_ASSERT_TRUE(s.failed);
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect mode != 0"));
    run(5);
    TEST_ASSERT_NOT_NULL(strstr(s.error, "!="));
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect mode < 0"));
    run(5);
    TEST_ASSERT_NOT_NULL(strstr(s.error, "<"));
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect mode <= -1"));
    run(5);
    TEST_ASSERT_NOT_NULL(strstr(s.error, "<="));
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect mode > 0"));
    run(5);
    TEST_ASSERT_NOT_NULL(strstr(s.error, "> 0"));
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect mode == 1"));
    run(5);
    TEST_ASSERT_NOT_NULL(strstr(s.error, "== 1"));
}

static void test_screenshot_print_log(void)
{
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "screenshot a.ppm\nprint mode\nprint nope\nlog hello world"));
    TEST_ASSERT_EQUAL_INT(1, run(10)); /* screenshot ends its tick */
    TEST_ASSERT_EQUAL_INT(1, g_shots);
    TEST_ASSERT_EQUAL_INT(3, g_logs);
    TEST_ASSERT_FALSE(s.failed);
    g_shot_ok = false;
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "screenshot b.ppm"));
    run(5);
    TEST_ASSERT_TRUE(s.failed);
}

static void test_null_host_callbacks(void)
{
    cadv_script_host empty;
    cadv_input_frame f;
    memset(&empty, 0, sizeof empty);
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "teleport 0 0 0 0\nscreenshot x.ppm\nlog hi\nprint mode\nbot 1"));
    while (cadv_script_step(&s, &empty, &f)) {
    }
    TEST_ASSERT_TRUE(s.failed); /* bot without outcome times out */
    TEST_ASSERT_TRUE(cadv_script_parse(&s, "expect_event x"));
    cadv_script_step(&s, &empty, &f);
    TEST_ASSERT_TRUE(s.failed);
}

static void test_load_file_and_helpers(void)
{
    FILE *f = fopen("test_tmp.gts", "wb");
    fputs("wait 1\n", f);
    fclose(f);
    TEST_ASSERT_TRUE(cadv_script_load(&s, "test_tmp.gts"));
    TEST_ASSERT_EQUAL_INT(1, s.count);
    remove("test_tmp.gts");
    TEST_ASSERT_EQUAL_INT(CADV_ACT_USE, cadv_script_action_from_name("use"));
    TEST_ASSERT_EQUAL_INT(-1, cadv_script_action_from_name("dance"));
    TEST_ASSERT_TRUE(cadv_script_compare(1.0, CADV_CMP_GE, 1.0));
    TEST_ASSERT_FALSE(cadv_script_compare(0.0, CADV_CMP_GE, 1.0));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_errors);
    RUN_TEST(test_too_many_commands);
    RUN_TEST(test_wait_hold_press_move_turn);
    RUN_TEST(test_face_and_teleport);
    RUN_TEST(test_walk_to);
    RUN_TEST(test_bot_until_outcome_or_mode_change);
    RUN_TEST(test_expect_wait_until_events);
    RUN_TEST(test_screenshot_print_log);
    RUN_TEST(test_null_host_callbacks);
    RUN_TEST(test_load_file_and_helpers);
    return UNITY_END();
}

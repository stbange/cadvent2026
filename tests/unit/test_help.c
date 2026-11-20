/* SPDX-License-Identifier: MIT */
/* On-screen help: content for every world, word wrap, layout and pages. */
#include <string.h>

#include "portal/help.h"
#include "portal/portal.h"
#include "unity.h"

static cadv_ui ui;

void setUp(void) { cadv_ui_reset(&ui, 400, 225); }
void tearDown(void) {}

static void test_on_screen_help__every_world_has_a_help_page(void)
{
    int day;
    TEST_ASSERT_EQUAL_STRING("THE HALL OF LONG NIGHTS", cadv_help_get(0, false)->title);
    TEST_ASSERT_EQUAL_STRING("DAY 1 - SLEIGHED!", cadv_help_get(1, true)->title);
    TEST_ASSERT_EQUAL_STRING("DAY 2 - AURORA CARPET", cadv_help_get(2, true)->title);
    TEST_ASSERT_EQUAL_STRING("DAY 3 - FROSTFRONT", cadv_help_get(3, true)->title);
    TEST_ASSERT_EQUAL_STRING("DAY 4 - ICE TOMB", cadv_help_get(4, true)->title);
    TEST_ASSERT_EQUAL_STRING("DAY 5 - HEARTHGUARD", cadv_help_get(5, true)->title);
    TEST_ASSERT_EQUAL_STRING("DAY 6 - IRON YULE", cadv_help_get(6, true)->title);
    TEST_ASSERT_EQUAL_STRING("DAY 7 - TINSEL STRIKE", cadv_help_get(7, true)->title);
    TEST_ASSERT_EQUAL_STRING("PRACTICE RANGE", cadv_help_get(9, false)->title);
    for (day = 0; day <= CADV_PORTAL_COUNT; ++day) {
        const cadv_portal_desc *d = day > 0 ? cadv_portal_get(day) : NULL;
        const cadv_help *h = cadv_help_get(day, d && d->implemented);
        int i;
        bool has_help_key = false;
        TEST_ASSERT_NOT_NULL(h->goal);
        TEST_ASSERT_TRUE(h->control_count > 0 && h->control_count <= CADV_HELP_MAX_CONTROLS);
        TEST_ASSERT_TRUE(h->tip_count <= CADV_HELP_MAX_TIPS);
        for (i = 0; i < h->control_count; ++i) {
            TEST_ASSERT_NOT_NULL(h->controls[i].keys);
            TEST_ASSERT_NOT_NULL(h->controls[i].pad);
            TEST_ASSERT_TRUE(strlen(h->controls[i].action) <= 20);
            has_help_key = has_help_key || strcmp(h->controls[i].keys, "F1 / H") == 0;
        }
        TEST_ASSERT_TRUE(has_help_key); /* every page tells how to close it */
        if (d && d->implemented) {
            TEST_ASSERT_NOT_EQUAL(cadv_help_get(9, false), h); /* shipped days have their own page */
        }
    }
}

static void test_help_key_names(void)
{
    TEST_ASSERT_EQUAL_STRING("F1", cadv_help_key(CADV_DEVICE_KBM));
    TEST_ASSERT_EQUAL_STRING("BACK", cadv_help_key(CADV_DEVICE_GAMEPAD));
    TEST_ASSERT_EQUAL_STRING("?", cadv_help_key(CADV_DEVICE_TOUCH));
}

static void test_help_word_wrap(void)
{
    char lines[8][64];
    int n = cadv_help_wrap("ONE TWO THREE FOUR FIVE", 9, lines, 8);
    TEST_ASSERT_EQUAL_INT(3, n);
    TEST_ASSERT_EQUAL_STRING("ONE TWO", lines[0]);
    TEST_ASSERT_EQUAL_STRING("THREE", lines[1]);
    TEST_ASSERT_EQUAL_STRING("FOUR FIVE", lines[2]);
    TEST_ASSERT_EQUAL_INT(2, cadv_help_wrap("ONE TWO THREE FOUR FIVE", 9, lines, 2)); /* capped */
    TEST_ASSERT_EQUAL_INT(1, cadv_help_wrap("  SPACED   OUT  ", 20, lines, 8));
    TEST_ASSERT_EQUAL_STRING("SPACED OUT", lines[0]);
    TEST_ASSERT_EQUAL_INT(2, cadv_help_wrap("SUPERCALIFRAGILISTIC X", 5, lines, 8));
    TEST_ASSERT_EQUAL_STRING("SUPER", lines[0]); /* over-long words are cut */
    TEST_ASSERT_EQUAL_INT(0, cadv_help_wrap("", 10, lines, 8));
    TEST_ASSERT_EQUAL_INT(0, cadv_help_wrap("   ", 10, lines, 8));
    TEST_ASSERT_EQUAL_INT(0, cadv_help_wrap("ABC", 0, lines, 8));
    TEST_ASSERT_EQUAL_INT(0, cadv_help_wrap("ABC", 10, lines, 0));
    TEST_ASSERT_EQUAL_INT(1, cadv_help_wrap("ABC", 200, lines, 8)); /* width capped to the buffer */
}

static void test_help_draws_one_page_on_a_normal_screen(void)
{
    int day;
    for (day = 0; day <= 7; ++day) {
        cadv_ui_reset(&ui, 400, 225);
        TEST_ASSERT_EQUAL_INT(1, cadv_help_draw(&ui, cadv_help_get(day, true), CADV_DEVICE_KBM, 1.0f, 0, 0));
        TEST_ASSERT_TRUE(ui.count > 100);
    }
    cadv_ui_reset(&ui, 400, 225);
    cadv_help_draw(&ui, cadv_help_get(1, true), CADV_DEVICE_GAMEPAD, 1.0f, 20, 0);
    TEST_ASSERT_TRUE(ui.count > 100);
}

static void test_on_screen_help__two_pages_on_small_screens(void)
{
    int first;
    cadv_ui_reset(&ui, 320, 180);
    TEST_ASSERT_EQUAL_INT(2, cadv_help_draw(&ui, cadv_help_get(3, true), CADV_DEVICE_KBM, 1.0f, 0, 0));
    first = ui.count;
    cadv_ui_reset(&ui, 320, 180);
    TEST_ASSERT_EQUAL_INT(2, cadv_help_draw(&ui, cadv_help_get(3, true), CADV_DEVICE_TOUCH, 1.0f, 0, 1));
    TEST_ASSERT_TRUE(ui.count > 20 && ui.count != first);
    cadv_ui_reset(&ui, 200, 100); /* tiny: nothing overflows the panel */
    cadv_help_draw(&ui, cadv_help_get(0, false), CADV_DEVICE_KBM, 1.0f, 0, 0);
    cadv_help_draw(&ui, cadv_help_get(0, false), CADV_DEVICE_KBM, 1.0f, 0, 1);
}

static void test_help_slides_in(void)
{
    TEST_ASSERT_EQUAL_INT(1, cadv_help_draw(&ui, cadv_help_get(1, true), CADV_DEVICE_KBM, 0.2f, 0, 0));
    TEST_ASSERT_EQUAL_INT(3, ui.count); /* only the backdrop and panel while sliding in */
    cadv_ui_reset(&ui, 400, 225);
    cadv_help_draw(&ui, cadv_help_get(1, true), CADV_DEVICE_KBM, -1.0f, 0, 0);
    TEST_ASSERT_EQUAL_INT(3, ui.count);
}

static void test_help_hint_and_touch(void)
{
    cadv_input in;
    cadv_help_draw_hint(&ui, CADV_DEVICE_KBM, 1.0f);
    TEST_ASSERT_TRUE(ui.count > 0);
    cadv_ui_reset(&ui, 400, 225);
    cadv_help_draw_hint(&ui, CADV_DEVICE_TOUCH, 0.0f);
    TEST_ASSERT_EQUAL_INT(0, ui.count);
    cadv_input_init(&in);
    cadv_help_draw_touch(&ui, &in);
    TEST_ASSERT_EQUAL_INT(0, ui.count); /* no touch yet: nothing */
    cadv_input_finger_down(&in, 1, 0.2f, 0.7f);
    cadv_input_finger_motion(&in, 1, 0.25f, 0.65f);
    cadv_help_draw_touch(&ui, &in);
    TEST_ASSERT_TRUE(ui.count > 5);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_on_screen_help__every_world_has_a_help_page);
    RUN_TEST(test_help_key_names);
    RUN_TEST(test_help_word_wrap);
    RUN_TEST(test_help_draws_one_page_on_a_normal_screen);
    RUN_TEST(test_on_screen_help__two_pages_on_small_screens);
    RUN_TEST(test_help_slides_in);
    RUN_TEST(test_help_hint_and_touch);
    return UNITY_END();
}

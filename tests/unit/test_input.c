/* SPDX-License-Identifier: MIT */
#include "input/input.h"
#include "unity.h"

static cadv_input in;

void setUp(void) { cadv_input_init(&in); }
void tearDown(void) {}

static void test_input_controls__remap_jump(void)
{
    cadv_input_frame f;
    cadv_input_bind_key(&in, CADV_KEY_R, CADV_ACT_JUMP);
    cadv_input_key(&in, CADV_KEY_R, true);
    f = cadv_input_frame_build(&in, 1.0f / 60.0f);
    TEST_ASSERT_TRUE(cadv_input_down(&f, CADV_ACT_JUMP));
    TEST_ASSERT_TRUE(cadv_input_pressed(&f, CADV_ACT_JUMP));
    f = cadv_input_frame_build(&in, 1.0f / 60.0f);
    TEST_ASSERT_TRUE(cadv_input_down(&f, CADV_ACT_JUMP));
    TEST_ASSERT_FALSE(cadv_input_pressed(&f, CADV_ACT_JUMP)); /* edge only once */
    cadv_input_unbind_key(&in, CADV_KEY_R);
    f = cadv_input_frame_build(&in, 1.0f / 60.0f);
    TEST_ASSERT_FALSE(cadv_input_down(&f, CADV_ACT_JUMP));
}

static void test_keyboard_movement_is_normalised(void)
{
    cadv_input_frame f;
    cadv_input_key(&in, CADV_KEY_W, true);
    cadv_input_key(&in, CADV_KEY_D, true);
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.7071068f, f.move_x);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.7071068f, f.move_y);
    cadv_input_key(&in, CADV_KEY_W, false);
    cadv_input_key(&in, CADV_KEY_D, false);
    cadv_input_key(&in, CADV_KEY_LEFT, true);
    cadv_input_key(&in, CADV_KEY_DOWN, true);
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_TRUE(f.move_x < 0.0f && f.move_y < 0.0f);
    cadv_input_key(&in, -1, true);   /* ignored */
    cadv_input_key(&in, 9999, true); /* ignored */
    cadv_input_bind_key(&in, 9999, CADV_ACT_JUMP);
    cadv_input_unbind_key(&in, -5);
}

static void test_mouse_look_and_buttons(void)
{
    cadv_input_frame f;
    cadv_input_mouse_motion(&in, 100.0f, -40.0f);
    cadv_input_mouse_button(&in, CADV_MOUSE_LEFT, true);
    cadv_input_mouse_button(&in, 7, true); /* ignored */
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.25f, f.look_x);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.1f, f.look_y);
    TEST_ASSERT_TRUE(cadv_input_down(&f, CADV_ACT_FIRE));
    in.invert_y = true;
    cadv_input_mouse_motion(&in, 0.0f, -40.0f);
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, -0.1f, f.look_y);
    TEST_ASSERT_EQUAL_INT(CADV_DEVICE_KBM, in.last_device);
}

static void test_input_controls__hot_plug_gamepad(void)
{
    cadv_input_frame f;
    cadv_input_pad_axis(&in, CADV_AXIS_LX, 0.1f); /* inside dead zone */
    TEST_ASSERT_EQUAL_INT(CADV_DEVICE_KBM, in.last_device);
    cadv_input_pad_axis(&in, CADV_AXIS_LY, -1.0f);
    cadv_input_pad_axis(&in, CADV_AXIS_RX, 1.0f);
    cadv_input_pad_axis(&in, 99, 1.0f); /* ignored */
    cadv_input_pad_button(&in, CADV_PAD_SOUTH, true);
    cadv_input_pad_button(&in, CADV_PAD_RTRIGGER, true);
    cadv_input_pad_button(&in, -1, true); /* ignored */
    f = cadv_input_frame_build(&in, 0.5f);
    TEST_ASSERT_EQUAL_INT(CADV_DEVICE_GAMEPAD, in.last_device);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, f.move_x);
    TEST_ASSERT_EQUAL_FLOAT(1.0f, f.move_y);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, -1.5f, f.look_x);
    TEST_ASSERT_TRUE(cadv_input_down(&f, CADV_ACT_JUMP));
    TEST_ASSERT_TRUE(cadv_input_down(&f, CADV_ACT_FIRE));
}

static void test_input_controls__phone_opens_the_game(void)
{
    cadv_input_frame f;
    TEST_ASSERT_FALSE(cadv_input_touch_visible(&in));
    cadv_input_finger_down(&in, 1, 0.2f, 0.7f); /* left half: stick */
    cadv_input_finger_motion(&in, 1, 0.2f, 0.58f);
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_TRUE(cadv_input_touch_visible(&in));
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, 1.0f, f.move_y);
    cadv_input_finger_down(&in, 2, 0.6f, 0.3f); /* right half: look */
    cadv_input_finger_motion(&in, 2, 0.65f, 0.3f);
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_FLOAT_WITHIN(1e-4f, -0.2f, f.look_x);
    cadv_input_finger_down(&in, 3, 0.90f, 0.80f); /* jump button */
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_TRUE(cadv_input_pressed(&f, CADV_ACT_JUMP));
    cadv_input_finger_up(&in, 3);
    cadv_input_finger_up(&in, 1);
    cadv_input_finger_up(&in, 42); /* unknown finger ignored */
    cadv_input_finger_motion(&in, 42, 0.1f, 0.1f);
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_FALSE(cadv_input_down(&f, CADV_ACT_JUMP));
    TEST_ASSERT_EQUAL_FLOAT(0.0f, f.move_y);
}

static void test_input_controls__back_to_keyboard(void)
{
    cadv_input_finger_down(&in, 1, 0.2f, 0.7f);
    TEST_ASSERT_TRUE(cadv_input_touch_visible(&in));
    cadv_input_key(&in, CADV_KEY_W, true);
    TEST_ASSERT_FALSE(cadv_input_touch_visible(&in));
}

static void test_finger_slots_are_limited(void)
{
    int i;
    for (i = 0; i < CADV_MAX_FINGERS + 2; ++i) {
        cadv_input_finger_down(&in, i, 0.3f, 0.3f);
    }
    TEST_ASSERT_TRUE(in.fingers[CADV_MAX_FINGERS - 1].active);
}

static void test_release_all(void)
{
    cadv_input_frame f;
    cadv_input_key(&in, CADV_KEY_SPACE, true);
    cadv_input_mouse_button(&in, CADV_MOUSE_RIGHT, true);
    cadv_input_pad_button(&in, CADV_PAD_NORTH, true);
    cadv_input_finger_down(&in, 3, 0.90f, 0.80f);
    cadv_input_release_all(&in);
    f = cadv_input_frame_build(&in, 0.016f);
    TEST_ASSERT_EQUAL_UINT32(0, f.held);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_input_controls__remap_jump);
    RUN_TEST(test_keyboard_movement_is_normalised);
    RUN_TEST(test_mouse_look_and_buttons);
    RUN_TEST(test_input_controls__hot_plug_gamepad);
    RUN_TEST(test_input_controls__phone_opens_the_game);
    RUN_TEST(test_input_controls__back_to_keyboard);
    RUN_TEST(test_finger_slots_are_limited);
    RUN_TEST(test_release_all);
    return UNITY_END();
}

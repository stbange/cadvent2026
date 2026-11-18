/* SPDX-License-Identifier: MIT */
#ifndef CADV_INPUT_INPUT_H
#define CADV_INPUT_INPUT_H

#include <stdbool.h>
#include <stdint.h>

/* Device-independent input. The platform layer feeds raw events (keys as USB
 * HID usage codes, mouse, gamepad, touch); gameplay only ever reads actions. */

typedef enum cadv_action {
    CADV_ACT_JUMP = 0,
    CADV_ACT_CROUCH,
    CADV_ACT_FIRE,
    CADV_ACT_ALT_FIRE,
    CADV_ACT_USE,
    CADV_ACT_RADIAL,
    CADV_ACT_RUN,
    CADV_ACT_PAUSE,
    CADV_ACT_NEXT_WEAPON,
    CADV_ACT_PREV_WEAPON,
    CADV_ACT_WEAPON_1,
    CADV_ACT_WEAPON_2,
    CADV_ACT_WEAPON_3,
    CADV_ACT_WEAPON_4,
    CADV_ACT_WEAPON_5,
    CADV_ACT_WEAPON_6,
    CADV_ACT_MAP,
    CADV_ACT_COUNT
} cadv_action;

#define CADV_ACT_BIT(a) (1u << (unsigned)(a))

/* One tick of input for one player. */
typedef struct cadv_input_frame {
    float move_x, move_y; /* strafe right / forward, each -1..1 */
    float look_x, look_y; /* yaw / pitch change in radians for this tick */
    uint32_t held;        /* actions currently down */
    uint32_t pressed;     /* actions that went down this tick */
} cadv_input_frame;

bool cadv_input_down(const cadv_input_frame *f, cadv_action a);
bool cadv_input_pressed(const cadv_input_frame *f, cadv_action a);

typedef enum cadv_device_kind {
    CADV_DEVICE_KBM = 0,
    CADV_DEVICE_GAMEPAD = 1,
    CADV_DEVICE_TOUCH = 2
} cadv_device_kind;

/* USB HID usage codes for the default keyboard bindings. */
enum {
    CADV_KEY_A = 4, CADV_KEY_C = 6, CADV_KEY_D = 7, CADV_KEY_E = 8, CADV_KEY_F = 9,
    CADV_KEY_M = 16, CADV_KEY_Q = 20, CADV_KEY_R = 21, CADV_KEY_S = 22, CADV_KEY_W = 26,
    CADV_KEY_1 = 30, CADV_KEY_5 = 34, CADV_KEY_RETURN = 40, CADV_KEY_ESCAPE = 41,
    CADV_KEY_TAB = 43, CADV_KEY_SPACE = 44, CADV_KEY_RIGHT = 79, CADV_KEY_LEFT = 80,
    CADV_KEY_DOWN = 81, CADV_KEY_UP = 82, CADV_KEY_LCTRL = 224, CADV_KEY_LSHIFT = 225,
    CADV_KEY_COUNT = 256
};

typedef enum cadv_mouse_button { CADV_MOUSE_LEFT = 0, CADV_MOUSE_RIGHT = 1, CADV_MOUSE_MIDDLE = 2 } cadv_mouse_button;

typedef enum cadv_pad_button {
    CADV_PAD_SOUTH = 0, CADV_PAD_EAST, CADV_PAD_WEST, CADV_PAD_NORTH,
    CADV_PAD_BACK, CADV_PAD_START, CADV_PAD_LSHOULDER, CADV_PAD_RSHOULDER,
    CADV_PAD_LSTICK, CADV_PAD_DPAD_UP, CADV_PAD_DPAD_DOWN, CADV_PAD_LTRIGGER, CADV_PAD_RTRIGGER,
    CADV_PAD_BUTTON_COUNT
} cadv_pad_button;

typedef enum cadv_pad_axis { CADV_AXIS_LX = 0, CADV_AXIS_LY, CADV_AXIS_RX, CADV_AXIS_RY, CADV_AXIS_COUNT } cadv_pad_axis;

#define CADV_MAX_FINGERS 8
#define CADV_TOUCH_MAX_BUTTONS 8

typedef struct cadv_touch_button {
    float x, y, r;       /* centre and radius, normalised to screen height (x to width) */
    cadv_action action;
    const char *label;
    bool down;
} cadv_touch_button;

typedef struct cadv_finger {
    long long id;
    bool active;
    int role;            /* 0 stick, 1 look, 2 button */
    int button;
    float x0, y0, x, y;  /* normalised 0..1 */
    float lx, ly;        /* last position for look deltas */
} cadv_finger;

typedef struct cadv_input {
    /* bindings */
    int key_bind[CADV_KEY_COUNT];      /* action index or -1 */
    int mouse_bind[3];
    int pad_bind[CADV_PAD_BUTTON_COUNT];
    float mouse_sensitivity;           /* radians per pixel */
    float pad_look_speed;              /* radians per second at full deflection */
    float touch_look_speed;            /* radians per screen width */
    float deadzone;
    bool invert_y;
    /* raw state */
    bool key_down[CADV_KEY_COUNT];
    bool mouse_down[3];
    bool pad_down[CADV_PAD_BUTTON_COUNT];
    float pad_axis[CADV_AXIS_COUNT];
    float mouse_dx, mouse_dy;          /* accumulated since last frame */
    float touch_look_dx, touch_look_dy;
    cadv_finger fingers[CADV_MAX_FINGERS];
    cadv_touch_button touch_buttons[CADV_TOUCH_MAX_BUTTONS];
    int touch_button_count;
    float screen_aspect;               /* width / height for touch hit tests */
    uint32_t prev_held;
    cadv_device_kind last_device;
} cadv_input;

void cadv_input_init(cadv_input *in);
void cadv_input_bind_key(cadv_input *in, int key, cadv_action a);
void cadv_input_unbind_key(cadv_input *in, int key);

void cadv_input_key(cadv_input *in, int key, bool down);
void cadv_input_mouse_motion(cadv_input *in, float dx, float dy);
void cadv_input_mouse_button(cadv_input *in, int button, bool down);
void cadv_input_pad_button(cadv_input *in, int button, bool down);
void cadv_input_pad_axis(cadv_input *in, int axis, float value); /* -1..1 (triggers 0..1) */
/* Touch coordinates normalised to 0..1 across the screen. */
void cadv_input_finger_down(cadv_input *in, long long id, float x, float y);
void cadv_input_finger_motion(cadv_input *in, long long id, float x, float y);
void cadv_input_finger_up(cadv_input *in, long long id);
/* Clears held state (e.g. when the window loses focus). */
void cadv_input_release_all(cadv_input *in);

/* Default touch layout: jump, use, fire, radial on the right. */
void cadv_input_touch_default_layout(cadv_input *in);
bool cadv_input_touch_visible(const cadv_input *in);

/* Produces the frame for one simulation tick of length dt seconds. */
cadv_input_frame cadv_input_frame_build(cadv_input *in, float dt);

#endif

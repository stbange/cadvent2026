/* SPDX-License-Identifier: MIT */
#include "input/input.h"

#include <string.h>

#include "core/mathx.h"

bool cadv_input_down(const cadv_input_frame *f, cadv_action a) { return (f->held & CADV_ACT_BIT(a)) != 0; }
bool cadv_input_pressed(const cadv_input_frame *f, cadv_action a) { return (f->pressed & CADV_ACT_BIT(a)) != 0; }

void cadv_input_init(cadv_input *in)
{
    int i;
    memset(in, 0, sizeof *in);
    for (i = 0; i < CADV_KEY_COUNT; ++i) {
        in->key_bind[i] = -1;
    }
    cadv_input_bind_key(in, CADV_KEY_SPACE, CADV_ACT_JUMP);
    cadv_input_bind_key(in, CADV_KEY_C, CADV_ACT_CROUCH);
    cadv_input_bind_key(in, CADV_KEY_LCTRL, CADV_ACT_CROUCH);
    cadv_input_bind_key(in, CADV_KEY_E, CADV_ACT_USE);
    cadv_input_bind_key(in, CADV_KEY_F, CADV_ACT_USE);
    cadv_input_bind_key(in, CADV_KEY_RETURN, CADV_ACT_USE);
    cadv_input_bind_key(in, CADV_KEY_Q, CADV_ACT_RADIAL);
    cadv_input_bind_key(in, CADV_KEY_LSHIFT, CADV_ACT_RUN);
    cadv_input_bind_key(in, CADV_KEY_ESCAPE, CADV_ACT_PAUSE);
    cadv_input_bind_key(in, CADV_KEY_TAB, CADV_ACT_MAP);
    cadv_input_bind_key(in, CADV_KEY_M, CADV_ACT_MAP);
    cadv_input_bind_key(in, CADV_KEY_F1, CADV_ACT_HELP);
    cadv_input_bind_key(in, CADV_KEY_H, CADV_ACT_HELP);
    for (i = 0; i < 6; ++i) {
        cadv_input_bind_key(in, CADV_KEY_1 + i, (cadv_action)(CADV_ACT_WEAPON_1 + i));
    }
    in->mouse_bind[CADV_MOUSE_LEFT] = CADV_ACT_FIRE;
    in->mouse_bind[CADV_MOUSE_RIGHT] = CADV_ACT_ALT_FIRE;
    in->mouse_bind[CADV_MOUSE_MIDDLE] = CADV_ACT_RADIAL;
    for (i = 0; i < CADV_PAD_BUTTON_COUNT; ++i) {
        in->pad_bind[i] = -1;
    }
    in->pad_bind[CADV_PAD_SOUTH] = CADV_ACT_JUMP;
    in->pad_bind[CADV_PAD_EAST] = CADV_ACT_CROUCH;
    in->pad_bind[CADV_PAD_WEST] = CADV_ACT_USE;
    in->pad_bind[CADV_PAD_NORTH] = CADV_ACT_RADIAL;
    in->pad_bind[CADV_PAD_START] = CADV_ACT_PAUSE;
    in->pad_bind[CADV_PAD_BACK] = CADV_ACT_HELP;
    in->pad_bind[CADV_PAD_DPAD_UP] = CADV_ACT_MAP;
    in->pad_bind[CADV_PAD_LSHOULDER] = CADV_ACT_PREV_WEAPON;
    in->pad_bind[CADV_PAD_RSHOULDER] = CADV_ACT_NEXT_WEAPON;
    in->pad_bind[CADV_PAD_LSTICK] = CADV_ACT_RUN;
    in->pad_bind[CADV_PAD_LTRIGGER] = CADV_ACT_ALT_FIRE;
    in->pad_bind[CADV_PAD_RTRIGGER] = CADV_ACT_FIRE;
    in->mouse_sensitivity = 0.0025f;
    in->pad_look_speed = 3.0f;
    in->touch_look_speed = 4.0f;
    in->deadzone = 0.2f;
    in->screen_aspect = 16.0f / 9.0f;
    cadv_input_touch_default_layout(in);
}

void cadv_input_bind_key(cadv_input *in, int key, cadv_action a)
{
    if (key >= 0 && key < CADV_KEY_COUNT && a >= 0 && a < CADV_ACT_COUNT) {
        in->key_bind[key] = (int)a;
    }
}

void cadv_input_unbind_key(cadv_input *in, int key)
{
    if (key >= 0 && key < CADV_KEY_COUNT) {
        in->key_bind[key] = -1;
    }
}

void cadv_input_key(cadv_input *in, int key, bool down)
{
    if (key < 0 || key >= CADV_KEY_COUNT) {
        return;
    }
    in->key_down[key] = down;
    in->last_device = CADV_DEVICE_KBM;
}

void cadv_input_mouse_motion(cadv_input *in, float dx, float dy)
{
    in->mouse_dx += dx;
    in->mouse_dy += dy;
    in->last_device = CADV_DEVICE_KBM;
}

void cadv_input_mouse_button(cadv_input *in, int button, bool down)
{
    if (button < 0 || button > 2) {
        return;
    }
    in->mouse_down[button] = down;
    in->last_device = CADV_DEVICE_KBM;
}

void cadv_input_pad_button(cadv_input *in, int button, bool down)
{
    if (button < 0 || button >= CADV_PAD_BUTTON_COUNT) {
        return;
    }
    in->pad_down[button] = down;
    in->last_device = CADV_DEVICE_GAMEPAD;
}

void cadv_input_pad_axis(cadv_input *in, int axis, float value)
{
    if (axis < 0 || axis >= CADV_AXIS_COUNT) {
        return;
    }
    in->pad_axis[axis] = cadv_clampf(value, -1.0f, 1.0f);
    if (cadv_absf(value) > in->deadzone) {
        in->last_device = CADV_DEVICE_GAMEPAD;
    }
}

static int hit_button(const cadv_input *in, float x, float y)
{
    int i;
    for (i = 0; i < in->touch_button_count; ++i) {
        const cadv_touch_button *b = &in->touch_buttons[i];
        float dx = (x - b->x) * in->screen_aspect;
        float dy = y - b->y;
        if (dx * dx + dy * dy <= b->r * b->r) {
            return i;
        }
    }
    return -1;
}

static cadv_finger *find_finger(cadv_input *in, long long id)
{
    int i;
    for (i = 0; i < CADV_MAX_FINGERS; ++i) {
        if (in->fingers[i].active && in->fingers[i].id == id) {
            return &in->fingers[i];
        }
    }
    return NULL;
}

void cadv_input_finger_down(cadv_input *in, long long id, float x, float y)
{
    int i;
    int b;
    cadv_finger *f = NULL;
    in->last_device = CADV_DEVICE_TOUCH;
    for (i = 0; i < CADV_MAX_FINGERS && !f; ++i) {
        if (!in->fingers[i].active) {
            f = &in->fingers[i];
        }
    }
    if (!f) {
        return;
    }
    memset(f, 0, sizeof *f);
    f->active = true;
    f->id = id;
    f->x0 = f->x = f->lx = x;
    f->y0 = f->y = f->ly = y;
    b = hit_button(in, x, y);
    if (b >= 0) {
        f->role = 2;
        f->button = b;
        in->touch_buttons[b].down = true;
    } else {
        f->role = x < 0.5f ? 0 : 1;
    }
}

void cadv_input_finger_motion(cadv_input *in, long long id, float x, float y)
{
    cadv_finger *f = find_finger(in, id);
    if (!f) {
        return;
    }
    in->last_device = CADV_DEVICE_TOUCH;
    f->x = x;
    f->y = y;
    if (f->role == 1) {
        in->touch_look_dx += x - f->lx;
        in->touch_look_dy += y - f->ly;
    }
    f->lx = x;
    f->ly = y;
}

void cadv_input_finger_up(cadv_input *in, long long id)
{
    cadv_finger *f = find_finger(in, id);
    if (!f) {
        return;
    }
    if (f->role == 2) {
        in->touch_buttons[f->button].down = false;
    }
    f->active = false;
}

void cadv_input_release_all(cadv_input *in)
{
    int i;
    memset(in->key_down, 0, sizeof in->key_down);
    memset(in->mouse_down, 0, sizeof in->mouse_down);
    memset(in->pad_down, 0, sizeof in->pad_down);
    memset(in->pad_axis, 0, sizeof in->pad_axis);
    for (i = 0; i < CADV_MAX_FINGERS; ++i) {
        in->fingers[i].active = false;
    }
    for (i = 0; i < in->touch_button_count; ++i) {
        in->touch_buttons[i].down = false;
    }
    in->mouse_dx = in->mouse_dy = 0.0f;
    in->touch_look_dx = in->touch_look_dy = 0.0f;
}

static void add_touch_button(cadv_input *in, float x, float y, float r, cadv_action a, const char *label)
{
    cadv_touch_button *b = &in->touch_buttons[in->touch_button_count++];
    b->x = x;
    b->y = y;
    b->r = r;
    b->action = a;
    b->label = label;
    b->down = false;
}

void cadv_input_touch_default_layout(cadv_input *in)
{
    in->touch_button_count = 0;
    add_touch_button(in, 0.90f, 0.80f, 0.09f, CADV_ACT_JUMP, "JUMP");
    add_touch_button(in, 0.80f, 0.62f, 0.08f, CADV_ACT_USE, "USE");
    add_touch_button(in, 0.92f, 0.52f, 0.08f, CADV_ACT_FIRE, "FIRE");
    add_touch_button(in, 0.94f, 0.12f, 0.06f, CADV_ACT_PAUSE, "II");
    add_touch_button(in, 0.82f, 0.12f, 0.06f, CADV_ACT_HELP, "?");
}

bool cadv_input_touch_visible(const cadv_input *in) { return in->last_device == CADV_DEVICE_TOUCH; }

static float dead(float v, float dz)
{
    float a = cadv_absf(v);
    if (a <= dz) {
        return 0.0f;
    }
    return (v < 0.0f ? -1.0f : 1.0f) * (a - dz) / (1.0f - dz);
}

cadv_input_frame cadv_input_frame_build(cadv_input *in, float dt)
{
    cadv_input_frame f;
    int i;
    float kx = 0.0f;
    float ky = 0.0f;
    float ylook = in->invert_y ? -1.0f : 1.0f;
    memset(&f, 0, sizeof f);
    for (i = 0; i < CADV_KEY_COUNT; ++i) {
        if (in->key_down[i] && in->key_bind[i] >= 0) {
            f.held |= CADV_ACT_BIT(in->key_bind[i]);
        }
    }
    for (i = 0; i < 3; ++i) {
        if (in->mouse_down[i]) {
            f.held |= CADV_ACT_BIT(in->mouse_bind[i]);
        }
    }
    for (i = 0; i < CADV_PAD_BUTTON_COUNT; ++i) {
        if (in->pad_down[i] && in->pad_bind[i] >= 0) {
            f.held |= CADV_ACT_BIT(in->pad_bind[i]);
        }
    }
    for (i = 0; i < in->touch_button_count; ++i) {
        if (in->touch_buttons[i].down) {
            f.held |= CADV_ACT_BIT(in->touch_buttons[i].action);
        }
    }
    /* movement: WASD / arrows, left stick, touch stick */
    kx += (in->key_down[CADV_KEY_D] || in->key_down[CADV_KEY_RIGHT]) ? 1.0f : 0.0f;
    kx -= (in->key_down[CADV_KEY_A] || in->key_down[CADV_KEY_LEFT]) ? 1.0f : 0.0f;
    ky += (in->key_down[CADV_KEY_W] || in->key_down[CADV_KEY_UP]) ? 1.0f : 0.0f;
    ky -= (in->key_down[CADV_KEY_S] || in->key_down[CADV_KEY_DOWN]) ? 1.0f : 0.0f;
    kx += dead(in->pad_axis[CADV_AXIS_LX], in->deadzone);
    ky -= dead(in->pad_axis[CADV_AXIS_LY], in->deadzone);
    for (i = 0; i < CADV_MAX_FINGERS; ++i) {
        const cadv_finger *fg = &in->fingers[i];
        if (fg->active && fg->role == 0) {
            const float radius = 0.12f;
            kx += cadv_clampf((fg->x - fg->x0) * in->screen_aspect / radius, -1.0f, 1.0f);
            ky -= cadv_clampf((fg->y - fg->y0) / radius, -1.0f, 1.0f);
        }
    }
    {
        float len = cadv_sqrtf(kx * kx + ky * ky);
        if (len > 1.0f) {
            kx /= len;
            ky /= len;
        }
    }
    f.move_x = kx;
    f.move_y = ky;
    /* looking: positive look_x turns left (yaw grows counter-clockwise) */
    f.look_x = -in->mouse_dx * in->mouse_sensitivity;
    f.look_y = -in->mouse_dy * in->mouse_sensitivity * ylook;
    f.look_x -= dead(in->pad_axis[CADV_AXIS_RX], in->deadzone) * in->pad_look_speed * dt;
    f.look_y -= dead(in->pad_axis[CADV_AXIS_RY], in->deadzone) * in->pad_look_speed * dt * ylook;
    f.look_x -= in->touch_look_dx * in->touch_look_speed;
    f.look_y -= in->touch_look_dy * in->touch_look_speed / in->screen_aspect * ylook;
    in->mouse_dx = in->mouse_dy = 0.0f;
    in->touch_look_dx = in->touch_look_dy = 0.0f;
    f.pressed = f.held & ~in->prev_held;
    in->prev_held = f.held;
    return f;
}

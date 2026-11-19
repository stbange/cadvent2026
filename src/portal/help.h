/* SPDX-License-Identifier: MIT */
#ifndef CADV_PORTAL_HELP_H
#define CADV_PORTAL_HELP_H

#include <stdbool.h>

#include "gfx/scene.h"
#include "input/input.h"

/* On-screen help: the goal, the controls (with key names for the device in
 * use) and a few tips for the hall and every portal. Toggled with F1 / H,
 * the gamepad's Back button or the "?" touch button; the game pauses while it
 * is open. Drawing only writes UI quads, so it is tested without a GPU. */

#define CADV_HELP_MAX_CONTROLS 13
#define CADV_HELP_MAX_TIPS 4

typedef struct cadv_help_control {
    const char *keys;     /* keyboard & mouse, e.g. "W A S D" */
    const char *pad;      /* gamepad, e.g. "L-STICK" */
    const char *action;   /* e.g. "MOVE" */
} cadv_help_control;

typedef struct cadv_help {
    const char *title;
    const char *goal;
    cadv_help_control controls[CADV_HELP_MAX_CONTROLS];
    int control_count;
    const char *tips[CADV_HELP_MAX_TIPS];
    int tip_count;
} cadv_help;

/* Help for a day (1..24); day 0 is the hall. Days whose game has not shipped
 * get the practice range's help. */
const cadv_help *cadv_help_get(int day, bool implemented);
/* Name of the help key for a device ("F1", "BACK", "?"). */
const char *cadv_help_key(cadv_device_kind device);
/* Greedy word wrap into lines of at most 'width' characters; returns the
 * number of lines written (at most max_lines, the last one may be cut). */
int cadv_help_wrap(const char *text, int width, char lines[][64], int max_lines);
/* Draws the panel; 'open' is 0..1 for the opening slide, 'ticks' animates,
 * 'page' picks page 2 when everything does not fit. Returns the page count. */
int cadv_help_draw(cadv_ui *ui, const cadv_help *h, cadv_device_kind device, float open, int ticks, int page);
/* The small "F1 HELP" reminder shown after entering a world. */
void cadv_help_draw_hint(cadv_ui *ui, cadv_device_kind device, float alpha);
/* Touch buttons and the virtual stick (shared by the hall and portals). */
void cadv_help_draw_touch(cadv_ui *ui, const cadv_input *input);

#endif

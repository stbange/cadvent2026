/* SPDX-License-Identifier: MIT */
#ifndef CADV_APP_APP_H
#define CADV_APP_APP_H

#include <stdbool.h>

#include "core/args.h"
#include "gfx/renderer.h"

/* Platform-independent application state; the SDL layer only forwards
 * window size, events and buffer swaps. */
typedef struct cadv_app {
    cadv_args args;
    cadv_renderer renderer;
    int frame;
} cadv_app;

typedef enum cadv_app_status {
    CADV_APP_CONTINUE = 0,
    CADV_APP_DONE_OK = 1,
    CADV_APP_DONE_FAIL = 2
} cadv_app_status;

/* Requires a current GL context. */
bool cadv_app_init(cadv_app *app, const cadv_args *args, cadv_gl_profile profile);
/* Renders one frame into the back buffer (the caller swaps afterwards). */
cadv_app_status cadv_app_iterate(cadv_app *app, int width, int height);
void cadv_app_shutdown(cadv_app *app);

#endif

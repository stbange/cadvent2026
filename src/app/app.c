/* SPDX-License-Identifier: MIT */
#include "app/app.h"

#include <string.h>

#include "core/log.h"

bool cadv_app_init(cadv_app *app, const cadv_args *args, cadv_gl_profile profile)
{
    memset(app, 0, sizeof *app);
    app->args = *args;
    return cadv_renderer_init(&app->renderer, profile);
}

static cadv_app_status write_screenshot(cadv_app *app, int width, int height)
{
    cadv_image img;
    bool ok = cadv_renderer_capture(&app->renderer, width, height, &img) && /* GCOVR_EXCL_BR_LINE */
              cadv_image_write_ppm(&img, app->args.screenshot);
    cadv_image_free(&img);
    if (!ok) {
        CADV_ERROR("could not write screenshot '%s'", app->args.screenshot);
        return CADV_APP_DONE_FAIL;
    }
    CADV_INFO("screenshot written: %s (%dx%d)", app->args.screenshot, width, height);
    return CADV_APP_DONE_OK;
}

cadv_app_status cadv_app_iterate(cadv_app *app, int width, int height)
{
    cadv_renderer_frame(&app->renderer, width, height);
    ++app->frame;
    if (app->args.frames > 0 && app->frame >= app->args.frames) {
        if (app->args.screenshot) {
            return write_screenshot(app, width, height);
        }
        return CADV_APP_DONE_OK;
    }
    return CADV_APP_CONTINUE;
}

void cadv_app_shutdown(cadv_app *app)
{
    cadv_renderer_shutdown(&app->renderer);
    CADV_INFO("shutdown after %d frames", app->frame);
}

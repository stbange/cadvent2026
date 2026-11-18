/* SPDX-License-Identifier: MIT */
/* The only translation unit that talks to SDL. Uses SDL3's main-callback
 * model, which drives the frame loop natively on desktop and in browsers. */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
/* SDL_main.h defines WinMain on Windows without a prior prototype. */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#endif
#include <SDL3/SDL_main.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include <stdio.h>
#include <stdlib.h>

#include "app/app.h"
#include "core/args.h"
#include "core/log.h"
#include "core/version.h"
#include "gfx/gl_api.h"

typedef struct platform_state {
    SDL_Window *window;
    SDL_GLContext gl;
    cadv_app app;
    bool app_ready;
} platform_state;

static SDL_AppResult fail(const char *what)
{
    CADV_ERROR("%s: %s", what, SDL_GetError());
    return SDL_APP_FAILURE;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    cadv_args args;
    platform_state *ps;
    cadv_gl_profile profile = cadv_gl_native_profile();

    cadv_args_defaults(&args);
    switch (cadv_args_parse(&args, argc, argv)) {
    case CADV_ARGS_ERROR:
        fprintf(stderr, "cadvent: %s\n%s", args.error, cadv_args_usage());
        return SDL_APP_FAILURE;
    case CADV_ARGS_EXIT_OK:
        if (args.show_version) {
            printf("%s %s\n", CADV_NAME, CADV_VERSION);
        }
        if (args.show_help) {
            printf("%s", cadv_args_usage());
        }
        return SDL_APP_SUCCESS;
    default: break;
    }
    cadv_log_set_level(args.log_level);

    ps = (platform_state *)SDL_calloc(1, sizeof *ps);
    if (!ps) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return fail("out of memory"); /* GCOVR_EXCL_LINE */
    }
    *appstate = ps;
    SDL_SetAppMetadata(CADV_NAME, CADV_VERSION, "org.cadvent.cadvent2026");
    if (!SDL_Init(SDL_INIT_VIDEO)) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return fail("SDL_Init"); /* GCOVR_EXCL_LINE */
    }
    if (profile == CADV_GL_ES_30) { /* GCOVR_EXCL_BR_LINE: compile-time constant per build */
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    } else {
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    }
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    ps->window = SDL_CreateWindow(CADV_NAME, args.width, args.height,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!ps->window) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return fail("SDL_CreateWindow"); /* GCOVR_EXCL_LINE */
    }
    ps->gl = SDL_GL_CreateContext(ps->window);
    if (!ps->gl) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return fail("SDL_GL_CreateContext"); /* GCOVR_EXCL_LINE */
    }
#if !defined(__EMSCRIPTEN__)
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return fail("gladLoadGL"); /* GCOVR_EXCL_LINE */
    }
#endif
    /* Scripted runs (--frames) render as fast as possible. */
    SDL_GL_SetSwapInterval(args.frames > 0 ? 0 : 1);
    if (!cadv_app_init(&ps->app, &args, profile)) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        return fail("renderer init"); /* GCOVR_EXCL_LINE */
    }
    ps->app_ready = true;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    platform_state *ps = (platform_state *)appstate;
    int w = 0;
    int h = 0;
    cadv_app_status st;
    SDL_GetWindowSizeInPixels(ps->window, &w, &h);
    st = cadv_app_iterate(&ps->app, w, h);
    SDL_GL_SwapWindow(ps->window);
    if (st == CADV_APP_DONE_OK) {
        return SDL_APP_SUCCESS;
    }
    return st == CADV_APP_DONE_FAIL ? SDL_APP_FAILURE : SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    (void)appstate;
    /* GCOVR_EXCL_START: needs a human closing the window or pressing Escape */
    if (event->type == SDL_EVENT_QUIT ||
        (event->type == SDL_EVENT_KEY_DOWN && event->key.key == SDLK_ESCAPE)) {
        return SDL_APP_SUCCESS;
    }
    /* GCOVR_EXCL_STOP */
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    platform_state *ps = (platform_state *)appstate;
    (void)result;
    if (!ps) {
        return;
    }
    if (ps->app_ready) { /* GCOVR_EXCL_BR_LINE: allocation/driver failure */
        cadv_app_shutdown(&ps->app);
    }
    /* Both accept NULL (init may have stopped early). */
    SDL_GL_DestroyContext(ps->gl);
    SDL_DestroyWindow(ps->window);
    SDL_free(ps);
}

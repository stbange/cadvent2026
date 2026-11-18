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

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include "app/app.h"
#include "core/args.h"
#include "core/log.h"
#include "core/version.h"
#include "gfx/gl_api.h"

typedef struct platform_state {
    SDL_Window *window;
    SDL_GLContext gl;
    SDL_AudioStream *audio;
    SDL_Mutex *audio_lock;
    cadv_app app;
    cadv_args args;
    bool app_ready;
    bool headless;
    Uint64 last_ns;
    char data_dir[512];
    char save_dir[512];
} platform_state;

static SDL_AppResult fail(const char *what)
{
    CADV_ERROR("%s: %s", what, SDL_GetError());
    return SDL_APP_FAILURE;
}

static cadv_datetime local_now(void)
{
    SDL_Time t = 0;
    SDL_DateTime dt;
    cadv_datetime r = cadv_datetime_make(2026, 11, 18, 12, 0);
    if (SDL_GetCurrentTime(&t) && SDL_TimeToDateTime(t, &dt, true)) { /* GCOVR_EXCL_BR_LINE */
        r = cadv_datetime_make(dt.year, dt.month, dt.day, dt.hour, dt.minute);
    }
    return r;
}

static void find_data_dir(platform_state *ps)
{
    /* assets/ next to the executable (release), in the current directory, or
     * in the source tree (development builds). */
    const char *base = SDL_GetBasePath();
    const char *candidates[3];
    char probe[600];
    int i;
    /* CADV_STRICT_ASSETS=1 (release smoke tests): only next to the executable */
    const char *strict = SDL_getenv("CADV_STRICT_ASSETS");
    int count = strict && strict[0] == '1' ? 1 : 3;
    candidates[0] = base ? base : "";
    candidates[1] = "";
    candidates[2] = CADV_SOURCE_DIR "/";
    for (i = 0; i < count; ++i) {
        SDL_PathInfo info;
        snprintf(probe, sizeof probe, "%sassets/text/friends.cfg", candidates[i]);
        if (SDL_GetPathInfo(probe, &info)) {
            snprintf(ps->data_dir, sizeof ps->data_dir, "%s", candidates[i]);
            return;
        }
    }
    ps->data_dir[0] = '\0'; /* GCOVR_EXCL_LINE: the source tree is always present in tests */
}

static void SDLCALL audio_callback(void *user, SDL_AudioStream *stream, int additional, int total)
{
    /* GCOVR_EXCL_START: needs an audio device, not available in CI */
    platform_state *ps = (platform_state *)user;
    float buf[2048];
    (void)total;
    while (additional > 0) {
        int frames = additional / (int)(sizeof(float) * 2);
        if (frames > 1024) {
            frames = 1024;
        }
        if (frames <= 0) {
            break;
        }
        SDL_LockMutex(ps->audio_lock);
        cadv_app_mix_audio(&ps->app, buf, frames);
        SDL_UnlockMutex(ps->audio_lock);
        SDL_PutAudioStreamData(stream, buf, frames * (int)sizeof(float) * 2);
        additional -= frames * (int)sizeof(float) * 2;
    }
    /* GCOVR_EXCL_STOP */
}

static void open_audio(platform_state *ps)
{
    SDL_AudioSpec spec;
    spec.format = SDL_AUDIO_F32;
    spec.channels = 2;
    spec.freq = CADV_AUDIO_RATE;
    ps->audio_lock = SDL_CreateMutex();
    ps->audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, audio_callback, ps);
    if (ps->audio) { /* GCOVR_EXCL_BR_LINE: depends on the machine */
        SDL_ResumeAudioStreamDevice(ps->audio); /* GCOVR_EXCL_LINE */
    } else {
        CADV_INFO("no audio: %s", SDL_GetError());
    }
}

static bool start_app(platform_state *ps)
{
    bool ok;
    if (ps->audio_lock) {
        SDL_LockMutex(ps->audio_lock);
    }
    ok = cadv_app_init(&ps->app, &ps->args, !ps->headless, cadv_gl_native_profile(), ps->data_dir,
                       ps->save_dir, local_now());
    if (ps->audio_lock) {
        SDL_UnlockMutex(ps->audio_lock);
    }
    ps->app_ready = ok;
    return ok;
}

#if defined(__EMSCRIPTEN__)
/* Saves live in IndexedDB (IDBFS) so they survive closing the tab. */
EM_JS(void, web_mount_saves, (void), {
    Module.cadvSaveReady = 0;
    FS.mkdir("/save");
    FS.mount(IDBFS, {}, "/save");
    FS.syncfs(true, function(err) { Module.cadvSaveReady = 1; });
})
EM_JS(int, web_saves_ready, (void), { return Module.cadvSaveReady ? 1 : 0; })
EM_JS(void, web_sync_saves, (void), { FS.syncfs(false, function(err) {}); })
EM_JS(char *, web_query, (void), {
    var s = window.location.search || "";
    var n = lengthBytesUTF8(s) + 1;
    var p = _malloc(n);
    stringToUTF8(s, p, n);
    return p;
})
#endif

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    platform_state *ps;
    cadv_args args;
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
#if defined(__EMSCRIPTEN__)
    {
        char *q = web_query();
        cadv_args_apply_query(&args, q);
        free(q);
    }
#endif
    cadv_log_set_level(args.log_level);
    ps = (platform_state *)SDL_calloc(1, sizeof *ps);
    if (!ps) {                        /* GCOVR_EXCL_BR_LINE: allocation failure */
        return fail("out of memory"); /* GCOVR_EXCL_LINE */
    }
    *appstate = ps;
    ps->args = args;
    ps->headless = args.headless;
    SDL_SetAppMetadata(CADV_NAME, CADV_VERSION, "org.cadvent.cadvent2026");
    find_data_dir(ps);
    {
        char *pref = SDL_GetPrefPath("cadvent", "cadvent2026");
        snprintf(ps->save_dir, sizeof ps->save_dir, "%s", pref ? pref : "");
        SDL_free(pref);
    }
#if defined(__EMSCRIPTEN__)
    snprintf(ps->save_dir, sizeof ps->save_dir, "/save/");
    web_mount_saves();
#endif
    if (ps->headless) {
        return start_app(ps) ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
    }
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) { /* GCOVR_EXCL_BR_LINE */
        if (!SDL_Init(SDL_INIT_VIDEO)) {                                 /* GCOVR_EXCL_BR_LINE */
            return fail("SDL_Init");                                     /* GCOVR_EXCL_LINE */
        }
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
    ps->window = SDL_CreateWindow(CADV_NAME, args.width, args.height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!ps->window) {                   /* GCOVR_EXCL_BR_LINE */
        return fail("SDL_CreateWindow"); /* GCOVR_EXCL_LINE */
    }
    ps->gl = SDL_GL_CreateContext(ps->window);
    if (!ps->gl) {                           /* GCOVR_EXCL_BR_LINE */
        return fail("SDL_GL_CreateContext"); /* GCOVR_EXCL_LINE */
    }
#if !defined(__EMSCRIPTEN__)
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) { /* GCOVR_EXCL_BR_LINE */
        return fail("gladLoadGL");                          /* GCOVR_EXCL_LINE */
    }
#endif
    SDL_GL_SetSwapInterval(args.frames > 0 || args.script ? 0 : 1);
    open_audio(ps);
    ps->last_ns = SDL_GetTicksNS();
#if defined(__EMSCRIPTEN__)
    return SDL_APP_CONTINUE; /* app starts once the save storage is ready */
#else
    return start_app(ps) ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
#endif
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    platform_state *ps = (platform_state *)appstate;
    int w = 0;
    int h = 0;
    Uint64 now;
    double elapsed;
    cadv_app_status st;
    if (ps->headless) {
        st = cadv_app_run_headless(&ps->app);
        return st == CADV_APP_DONE_OK ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
    }
#if defined(__EMSCRIPTEN__)
    if (!ps->app_ready) {
        if (!web_saves_ready() || !start_app(ps)) {
            return SDL_APP_CONTINUE;
        }
    }
#endif
    now = SDL_GetTicksNS();
    elapsed = (double)(now - ps->last_ns) / 1e9;
    ps->last_ns = now;
    SDL_GetWindowSizeInPixels(ps->window, &w, &h);
    if (ps->audio_lock) {
        SDL_LockMutex(ps->audio_lock);
    }
    cadv_app_set_now(&ps->app, local_now());
    st = cadv_app_frame(&ps->app, elapsed, w, h);
    if (ps->audio_lock) {
        SDL_UnlockMutex(ps->audio_lock);
    }
    SDL_GL_SwapWindow(ps->window);
    if (st == CADV_APP_DONE_OK) {
        return SDL_APP_SUCCESS;
    }
    return st == CADV_APP_DONE_FAIL ? SDL_APP_FAILURE : SDL_APP_CONTINUE;
}

static int pad_button(int b)
{
    /* GCOVR_EXCL_START: needs a physical gamepad */
    switch (b) {
    case SDL_GAMEPAD_BUTTON_SOUTH: return CADV_PAD_SOUTH;
    case SDL_GAMEPAD_BUTTON_EAST: return CADV_PAD_EAST;
    case SDL_GAMEPAD_BUTTON_WEST: return CADV_PAD_WEST;
    case SDL_GAMEPAD_BUTTON_NORTH: return CADV_PAD_NORTH;
    case SDL_GAMEPAD_BUTTON_BACK: return CADV_PAD_BACK;
    case SDL_GAMEPAD_BUTTON_START: return CADV_PAD_START;
    case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: return CADV_PAD_LSHOULDER;
    case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: return CADV_PAD_RSHOULDER;
    case SDL_GAMEPAD_BUTTON_LEFT_STICK: return CADV_PAD_LSTICK;
    case SDL_GAMEPAD_BUTTON_DPAD_UP: return CADV_PAD_DPAD_UP;
    case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return CADV_PAD_DPAD_DOWN;
    default: return -1;
    }
    /* GCOVR_EXCL_STOP */
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *e)
{
    platform_state *ps = (platform_state *)appstate;
    cadv_input *in;
    /* GCOVR_EXCL_START: interactive input needs a human (logic is unit-tested in input.c) */
    if (!ps || !ps->app_ready) {
        return e->type == SDL_EVENT_QUIT ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
    }
    in = &ps->app.input;
    switch (e->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
        cadv_input_key(in, (int)e->key.scancode, e->type == SDL_EVENT_KEY_DOWN);
        if (e->type == SDL_EVENT_KEY_DOWN && e->key.scancode == SDL_SCANCODE_ESCAPE) {
            SDL_SetWindowRelativeMouseMode(ps->window, false);
        }
        break;
    case SDL_EVENT_MOUSE_MOTION:
        if (SDL_GetWindowRelativeMouseMode(ps->window)) {
            cadv_input_mouse_motion(in, e->motion.xrel, e->motion.yrel);
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (e->button.which == SDL_TOUCH_MOUSEID) {
            break; /* synthesized from touch */
        }
        if (e->type == SDL_EVENT_MOUSE_BUTTON_DOWN && !SDL_GetWindowRelativeMouseMode(ps->window)) {
            SDL_SetWindowRelativeMouseMode(ps->window, true);
            break;
        }
        cadv_input_mouse_button(in, e->button.button == SDL_BUTTON_LEFT ? 0 : (e->button.button == SDL_BUTTON_RIGHT ? 1 : 2),
                                e->type == SDL_EVENT_MOUSE_BUTTON_DOWN);
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        SDL_OpenGamepad(e->gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
    case SDL_EVENT_GAMEPAD_BUTTON_UP:
        cadv_input_pad_button(in, pad_button(e->gbutton.button), e->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
        break;
    case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
        float v = (float)e->gaxis.value / 32767.0f;
        switch (e->gaxis.axis) {
        case SDL_GAMEPAD_AXIS_LEFTX: cadv_input_pad_axis(in, CADV_AXIS_LX, v); break;
        case SDL_GAMEPAD_AXIS_LEFTY: cadv_input_pad_axis(in, CADV_AXIS_LY, v); break;
        case SDL_GAMEPAD_AXIS_RIGHTX: cadv_input_pad_axis(in, CADV_AXIS_RX, v); break;
        case SDL_GAMEPAD_AXIS_RIGHTY: cadv_input_pad_axis(in, CADV_AXIS_RY, v); break;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: cadv_input_pad_button(in, CADV_PAD_LTRIGGER, v > 0.5f); break;
        default: cadv_input_pad_button(in, CADV_PAD_RTRIGGER, v > 0.5f); break;
        }
        break;
    }
    case SDL_EVENT_FINGER_DOWN:
        cadv_input_finger_down(in, (long long)e->tfinger.fingerID, e->tfinger.x, e->tfinger.y);
        break;
    case SDL_EVENT_FINGER_MOTION:
        cadv_input_finger_motion(in, (long long)e->tfinger.fingerID, e->tfinger.x, e->tfinger.y);
        break;
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        cadv_input_finger_up(in, (long long)e->tfinger.fingerID);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        cadv_input_release_all(in);
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
    /* GCOVR_EXCL_STOP */
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    platform_state *ps = (platform_state *)appstate;
    (void)result;
    if (!ps) {
        return;
    }
    if (ps->audio) {                    /* GCOVR_EXCL_BR_LINE: machine dependent */
        SDL_DestroyAudioStream(ps->audio); /* GCOVR_EXCL_LINE */
    }
    if (ps->app_ready) {
        cadv_app_shutdown(&ps->app);
#if defined(__EMSCRIPTEN__)
        web_sync_saves();
#endif
    }
    if (ps->audio_lock) {
        SDL_DestroyMutex(ps->audio_lock);
    }
    SDL_GL_DestroyContext(ps->gl);
    SDL_DestroyWindow(ps->window);
    SDL_free(ps);
}

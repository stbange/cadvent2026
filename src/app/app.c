/* SPDX-License-Identifier: MIT */
#include "app/app.h"

#include <stdio.h>
#include <string.h>

#include "core/log.h"
#include "core/version.h"

/* ---- events (for scenario scripts) -------------------------------------- */

static void emit(cadv_app *app, const char *name)
{
    CADV_DEBUG("event: %s", name);
    if (app->event_count < CADV_APP_MAX_EVENTS) {
        snprintf(app->events[app->event_count++], sizeof app->events[0], "%s", name);
    }
}

/* Emits "<name>:<day>" in addition to the plain event name. */
static void emit_day(cadv_app *app, const char *name, int day)
{
    char buf[40];
    snprintf(buf, sizeof buf, "%s:%d", name, day);
    emit(app, buf);
}

bool cadv_app_event_happened(const cadv_app *app, const char *name)
{
    int i;
    for (i = 0; i < app->event_count; ++i) {
        if (strcmp(app->events[i], name) == 0) {
            return true;
        }
    }
    return false;
}

/* ---- persistence ------------------------------------------------------------ */

void cadv_app_save(cadv_app *app)
{
    static cadv_kvstore kv;
    cadv_kv_clear(&kv);
    cadv_progress_to_kv(&app->progress, &kv);
    if (app->args.unlock_all && !app->args.save_path) {
        return; /* never pollute the real save with the development override */
    }
    if (!cadv_kv_save_file(&kv, app->save_path, CADV_SAVE_MAGIC)) {
        CADV_WARN("could not write save file %s", app->save_path);
    }
}

static void load_save(cadv_app *app)
{
    static cadv_kvstore kv;
    if (cadv_kv_load_file(&kv, app->save_path, CADV_SAVE_MAGIC)) {
        cadv_progress_from_kv(&app->progress, &kv);
        CADV_INFO("save loaded: %s", app->save_path);
    } else {
        cadv_progress_reset(&app->progress);
    }
}

/* ---- portal sessions ---------------------------------------------------------- */

static void enter_portal(cadv_app *app, int day)
{
    cadv_portal_cfg cfg;
    const cadv_portal_desc *d = cadv_portal_get(day);
    memset(&cfg, 0, sizeof cfg);
    cfg.day = day;
    cfg.seed = (uint64_t)app->args.seed * 31u + (uint64_t)day;
    cfg.difficulty = CADV_DIFF_DEFAULT;
    cfg.checkpoint = app->progress.p[day - 1].checkpoint;
    cfg.audio = &app->audio;
    cfg.data_dir = app->data_dir;
    cfg.friends = app->friends.ptr;
    cfg.friend_count = app->friends.count;
    if (!cadv_session_start(&app->session, d, &cfg)) {
        CADV_ERROR("could not start portal %d", day); /* GCOVR_EXCL_LINE: init cannot fail today */
        return;                                         /* GCOVR_EXCL_LINE */
    }
    if (app->gpu) {
        const cadv_portal_view *v = cadv_portal_view_get(day);
        v->gpu_init(app->session.state);
        app->session.gpu_free = v->gpu_free;
    }
    app->mode = CADV_MODE_PORTAL;
    app->portal_day = day;
    app->progress.p[day - 1].entered = true;
    cadv_audio_sfx(&app->audio, CADV_SFX_PORTAL, 1.0f, 0.0f);
    emit(app, "portal_entered");
    emit_day(app, "portal_entered", day);
    CADV_INFO("entered portal %d: %s", day, d->title);
}

static void leave_portal(cadv_app *app, const cadv_portal_result *r)
{
    int day = app->portal_day;
    cadv_progress_record(&app->progress, day, r);
    if (r->outcome == CADV_OUTCOME_WON) {
        emit(app, "portal_won");
        emit_day(app, "portal_won", day);
    } else if (r->outcome == CADV_OUTCOME_LOST) {
        emit(app, "portal_lost");
    } else {
        emit(app, "portal_quit");
    }
    if (r->star_collected) {
        emit(app, "star_collected");
        emit_day(app, "star_collected", day);
    }
    cadv_session_end(&app->session);
    app->mode = CADV_MODE_HUB;
    app->portal_day = 0;
    cadv_hub_return_to(&app->hub, day);
    cadv_app_save(app);
    CADV_INFO("left portal %d (outcome %d, snowflakes %d, star %d)", day, (int)r->outcome, r->snowflakes,
              r->star_collected ? 1 : 0);
}

/* ---- script host ------------------------------------------------------------------ */

static bool host_query(void *user, const char *key, double *out) { return cadv_app_query((cadv_app *)user, key, out); }

static void host_teleport(void *user, float x, float y, float z, float yaw)
{
    cadv_app *app = (cadv_app *)user;
    if (app->mode == CADV_MODE_HUB) {
        cadv_hub_teleport(&app->hub, x, y, z, yaw);
    } else {
        app->session.desc->teleport(app->session.state, x, y, z, yaw);
    }
}

static void host_bot(void *user, cadv_input_frame *out)
{
    cadv_app *app = (cadv_app *)user;
    if (app->mode == CADV_MODE_HUB) {
        cadv_hub_bot(&app->hub, out);
    } else {
        app->session.desc->bot(app->session.state, out);
    }
}

static bool host_event(void *user, const char *name) { return cadv_app_event_happened((cadv_app *)user, name); }

static bool host_screenshot(void *user, const char *path)
{
    cadv_app *app = (cadv_app *)user;
    if (!app->gpu) {
        CADV_WARN("screenshot '%s' skipped: no rendering in headless mode", path);
        return true;
    }
    app->screenshot_pending = true;
    snprintf(app->screenshot_path, sizeof app->screenshot_path, "%s", path);
    return true;
}

static void host_log(void *user, const char *text)
{
    (void)user;
    CADV_INFO("script: %s", text);
}

/* ---- init / queries ----------------------------------------------------------------- */

bool cadv_app_init(cadv_app *app, const cadv_args *args, bool gpu, cadv_gl_profile profile,
                   const char *data_dir, const char *save_dir, cadv_datetime now)
{
    char path[512];
    memset(app, 0, sizeof *app);
    app->args = *args;
    app->gpu = gpu;
    app->profile = profile;
    app->now = args->has_date ? args->date : now;
    snprintf(app->data_dir, sizeof app->data_dir, "%s", data_dir);
    cadv_clock_init(&app->clock);
    cadv_input_init(&app->input);
    cadv_audio_init(&app->audio);
    snprintf(path, sizeof path, "%sassets/text/friends.cfg", data_dir);
    if (!cadv_friends_load(&app->friends, path)) {
        CADV_INFO("friends list not found at %s, using defaults", path);
    }
    if (args->save_path) {
        snprintf(app->save_path, sizeof app->save_path, "%s", args->save_path);
    } else {
        snprintf(app->save_path, sizeof app->save_path, "%scadvent-save.txt", save_dir);
    }
    load_save(app);
    cadv_hub_init(&app->hub, &app->progress, &app->friends, app->now, CADV_SEASON_YEAR, args->unlock_all);
    if (args->script) {
        if (!cadv_script_load(&app->script, args->script)) {
            CADV_ERROR("script error: %s", app->script.error);
            return false;
        }
        app->has_script = true;
    }
    if (gpu) {
        if (!cadv_renderer_init(&app->renderer, profile, args->internal_height)) { /* GCOVR_EXCL_BR_LINE */
            return false;                                                         /* GCOVR_EXCL_LINE */
        }
        cadv_hub_gfx_init(&app->hub_gfx, &app->hub);
        cadv_audio_music(&app->audio, true, 0.8f);
    }
    if (args->portal >= 1) {
        enter_portal(app, args->portal);
    }
    CADV_INFO("%s %s ready (%s, %04d-%02d-%02d)", CADV_NAME, CADV_VERSION, gpu ? "rendering" : "headless",
              app->now.year, app->now.month, app->now.day);
    return true;
}

void cadv_app_set_now(cadv_app *app, cadv_datetime now)
{
    if (app->args.has_date) {
        return;
    }
    if (now.day != app->now.day || now.month != app->now.month || now.hour != app->now.hour ||
        now.minute != app->now.minute) {
        app->now = now;
        cadv_hub_set_time(&app->hub, now);
    }
}

static bool progress_query(const cadv_app *app, const char *key, double *out)
{
    int d;
    if (sscanf(key, "progress.star.%d", &d) == 1 && d >= 1 && d <= CADV_PORTAL_COUNT) {
        *out = app->progress.p[d - 1].star_collected;
    } else if (sscanf(key, "progress.socket.%d", &d) == 1 && d >= 1 && d <= CADV_PORTAL_COUNT) {
        *out = app->progress.p[d - 1].star_socketed;
    } else if (sscanf(key, "progress.snowflakes.%d", &d) == 1 && d >= 1 && d <= CADV_PORTAL_COUNT) {
        *out = app->progress.p[d - 1].snowflakes;
    } else if (sscanf(key, "progress.completed.%d", &d) == 1 && d >= 1 && d <= CADV_PORTAL_COUNT) {
        *out = app->progress.p[d - 1].completed;
    } else if (sscanf(key, "progress.checkpoint.%d", &d) == 1 && d >= 1 && d <= CADV_PORTAL_COUNT) {
        *out = app->progress.p[d - 1].checkpoint;
    } else if (sscanf(key, "progress.marks.%d", &d) == 1 && d >= 1 && d <= CADV_PORTAL_COUNT) {
        *out = app->progress.p[d - 1].marks_found;
    } else {
        return false;
    }
    return true;
}

bool cadv_app_query(cadv_app *app, const char *key, double *out)
{
    if (strcmp(key, "mode") == 0) {
        *out = (double)app->mode;
        return true;
    }
    if (strcmp(key, "day") == 0) {
        *out = app->portal_day;
        return true;
    }
    if (strcmp(key, "ticks") == 0) {
        *out = (double)app->ticks;
        return true;
    }
    if (strcmp(key, "arena.peak") == 0) {
        *out = (double)app->session.arena_peak;
        return true;
    }
    if (strncmp(key, "hub.", 4) == 0) {
        return cadv_hub_query(&app->hub, key + 4, out);
    }
    if (strncmp(key, "portal.", 7) == 0) {
        return app->mode == CADV_MODE_PORTAL && app->session.desc->query(app->session.state, key + 7, out);
    }
    if (strncmp(key, "progress.", 9) == 0) {
        return progress_query(app, key, out);
    }
    if (app->mode == CADV_MODE_PORTAL) {
        return app->session.desc->query(app->session.state, key, out);
    }
    return cadv_hub_query(&app->hub, key, out);
}

/* ---- ticking ------------------------------------------------------------------------ */

static void handle_hub_events(cadv_app *app, uint32_t ev)
{
    if (ev & CADV_HUB_EV_STEP) {
        cadv_audio_sfx(&app->audio, CADV_SFX_STEP, 0.6f, 0.0f);
    }
    if (ev & CADV_HUB_EV_JUMP) {
        cadv_audio_sfx(&app->audio, CADV_SFX_JUMP, 0.5f, 0.0f);
    }
    if (ev & CADV_HUB_EV_LANDED) {
        cadv_audio_sfx(&app->audio, CADV_SFX_LAND, 0.6f, 0.0f);
    }
    if (ev & CADV_HUB_EV_REVEAL) {
        cadv_audio_sfx(&app->audio, CADV_SFX_BELL, 1.0f, 0.0f);
        emit(app, "reveal");
    }
    if (ev & CADV_HUB_EV_DENIED) {
        cadv_audio_sfx(&app->audio, CADV_SFX_DENIED, 1.0f, 0.0f);
        emit(app, "denied");
    }
    if (ev & CADV_HUB_EV_SOCKETED) {
        cadv_audio_sfx(&app->audio, CADV_SFX_SOCKET, 1.0f, 0.0f);
        emit(app, "star_socketed");
    }
    if (ev & CADV_HUB_EV_CROWNED) {
        cadv_audio_sfx(&app->audio, CADV_SFX_STAR, 1.0f, 0.0f);
        emit(app, "tree_crowned");
        emit(app, "epilogue");
    }
    if (ev & CADV_HUB_EV_EPILOGUE_END) {
        emit(app, "epilogue_end");
    }
    if (ev & CADV_HUB_EV_SAVE) {
        cadv_app_save(app);
    }
    if (ev & CADV_HUB_EV_ENTER) {
        enter_portal(app, app->hub.enter_day);
    }
}

void cadv_app_tick(cadv_app *app)
{
    cadv_input_frame in;
    cadv_script_host host;
    if (app->has_script && !app->script.done) {
        host.user = app;
        host.query = host_query;
        host.teleport = host_teleport;
        host.bot = host_bot;
        host.event = host_event;
        host.screenshot = host_screenshot;
        host.log = host_log;
        cadv_script_step(&app->script, &host, &in);
        if (app->script.failed) {
            CADV_ERROR("script failed: %s", app->script.error);
        }
    } else {
        in = cadv_input_frame_build(&app->input, (float)CADV_TICK_DT);
    }
    ++app->ticks;
    if (app->mode == CADV_MODE_HUB) {
        handle_hub_events(app, cadv_hub_tick(&app->hub, &in));
    } else {
        cadv_portal_result r;
        cadv_session_tick(&app->session, &in);
        r = cadv_session_result(&app->session);
        if (r.outcome != CADV_OUTCOME_RUNNING) {
            leave_portal(app, &r);
        }
    }
}

cadv_app_status cadv_app_script_status(const cadv_app *app)
{
    if (!app->has_script || !app->script.done) {
        return CADV_APP_CONTINUE;
    }
    return app->script.failed ? CADV_APP_DONE_FAIL : CADV_APP_DONE_OK;
}

cadv_app_status cadv_app_run_headless(cadv_app *app)
{
    unsigned long long limit = app->args.max_ticks > 0 ? (unsigned long long)app->args.max_ticks
                                                       : (app->has_script ? 60ull * 60ull * 60ull : (unsigned long long)app->args.frames);
    while (app->ticks < limit) {
        cadv_app_status st;
        cadv_app_tick(app);
        st = cadv_app_script_status(app);
        if (st != CADV_APP_CONTINUE) {
            CADV_INFO("script finished after %llu ticks: %s", app->ticks, st == CADV_APP_DONE_OK ? "ok" : "FAILED");
            return st;
        }
    }
    if (app->has_script) {
        CADV_ERROR("tick limit %llu reached before the script finished", limit);
        return CADV_APP_DONE_FAIL;
    }
    return CADV_APP_DONE_OK;
}

/* ---- rendering ------------------------------------------------------------------------- */

static bool write_screenshot(cadv_app *app, const char *path, int width, int height)
{
    cadv_image img;
    bool ok = cadv_renderer_capture(&app->renderer, width, height, &img) && cadv_image_write_ppm(&img, path);
    cadv_image_free(&img);
    if (!ok) {
        CADV_ERROR("could not write screenshot '%s'", path);
    } else {
        CADV_INFO("screenshot written: %s (%dx%d)", path, width, height);
    }
    return ok;
}

static void render(cadv_app *app, int width, int height, float alpha)
{
    cadv_render_view view;
    int tw;
    int th;
    view.viewport.x = 0;
    view.viewport.y = 0;
    view.viewport.w = width;
    view.viewport.h = height;
    cadv_renderer_target_size(&app->renderer, view.viewport, &tw, &th);
    cadv_scene_reset(&app->scene);
    cadv_ui_reset(&app->ui, tw, th);
    memset(&app->cam, 0, sizeof app->cam);
    app->input.screen_aspect = height > 0 ? (float)width / (float)height : 1.0f;
    if (app->mode == CADV_MODE_HUB) {
        cadv_hub_gfx_draw(&app->hub_gfx, &app->hub, &app->input, &app->scene, &app->cam, &app->ui, alpha,
                          app->args.unlock_all);
    } else {
        cadv_portal_view_get(app->portal_day)->draw(app->session.state, &app->scene, &app->cam, &app->ui, alpha);
    }
    view.camera = &app->cam;
    view.scene = &app->scene;
    view.ui = &app->ui;
    cadv_renderer_render(&app->renderer, width, height, &view, 1);
}

cadv_app_status cadv_app_frame(cadv_app *app, double elapsed, int width, int height)
{
    int n;
    int i;
    cadv_app_status st = CADV_APP_CONTINUE;
    bool scripted = app->has_script || app->args.frames > 0;
    app->width = width;
    app->height = height;
    /* scripted runs advance exactly one tick per frame so they are reproducible */
    n = scripted ? 1 : cadv_clock_advance(&app->clock, elapsed, 1.0);
    for (i = 0; i < n; ++i) {
        cadv_app_tick(app);
    }
    if (cadv_input_touch_visible(&app->input) != app->touch_visible) {
        app->touch_visible = !app->touch_visible;
        CADV_INFO("touch controls %s", app->touch_visible ? "shown" : "hidden"); /* GCOVR_EXCL_LINE: needs a touch screen */
    }
    render(app, width, height, scripted ? 0.0f : cadv_clock_alpha(&app->clock));
    ++app->frame;
    if (app->screenshot_pending) {
        app->screenshot_pending = false;
        if (!write_screenshot(app, app->screenshot_path, width, height)) {
            return CADV_APP_DONE_FAIL;
        }
    }
    st = cadv_app_script_status(app);
    if (st != CADV_APP_CONTINUE) {
        return st;
    }
    if (app->args.frames > 0 && app->frame >= app->args.frames && !app->has_script) {
        if (app->args.screenshot && !write_screenshot(app, app->args.screenshot, width, height)) {
            return CADV_APP_DONE_FAIL;
        }
        return CADV_APP_DONE_OK;
    }
    return CADV_APP_CONTINUE;
}

void cadv_app_mix_audio(cadv_app *app, float *out, int frames) { cadv_audio_mix(&app->audio, out, frames); }

void cadv_app_shutdown(cadv_app *app)
{
    if (cadv_session_active(&app->session)) {
        cadv_portal_result r = cadv_session_result(&app->session);
        cadv_progress_record(&app->progress, app->portal_day, &r); /* keep checkpoints */
        cadv_session_end(&app->session);
    }
    cadv_app_save(app);
    if (app->gpu) {
        cadv_hub_gfx_free(&app->hub_gfx);
        cadv_renderer_shutdown(&app->renderer);
    }
    CADV_INFO("shutdown after %d frames, %llu ticks", app->frame, app->ticks);
}

/* SPDX-License-Identifier: MIT */
#include "hub/hub.h"

#include <stdio.h>
#include <string.h>

#include "hub/layout.h"
#include "portal/portal.h"

#define DT (1.0f / 60.0f)
#define REVEAL_TICKS 90

const char *cadv_portal_state_name(cadv_portal_state s)
{
    static const char *const names[] = {"sealed", "still thawing", "stars missing", "thawing", "open", "mastered"};
    return names[s];
}

void cadv_hub_update_states(cadv_hub *h)
{
    int day;
    bool arch = cadv_progress_arch_complete(h->progress);
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        const cadv_portal_progress *p = &h->progress->p[day - 1];
        const cadv_portal_desc *d = cadv_portal_get(day);
        cadv_portal_access a = cadv_calendar_access(day, &h->now, h->season_year, arch, h->unlock_all);
        cadv_portal_state st;
        if (a == CADV_ACCESS_SEALED_DATE) {
            st = CADV_PS_SEALED;
        } else if (a == CADV_ACCESS_SEALED_STARS) {
            st = CADV_PS_STARS_MISSING;
        } else if (!d->implemented && !h->unlock_all) {
            st = CADV_PS_STILL_THAWING;
        } else if (p->snowflakes >= 3) {
            st = CADV_PS_MASTERED;
        } else if (p->entered) {
            st = CADV_PS_OPEN;
        } else {
            st = CADV_PS_THAWING;
        }
        h->state[day - 1] = st;
    }
}

void cadv_hub_init(cadv_hub *h, cadv_progress *progress, const cadv_friends *friends,
                   cadv_datetime now, int season_year, bool unlock_all)
{
    memset(h, 0, sizeof *h);
    h->progress = progress;
    h->friends = friends;
    h->now = now;
    h->season_year = season_year;
    h->unlock_all = unlock_all;
    h->focus_legend = -1;
    if (unlock_all) {
        cadv_progress_dev_fill(progress);
    }
    cadv_hall_build_world(&h->world);
    cadv_player_spawn(&h->player, cadv_hall_spawn(), cadv_hall_spawn_yaw());
    cadv_hub_update_states(h);
}

void cadv_hub_return_to(cadv_hub *h, int day)
{
    cadv_portal_slot s = cadv_hall_portal(day);
    cadv_vec3 p = cadv_hall_portal_front(day, 2.0f);
    cadv_player_spawn(&h->player, p, s.yaw + CADV_PI);
    h->player.yaw = cadv_wrap_angle(s.yaw + CADV_PI); /* look away from the portal, into the hall */
    h->cooldown = 30;
    cadv_hub_update_states(h);
}

void cadv_hub_set_time(cadv_hub *h, cadv_datetime now)
{
    h->now = now;
    cadv_hub_update_states(h);
}

static void say(cadv_hub *h, const char *text)
{
    snprintf(h->message, sizeof h->message, "%s", text);
    h->message_timer = 180;
}

static bool looking_at(const cadv_hub *h, cadv_vec3 target, float max_dist, float min_dot)
{
    cadv_vec3 eye = cadv_player_eye(&h->player);
    cadv_vec3 to = cadv_v3_sub(target, eye);
    float d = cadv_v3_len(to);
    if (d > max_dist || d < 1e-4f) {
        return false;
    }
    return cadv_v3_dot(cadv_v3_scale(to, 1.0f / d), cadv_player_forward(&h->player)) >= min_dot;
}

static void describe_portal(cadv_hub *h, int day, char *out, size_t cap)
{
    const cadv_portal_desc *d = cadv_portal_get(day);
    const cadv_portal_progress *p = &h->progress->p[day - 1];
    char cd[40];
    switch (h->state[day - 1]) {
    case CADV_PS_SEALED:
        cadv_calendar_countdown_text(cadv_calendar_minutes_until(day, &h->now, h->season_year), cd, sizeof cd);
        snprintf(out, cap, "DAY %d - SEALED - %s", day, cd);
        break;
    case CADV_PS_STILL_THAWING:
        snprintf(out, cap, "DAY %d - STILL THAWING... COME BACK SOON", day);
        break;
    case CADV_PS_STARS_MISSING:
        snprintf(out, cap, "DAY 24 - SET ALL 23 SHINING STARS INTO THE ARCH");
        break;
    default:
        snprintf(out, cap, "DAY %d - %s  %s%s", day, d->title,
                 p->snowflakes >= 1 ? (p->snowflakes >= 2 ? (p->snowflakes >= 3 ? "***" : "**") : "*") : "",
                 p->star_collected ? "  [STAR]" : (p->completed ? "  [STAR MISSING]" : ""));
        break;
    }
}

static uint32_t try_enter(cadv_hub *h, int day)
{
    cadv_portal_state st = h->state[day - 1];
    char buf[96];
    if (h->cooldown > 0) {
        return 0;
    }
    h->cooldown = 45;
    if (st == CADV_PS_SEALED || st == CADV_PS_STILL_THAWING || st == CADV_PS_STARS_MISSING) {
        if (st == CADV_PS_STARS_MISSING) {
            int missing[23];
            int n = cadv_progress_missing_sockets(h->progress, missing, 23);
            snprintf(buf, sizeof buf, "THE ARCH NEEDS %d MORE STAR%s", n, n == 1 ? "" : "S");
            say(h, buf);
        } else {
            describe_portal(h, day, buf, sizeof buf);
            say(h, buf);
        }
        /* bounce back into the hall */
        h->player.vel = cadv_v3_scale(cadv_hall_portal(day).normal, 4.0f);
        return CADV_HUB_EV_DENIED;
    }
    h->enter_day = day;
    return CADV_HUB_EV_ENTER;
}

static uint32_t use_arch(cadv_hub *h)
{
    char buf[96];
    int n = cadv_progress_socket_all(h->progress);
    int missing[23];
    int m = cadv_progress_missing_sockets(h->progress, missing, 23);
    h->last_socketed = n;
    cadv_hub_update_states(h);
    if (n > 0) {
        snprintf(buf, sizeof buf, "%d SHINING STAR%s SET - %d/23", n, n == 1 ? "" : "S", 23 - m);
        say(h, buf);
        return CADV_HUB_EV_SOCKETED | CADV_HUB_EV_SAVE;
    }
    if (m == 0) {
        say(h, "THE STAR ARCH IS COMPLETE");
    } else {
        int i;
        int len = snprintf(buf, sizeof buf, "MISSING:");
        for (i = 0; i < m && len < (int)sizeof buf - 4; ++i) {
            len += snprintf(buf + len, sizeof buf - (size_t)len, " %d", missing[i]);
        }
        say(h, buf);
    }
    return 0;
}

static uint32_t use_tree(cadv_hub *h)
{
    if (cadv_progress_can_crown(h->progress)) {
        h->progress->tree_crowned = true;
        h->epilogue_timer = CADV_HUB_EPILOGUE_TICKS;
        say(h, "THE TREE IS CROWNED - MERRY CHRISTMAS!");
        return CADV_HUB_EV_CROWNED | CADV_HUB_EV_SAVE;
    }
    if (h->progress->tree_crowned) {
        h->epilogue_timer = CADV_HUB_EPILOGUE_TICKS; /* replay */
        return 0;
    }
    say(h, "THE TREE WAITS FOR THE 24TH STAR");
    return 0;
}

static void update_focus(cadv_hub *h)
{
    int day;
    int i;
    float best = 1e9f;
    h->focus_day = 0;
    h->focus_legend = -1;
    h->focus_arch = false;
    h->focus_tree = false;
    h->prompt[0] = '\0';
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        cadv_portal_slot s = cadv_hall_portal(day);
        cadv_vec3 c = cadv_v3_add(s.base, cadv_v3(0, 1.6f, 0));
        float d = cadv_v3_dist(c, cadv_player_eye(&h->player));
        if (d < 6.0f && d < best && looking_at(h, c, 6.0f, 0.75f)) {
            best = d;
            h->focus_day = day;
        }
    }
    for (i = 0; i < 6; ++i) {
        if (looking_at(h, cadv_hall_legend_plaque(i), 3.5f, 0.95f)) {
            h->focus_legend = i;
        }
    }
    if (cadv_v3_dist(h->player.pos, cadv_hall_arch_use_point()) < 3.5f) {
        h->focus_arch = true;
    }
    if (cadv_v3_dist(h->player.pos, cadv_hall_tree()) < 4.5f) {
        h->focus_tree = true;
    }
    if (h->focus_legend >= 0 && h->focus_legend < h->friends->count) {
        int found;
        int total;
        cadv_friends_tally(h->progress, h->focus_legend, &found, &total);
        snprintf(h->prompt, sizeof h->prompt, "WALL OF LEGENDS: %s - %d/%d MARKS FOUND",
                 h->friends->name[h->focus_legend], found, total);
    } else if (h->focus_arch) {
        int carried = cadv_progress_stars_carried(h->progress);
        snprintf(h->prompt, sizeof h->prompt, "STAR ARCH %d/23 - USE TO SET %d STAR%s",
                 cadv_progress_stars_socketed(h->progress), carried, carried == 1 ? "" : "S");
    } else if (h->focus_tree) {
        snprintf(h->prompt, sizeof h->prompt, "%s",
                 cadv_progress_can_crown(h->progress) ? "USE TO CROWN THE TREE"
                 : h->progress->tree_crowned ? "USE TO REPLAY THE EPILOGUE" : "THE GREAT FIR");
    } else if (h->focus_day > 0) {
        describe_portal(h, h->focus_day, h->prompt, sizeof h->prompt);
    }
}

static uint32_t check_portal_contact(cadv_hub *h)
{
    int day;
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        cadv_portal_slot s = cadv_hall_portal(day);
        cadv_vec3 rel = cadv_v3_sub(h->player.pos, s.base);
        float along = cadv_v3_dot(rel, s.normal);       /* distance in front of the wall */
        float side = s.normal.x != 0.0f ? rel.z : rel.x; /* offset along the wall */
        if (along < CADV_PLAYER_RADIUS + 0.15f && along > -0.2f &&
            side > -0.5f * CADV_ARCH_WIDTH + 0.2f && side < 0.5f * CADV_ARCH_WIDTH - 0.2f &&
            rel.y > -0.3f && rel.y < 1.0f) {
            return try_enter(h, day);
        }
    }
    return 0;
}

static uint32_t check_reveals(cadv_hub *h)
{
    uint32_t ev = 0;
    int day;
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        cadv_portal_slot s = cadv_hall_portal(day);
        cadv_vec3 c = cadv_v3_add(s.base, cadv_v3(0, 1.6f, 0));
        if (h->reveal_timer[day - 1] > 0) {
            if (--h->reveal_timer[day - 1] == 0) {
                h->progress->p[day - 1].entered = true; /* reveal seen: never again */
                ev |= CADV_HUB_EV_SAVE;
                cadv_hub_update_states(h);
            }
            continue;
        }
        if (h->state[day - 1] == CADV_PS_THAWING && looking_at(h, c, 18.0f, 0.85f) &&
            cadv_world_line_of_sight(&h->world, cadv_player_eye(&h->player),
                                     cadv_v3_add(c, cadv_v3_scale(s.normal, 0.8f)))) {
            h->reveal_timer[day - 1] = REVEAL_TICKS;
            ev |= CADV_HUB_EV_REVEAL;
        }
    }
    return ev;
}

uint32_t cadv_hub_tick(cadv_hub *h, const cadv_input_frame *in)
{
    uint32_t ev = 0;
    uint32_t pev;
    ++h->ticks;
    if (h->message_timer > 0) {
        --h->message_timer;
    }
    if (h->cooldown > 0) {
        --h->cooldown;
    }
    if (h->epilogue_timer > 0) {
        if (--h->epilogue_timer == 0 || (h->epilogue_timer < CADV_HUB_EPILOGUE_TICKS - 120 &&
                                         cadv_input_pressed(in, CADV_ACT_USE))) {
            h->epilogue_timer = 0;
            if (!h->progress->epilogue_seen) {
                h->progress->epilogue_seen = true;
                ev |= CADV_HUB_EV_SAVE;
            }
            ev |= CADV_HUB_EV_EPILOGUE_END;
        }
        return ev;
    }
    pev = cadv_player_tick(&h->player, &h->world, in, DT);
    if (pev & CADV_PEV_STEP) {
        ev |= CADV_HUB_EV_STEP;
    }
    if (pev & CADV_PEV_JUMP) {
        ev |= CADV_HUB_EV_JUMP;
    }
    if (pev & CADV_PEV_LAND) {
        ev |= CADV_HUB_EV_LANDED;
    }
    update_focus(h);
    if (cadv_input_pressed(in, CADV_ACT_USE)) {
        if (h->focus_arch) {
            ev |= use_arch(h);
        } else if (h->focus_tree) {
            ev |= use_tree(h);
        } else if (h->focus_day > 0) {
            describe_portal(h, h->focus_day, h->message, sizeof h->message);
            h->message_timer = 180;
        }
    }
    ev |= check_reveals(h);
    ev |= check_portal_contact(h);
    return ev;
}

bool cadv_hub_query(const cadv_hub *h, const char *key, double *out)
{
    int day;
    if (strcmp(key, "player.x") == 0) {
        *out = h->player.pos.x;
    } else if (strcmp(key, "player.y") == 0) {
        *out = h->player.pos.y;
    } else if (strcmp(key, "player.z") == 0) {
        *out = h->player.pos.z;
    } else if (strcmp(key, "player.yaw") == 0) {
        *out = h->player.yaw;
    } else if (strcmp(key, "stars_carried") == 0) {
        *out = cadv_progress_stars_carried(h->progress);
    } else if (strcmp(key, "stars_socketed") == 0) {
        *out = cadv_progress_stars_socketed(h->progress);
    } else if (strcmp(key, "snowflakes") == 0) {
        *out = cadv_progress_total_snowflakes(h->progress);
    } else if (strcmp(key, "candles") == 0) {
        *out = cadv_progress_candles(h->progress);
    } else if (strcmp(key, "crowned") == 0) {
        *out = h->progress->tree_crowned ? 1.0 : 0.0;
    } else if (strcmp(key, "epilogue") == 0) {
        *out = h->epilogue_timer > 0 ? 1.0 : 0.0;
    } else if (strcmp(key, "focus") == 0) {
        *out = h->focus_day;
    } else if (strcmp(key, "beams") == 0) {
        int days[CADV_PORTAL_COUNT];
        *out = cadv_hub_beams(h, days, CADV_PORTAL_COUNT);
    } else if (strcmp(key, "outcome") == 0) {
        *out = 0.0; /* the hall never "ends" */
    } else if (sscanf(key, "state.%d", &day) == 1 && day >= 1 && day <= CADV_PORTAL_COUNT) {
        *out = (double)h->state[day - 1];
    } else if (sscanf(key, "reveal.%d", &day) == 1 && day >= 1 && day <= CADV_PORTAL_COUNT) {
        *out = h->reveal_timer[day - 1];
    } else if (sscanf(key, "legend.%d", &day) == 1 && day >= 0 && day < h->friends->count) {
        int found;
        int total;
        cadv_friends_tally(h->progress, day, &found, &total);
        *out = found;
    } else {
        return false;
    }
    return true;
}

void cadv_hub_teleport(cadv_hub *h, float x, float y, float z, float yaw)
{
    h->player.pos = cadv_v3(x, y, z);
    h->player.vel = cadv_v3(0, 0, 0);
    h->player.yaw = yaw;
    h->player.pitch = 0.0f;
}

int cadv_hub_beams(const cadv_hub *h, int *days, int max)
{
    int n = 0;
    int day;
    if (h->state[CADV_PORTAL_COUNT - 1] == CADV_PS_STARS_MISSING) {
        for (day = 1; day < CADV_PORTAL_COUNT && n < max; ++day) {
            if (!h->progress->p[day - 1].star_socketed) {
                days[n++] = day;
            }
        }
        return n;
    }
    for (day = CADV_PORTAL_COUNT; day >= 1; --day) {
        cadv_portal_state st = h->state[day - 1];
        if (st == CADV_PS_THAWING || st == CADV_PS_OPEN || st == CADV_PS_MASTERED) {
            if (max > 0) {
                days[0] = day;
                return 1;
            }
            break;
        }
    }
    return 0;
}

void cadv_hub_bot(const cadv_hub *h, cadv_input_frame *out)
{
    int target = 0;
    int day;
    cadv_vec3 path[8];
    int n;
    int i;
    int best = 0;
    float best_d = 1e9f;
    memset(out, 0, sizeof *out);
    for (day = CADV_PORTAL_COUNT; day >= 1 && !target; --day) {
        cadv_portal_state st = h->state[day - 1];
        if (st == CADV_PS_THAWING || st == CADV_PS_OPEN || st == CADV_PS_MASTERED) {
            target = day;
        }
    }
    if (!target) {
        return;
    }
    n = cadv_hall_path(target, path, 8);
    /* head for the closest waypoint, or the following one once we are already
     * closer to it than the closest waypoint is (i.e. past the corner) */
    for (i = 0; i < n; ++i) {
        float d = cadv_v3_dist(cadv_v3(h->player.pos.x, 0, h->player.pos.z), cadv_v3(path[i].x, 0, path[i].z));
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    if (best + 1 < n) {
        cadv_vec3 p = cadv_v3(h->player.pos.x, 0, h->player.pos.z);
        cadv_vec3 a = cadv_v3(path[best].x, 0, path[best].z);
        cadv_vec3 b = cadv_v3(path[best + 1].x, 0, path[best + 1].z);
        if (best_d < 0.8f || cadv_v3_dist(p, b) < cadv_v3_dist(a, b)) {
            ++best;
        }
    }
    {
        cadv_vec3 goal = best == n - 1 ? cadv_hall_portal(target).base : path[best];
        float dx = goal.x - h->player.pos.x;
        float dz = goal.z - h->player.pos.z;
        float dy = cadv_wrap_angle(cadv_atan2f(-dx, -dz) - h->player.yaw);
        out->look_x = cadv_clampf(dy, -0.2f, 0.2f);
        out->move_y = cadv_absf(dy) < 0.6f ? 1.0f : 0.15f;
    }
}

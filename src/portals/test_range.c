/* SPDX-License-Identifier: MIT */
#include "portals/test_range.h"

#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "core/mathx.h"
#include "gfx/mesh.h"
#include "hub/friends.h"
#include "kits/fps/combat.h"
#include "portals/test_range_internal.h"

size_t cadv_test_range_state_size(void) { return sizeof(tr_state); }

static void add_room_solids(cadv_world *w)
{
    cadv_world_add(w, cadv_v3(-9, -1, -15), cadv_v3(9, 0, 9), 0);    /* floor */
    cadv_world_add(w, cadv_v3(-9, 4, -15), cadv_v3(9, 5, 9), 0);     /* ceiling */
    cadv_world_add(w, cadv_v3(-9, 0, 8), cadv_v3(9, 4, 9), 0);       /* south */
    cadv_world_add(w, cadv_v3(-9, 0, -15), cadv_v3(-8, 4, 9), 0);    /* west */
    cadv_world_add(w, cadv_v3(8, 0, -15), cadv_v3(9, 4, 9), 0);      /* east */
    cadv_world_add(w, cadv_v3(-8, 0, -8.5f), cadv_v3(-1, 4, -8), 0); /* north left */
    cadv_world_add(w, cadv_v3(1, 0, -8.5f), cadv_v3(8, 4, -8), 0);   /* north right */
    cadv_world_add(w, cadv_v3(-1, 2.6f, -8.5f), cadv_v3(1, 4, -8), 0); /* lintel */
    cadv_world_add(w, cadv_v3(-4, 0, -15), cadv_v3(4, 4, -14), 0);   /* star room back */
    cadv_world_add(w, cadv_v3(-8, 0, -14), cadv_v3(-4, 4, -8.5f), 0); /* star room sides */
    cadv_world_add(w, cadv_v3(4, 0, -14), cadv_v3(8, 4, -8.5f), 0);
    cadv_world_add(w, cadv_v3(-3, 0, -2), cadv_v3(-2, 2, 2), 0);     /* cover blocks */
    cadv_world_add(w, cadv_v3(2, 0, -2), cadv_v3(3, 1.2f, 2), 0);
    cadv_world_add(w, cadv_v3(5.5f, 0, 3.5f), cadv_v3(6.5f, 0.8f, 4.5f), 0); /* key pedestal */
}

bool cadv_test_range_init(void *state, const cadv_portal_cfg *cfg)
{
    tr_state *s = (tr_state *)state;
    cadv_fps *f = &s->fps;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->day = cfg->day;
    s->checkpoint = cfg->checkpoint;
    cadv_fps_init(f, cfg->seed);
    f->damage_scale = cfg->difficulty == CADV_DIFF_STORY ? 0.35f : (cfg->difficulty == CADV_DIFF_HARD ? 1.5f : 1.0f);
    add_room_solids(&f->world);
    cadv_fps_add_door(f, cadv_v3(-1, 0, -8.4f), cadv_v3(1, 2.6f, -8.1f), KEY_BIT);
    if (s->checkpoint >= 1) {
        cadv_player_spawn(&f->player, cadv_v3(5.0f, 0.0f, 2.0f), 0.0f);
        f->player.keys = KEY_BIT;
    } else {
        cadv_player_spawn(&f->player, cadv_v3(0.0f, 0.0f, 6.5f), 0.0f);
        cadv_fps_add_pickup(f, CADV_PICKUP_KEY, (int)KEY_BIT, cadv_v3(6.0f, 1.2f, 4.0f));
    }
    cadv_fps_add_enemy(f, 0, cadv_v3(-5.0f, 0.0f, -5.0f), CADV_PI);
    cadv_fps_add_enemy(f, 1, cadv_v3(5.0f, 0.0f, -5.0f), CADV_PI);
    cadv_fps_add_enemy(f, cfg->difficulty == CADV_DIFF_STORY ? 0 : 2, cadv_v3(0.0f, 0.0f, -6.5f), CADV_PI);
    cadv_fps_add_pickup(f, CADV_PICKUP_HEALTH, 25, cadv_v3(-6.0f, 0.5f, 6.0f));
    cadv_fps_add_pickup(f, CADV_PICKUP_WEAPON, 2, cadv_v3(-6.0f, 0.5f, 2.0f));
    s->star_pickup = cadv_fps_add_pickup(f, CADV_PICKUP_STAR, 0, cadv_v3(0.0f, 1.3f, -12.0f));
    /* plaques with this day's friend marks on the walls */
    for (i = 0; d && i < d->mark_count && i < MAX_PLAQUES; ++i) {
        static const float px[MAX_PLAQUES] = {-7.95f, 7.95f, -7.95f, 7.95f};
        static const float pz[MAX_PLAQUES] = {-3.0f, -3.0f, 4.0f, 4.0f};
        cadv_friends fr;
        cadv_friends_default(&fr);
        if (cfg->friends) {
            int k;
            for (k = 0; k < cfg->friend_count && k < CADV_MAX_FRIENDS; ++k) {
                snprintf(fr.name[k], CADV_FRIEND_NAME, "%s", cfg->friends[k]);
            }
            fr.count = cfg->friend_count < CADV_MAX_FRIENDS ? cfg->friend_count : CADV_MAX_FRIENDS;
        }
        s->plaque_mark[i] = i;
        s->plaque_pos[i] = cadv_v3(px[i], 2.0f, pz[i]);
        s->plaque_yaw[i] = px[i] < 0.0f ? 0.5f * CADV_PI : -0.5f * CADV_PI; /* face into the room */
        cadv_friends_mark_text(&fr, &d->marks[i], s->plaque_text[i], sizeof s->plaque_text[i]);
        s->plaque_count++;
    }
    return true;
}

static void message(tr_state *s, const char *text)
{
    snprintf(s->message, sizeof s->message, "%s", text);
    s->message_timer = 150;
}

static void sfx(tr_state *s, cadv_sfx id)
{
    if (s->cfg.audio) {
        cadv_audio_sfx(s->cfg.audio, id, 1.0f, 0.0f);
    }
}

void cadv_test_range_tick(void *state, const cadv_input_frame *in)
{
    tr_state *s = (tr_state *)state;
    cadv_fps *f = &s->fps;
    uint32_t ev;
    int i;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
    if (s->message_timer > 0) {
        --s->message_timer;
    }
    if (s->win_timer > 0) {
        if (--s->win_timer == 0 || cadv_input_pressed(in, CADV_ACT_USE)) {
            s->outcome = CADV_OUTCOME_WON;
        }
        return;
    }
    if (s->death_timer > 0) {
        if (--s->death_timer == 0) {
            s->outcome = CADV_OUTCOME_LOST;
        }
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_PAUSE)) {
        s->outcome = CADV_OUTCOME_QUIT;
        return;
    }
    {
        uint32_t keys_before = f->player.keys;
        ev = cadv_fps_tick(f, in, DT);
        if (!(keys_before & KEY_BIT) && (f->player.keys & KEY_BIT)) {
            s->checkpoint = 1;
            message(s, "KEY FOUND - CHECKPOINT");
        }
    }
    if (ev & CADV_FPS_EV_SHOT) {
        sfx(s, CADV_SFX_SHOT);
    }
    if (ev & CADV_FPS_EV_ENEMY_HIT) {
        sfx(s, CADV_SFX_HIT);
    }
    if (ev & CADV_FPS_EV_EXPLOSION) {
        sfx(s, CADV_SFX_EXPLOSION);
    }
    if (ev & CADV_FPS_EV_DOOR) {
        sfx(s, CADV_SFX_DOOR);
    }
    if (ev & CADV_FPS_EV_DOOR_LOCKED) {
        sfx(s, CADV_SFX_DENIED);
        message(s, "LOCKED - FIND THE KEY");
    }
    if (ev & CADV_FPS_EV_PICKUP) {
        sfx(s, CADV_SFX_PICKUP);
    }
    if (ev & CADV_FPS_EV_STAR) {
        sfx(s, CADV_SFX_STAR);
        s->win_timer = 180;
    }
    if (ev & CADV_FPS_EV_PLAYER_DIED) {
        s->death_timer = 120;
        message(s, "YOU FROZE SOLID");
    }
    /* friend marks register when looked at from close range */
    for (i = 0; i < s->plaque_count; ++i) {
        cadv_vec3 eye = cadv_player_eye(&f->player);
        cadv_vec3 to = cadv_v3_sub(s->plaque_pos[i], eye);
        float dist = cadv_v3_len(to);
        if (dist < 3.0f && cadv_v3_dot(cadv_v3_scale(to, 1.0f / dist), cadv_player_forward(&f->player)) > 0.8f &&
            !(s->marks_found & (1u << (unsigned)s->plaque_mark[i]))) {
            s->marks_found |= 1u << (unsigned)s->plaque_mark[i];
            message(s, "A FRIEND'S MARK!");
        }
    }
}

cadv_portal_result cadv_test_range_result(const void *state)
{
    const tr_state *s = (const tr_state *)state;
    cadv_portal_result r;
    memset(&r, 0, sizeof r);
    r.outcome = s->outcome;
    r.star_collected = s->fps.star_taken;
    r.checkpoint = s->checkpoint;
    r.marks_found = s->marks_found;
    r.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        r.snowflakes = 1;
        if (cadv_fps_alive_enemies(&s->fps) == 0) {
            r.snowflakes = 2;
            if (s->ticks < 90 * 60 && s->cfg.difficulty != CADV_DIFF_STORY) {
                r.snowflakes = 3;
            }
        }
    }
    return r;
}

bool cadv_test_range_query(const void *state, const char *key, double *out)
{
    const tr_state *s = (const tr_state *)state;
    const cadv_player *p = &s->fps.player;
    if (strcmp(key, "player.x") == 0) {
        *out = p->pos.x;
    } else if (strcmp(key, "player.y") == 0) {
        *out = p->pos.y;
    } else if (strcmp(key, "player.z") == 0) {
        *out = p->pos.z;
    } else if (strcmp(key, "player.yaw") == 0) {
        *out = p->yaw;
    } else if (strcmp(key, "player.health") == 0) {
        *out = p->health;
    } else if (strcmp(key, "enemies.alive") == 0) {
        *out = cadv_fps_alive_enemies(&s->fps);
    } else if (strcmp(key, "star.taken") == 0) {
        *out = s->fps.star_taken ? 1.0 : 0.0;
    } else if (strcmp(key, "keys") == 0) {
        *out = p->keys;
    } else if (strcmp(key, "outcome") == 0) {
        *out = (double)s->outcome;
    } else if (strcmp(key, "marks") == 0) {
        *out = s->marks_found;
    } else if (strcmp(key, "ticks") == 0) {
        *out = s->ticks;
    } else if (strcmp(key, "door.open") == 0) {
        *out = s->fps.doors[0].open;
    } else {
        return false;
    }
    return true;
}

void cadv_test_range_teleport(void *state, float x, float y, float z, float yaw)
{
    tr_state *s = (tr_state *)state;
    s->fps.player.pos = cadv_v3(x, y, z);
    s->fps.player.vel = cadv_v3(0, 0, 0);
    s->fps.player.yaw = yaw;
}

/* ---- bot ------------------------------------------------------------------- */

static void steer(const cadv_player *p, cadv_vec3 target, cadv_input_frame *out, bool move)
{
    cadv_vec3 eye = cadv_player_eye(p);
    cadv_vec3 to = cadv_v3_sub(target, eye);
    float want_yaw = cadv_atan2f(-to.x, -to.z);
    float flat = cadv_sqrtf(to.x * to.x + to.z * to.z);
    float want_pitch = cadv_atan2f(to.y, flat);
    float dy = cadv_wrap_angle(want_yaw - p->yaw);
    out->look_x = cadv_clampf(dy, -0.15f, 0.15f);
    out->look_y = cadv_clampf(want_pitch - p->pitch, -0.1f, 0.1f);
    out->move_y = (move && cadv_absf(dy) < 0.6f) ? 1.0f : 0.0f;
}

void cadv_test_range_bot(const void *state, cadv_input_frame *out)
{
    const tr_state *s = (const tr_state *)state;
    const cadv_fps *f = &s->fps;
    const cadv_player *p = &f->player;
    int i;
    int best = -1;
    float best_d = 1e9f;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        out->pressed = out->held = CADV_ACT_BIT(CADV_ACT_USE);
        return;
    }
    /* 1. shoot visible enemies */
    for (i = 0; i < f->enemy_count; ++i) {
        const cadv_enemy *e = &f->enemies[i];
        float d = cadv_v3_dist(e->pos, p->pos);
        if (e->state != CADV_ENEMY_DEAD &&
            cadv_world_line_of_sight(&f->world, cadv_player_eye(p), cadv_v3_add(e->pos, cadv_v3(0, 1.0f, 0))) &&
            d < best_d) {
            best_d = d;
            best = i;
        }
    }
    if (best >= 0) {
        cadv_vec3 aim = cadv_v3_add(f->enemies[best].pos, cadv_v3(0, 1.0f, 0));
        steer(p, aim, out, false);
        if (cadv_absf(out->look_x) < 0.1f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
        }
        out->move_x = (s->ticks / 60) % 2 ? 0.6f : -0.6f; /* strafe */
        return;
    }
    /* 2. key, 3. door, 4. star */
    if (!(p->keys & KEY_BIT)) {
        steer(p, cadv_v3(6.0f, 1.2f, 4.0f), out, true);
        if (cadv_v3_dist(p->pos, cadv_v3(6.0f, 0, 4.0f)) < 2.0f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_JUMP);
            out->pressed |= CADV_ACT_BIT(CADV_ACT_JUMP);
        }
        return;
    }
    if (f->doors[0].open < 1.0f) {
        if (p->pos.z > -6.0f || cadv_absf(p->pos.x) > 0.5f) {
            steer(p, cadv_v3(0.0f, 1.5f, -6.5f), out, true);
        } else {
            steer(p, cadv_v3(0.0f, 1.5f, -9.0f), out, false);
            if ((s->ticks % 20) == 0) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_USE);
                out->held |= CADV_ACT_BIT(CADV_ACT_USE);
            }
        }
        return;
    }
    steer(p, cadv_v3(0.0f, 1.3f, -12.0f), out, true);
}


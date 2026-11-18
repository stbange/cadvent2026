/* SPDX-License-Identifier: MIT */
#include "portals/p01_sleighed.h"

#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "core/mathx.h"
#include "hub/friends.h"
#include "portals/p01_internal.h"

static const cadv_weapon_def k_weapons[P01_WEAPONS] = {
    /* name                  kind               dmg pel spread cool  range ammo per speed splash */
    {"Candy-Cane Kick", CADV_WK_MELEE, 30, 1, 0.00f, 0.40f, 1.9f, 0, 0, 0.0f, 0.0f},
    {"Snow Pistol", CADV_WK_HITSCAN, 14, 1, 0.015f, 0.28f, 60.f, 0, 0, 0.0f, 0.0f},
    {"Scattershot", CADV_WK_HITSCAN, 9, 7, 0.08f, 0.80f, 30.f, 1, 1, 0.0f, 0.0f},
    {"Gift-Bomb", CADV_WK_REMOTE, 110, 1, 0.00f, 0.55f, 30.f, 2, 1, 9.0f, 3.5f},
    {"Shrinkray", CADV_WK_SHRINK, 6, 1, 0.00f, 0.60f, 40.f, 3, 1, 0.0f, 0.0f},
    {"Snowblower Chaingun", CADV_WK_HITSCAN, 9, 1, 0.035f, 0.08f, 50.f, 0, 1, 0.0f, 0.0f},
};

static const cadv_enemy_def k_enemies[P01_ENEMY_TYPES] = {
    /* name                hp  speed  r     h    sight range cool  dmg  proj   fly   shrink */
    {"Trooper Hog", 40, 2.4f, 0.40f, 1.7f, 20.f, 16.f, 1.30f, 5, false, 0.0f, true},
    {"Snowplow Boar", 80, 4.4f, 0.55f, 1.5f, 16.f, 1.6f, 0.90f, 12, false, 0.0f, true},
    {"Drone Elf", 30, 3.2f, 0.35f, 0.7f, 22.f, 14.f, 1.60f, 4, false, 2.2f, false},
    {"Gremlin Gift", 25, 4.8f, 0.35f, 0.8f, 4.5f, 1.4f, 0.70f, 8, false, 0.0f, true},
    {"Hog Bombardier", 50, 1.8f, 0.45f, 1.8f, 24.f, 22.f, 2.40f, 14, true, 0.0f, true},
    {"Boarlord Overseer", 900, 1.7f, 0.90f, 3.2f, 40.f, 26.f, 1.60f, 18, true, 0.0f, false},
};

const cadv_weapon_def *cadv_p01_weapons(void) { return k_weapons; }
const cadv_enemy_def *cadv_p01_enemies(void) { return k_enemies; }

static int enemy_type(const char *name)
{
    static const char *const names[P01_ENEMY_TYPES] = {"hog", "boar", "drone", "gremlin", "lobber", "boss"};
    int i;
    for (i = 0; i < P01_ENEMY_TYPES; ++i) {
        if (strcmp(name, names[i]) == 0) {
            return i;
        }
    }
    return -1;
}

static void say(p01_state *s, const char *text)
{
    snprintf(s->message, sizeof s->message, "%s", text);
    s->message_timer = 180;
}

static void quip(p01_state *s, const char *text)
{
    snprintf(s->quip, sizeof s->quip, "%s", text);
    s->quip_timer = 200;
}

static void sfx(p01_state *s, cadv_sfx id, float vol)
{
    if (s->cfg.audio) {
        cadv_audio_sfx(s->cfg.audio, id, vol, 0.0f);
    }
}

static void deactivate_pickups(p01_state *s, cadv_pickup_kind kind, int value)
{
    int i;
    for (i = 0; i < s->fps.pickup_count; ++i) {
        cadv_pickup *p = &s->fps.pickups[i];
        if (p->kind == kind && (value < 0 || p->value == value)) {
            p->active = false;
        }
    }
}

static void spawn_at_trigger(p01_state *s, const char *name, float yaw)
{
    const cadv_map_item *t = cadv_map_find_trigger(&s->map, name);
    if (t) {
        cadv_vec3 c = cadv_v3(0.5f * (t->pos.x + t->hi.x), t->pos.y, 0.5f * (t->pos.z + t->hi.z));
        cadv_player_spawn(&s->fps.player, c, yaw);
    }
}

static void wake_boss(p01_state *s)
{
    if (s->boss_index >= 0 && !s->boss_awake) {
        s->boss_awake = true;
        s->fps.enemies[s->boss_index].active = true;
        s->fps.enemies[s->boss_index].awake = true;
        s->fps.enemies[s->boss_index].state = CADV_ENEMY_CHASE;
        say(s, "THE BOARLORD OVERSEER!");
    }
}

/* Nearest waypoint at or after 'from' (used after teleports and resumes). */
static int nearest_route(const p01_state *s, int from)
{
    int best = from;
    float bd = 1e9f;
    int i;
    for (i = from; i < s->map.nwaypoint; ++i) {
        float d = cadv_v3_dist(s->map.waypoint[i].pos, s->fps.player.pos);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    return best;
}

bool cadv_p01_init(void *state, const cadv_portal_cfg *cfg)
{
    p01_state *s = (p01_state *)state;
    cadv_fps *f = &s->fps;
    char path[600];
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->day = cfg->day;
    s->checkpoint = cfg->checkpoint;
    s->boss_index = -1;
    s->siphon_base_box = -1;
    s->star_pickup = -1;
    snprintf(path, sizeof path, "%sassets/maps/p01_sleighed.map", cfg->data_dir ? cfg->data_dir : "");
    if (!cadv_map_load(&s->map, path)) {
        return false;
    }
    cadv_fps_init(f, cfg->seed);
    cadv_fps_set_tables(f, k_weapons, P01_WEAPONS, k_enemies, P01_ENEMY_TYPES);
    f->damage_scale = cfg->difficulty == CADV_DIFF_STORY ? 0.35f : (cfg->difficulty == CADV_DIFF_HARD ? 1.5f : 1.0f);
    cadv_map_apply(&s->map, f, enemy_type, &s->links);
    f->owned[P01_WEAPON_CANE] = true;
    f->owned[P01_WEAPON_PISTOL] = true;
    f->weapon = P01_WEAPON_PISTOL;
    for (i = 0; i < s->map.nenemy; ++i) {
        if (strcmp(s->map.enemy[i].name, "boss") == 0) {
            s->boss_index = s->links.enemy_index[i];
            f->enemies[s->boss_index].active = false; /* wakes on the roof */
        }
    }
    for (i = 0; i < s->map.nbox; ++i) {
        if (strcmp(s->map.box[i].mat, "siphon_base") == 0) {
            s->siphon_base_box = i;
        }
    }
    if (s->checkpoint >= 2) {
        spawn_at_trigger(s, "roof_arrive", 0.5f * CADV_PI);
    } else if (s->checkpoint >= 1) {
        spawn_at_trigger(s, "checkpoint1", 0.0f);
    }
    if (s->checkpoint >= 1) {
        f->player.keys = P01_KEYS_ALL;
        deactivate_pickups(s, CADV_PICKUP_KEY, -1);
        f->owned[P01_WEAPON_SCATTER] = f->owned[P01_WEAPON_GIFTBOMB] = f->owned[P01_WEAPON_SHRINK] = true;
        f->ammo[1] = 20;
        f->ammo[2] = 6;
        f->ammo[3] = 10;
        f->doors[0].opening = true;
        f->doors[0].open = 1.0f;
        f->world.s[f->doors[0].solid].enabled = false;
    }
    if (s->checkpoint >= 2) {
        f->owned[P01_WEAPON_CHAINGUN] = true;
        f->ammo[0] = 60;
        deactivate_pickups(s, CADV_PICKUP_WEAPON, P01_WEAPON_CHAINGUN);
        wake_boss(s);
    }
    s->route = nearest_route(s, 0);
    /* friend marks: text of this day's mark definitions */
    cadv_friends_default(&fr);
    if (cfg->friends) {
        for (i = 0; i < cfg->friend_count && i < CADV_MAX_FRIENDS; ++i) {
            snprintf(fr.name[i], CADV_FRIEND_NAME, "%s", cfg->friends[i]);
        }
        fr.count = cfg->friend_count < CADV_MAX_FRIENDS ? cfg->friend_count : CADV_MAX_FRIENDS;
    }
    for (i = 0; d && i < d->mark_count && i < 16; ++i) {
        cadv_friends_mark_text(&fr, &d->marks[i], s->mark_text[i], sizeof s->mark_text[i]);
    }
    quip(s, s->checkpoint >= 2 ? "Back on the roof. Round two."
                               : "My mall. My lights. My Christmas.");
    return true;
}

static bool in_box(cadv_vec3 p, cadv_vec3 lo, cadv_vec3 hi)
{
    return p.x >= lo.x && p.x <= hi.x && p.y >= lo.y - 0.1f && p.y <= hi.y && p.z >= lo.z && p.z <= hi.z;
}

static void handle_triggers(p01_state *s)
{
    static const struct { const char *name; const char *line; } zones[] = {
        {"zone_atrium", "Glitterdome, I'm home. And it's full of pigs."},
        {"zone_toys", "Toys for good kids. Snowballs for bad hogs."},
        {"zone_food", "Nobody eats in my food court without paying."},
        {"zone_cinema", "Tonight's feature: Brick versus everybody."},
    };
    cadv_vec3 p = s->fps.player.pos;
    int i;
    size_t z;
    for (i = 0; i < s->map.ntrigger; ++i) {
        const cadv_map_item *t = &s->map.trigger[i];
        if (!in_box(p, t->pos, t->hi)) {
            continue;
        }
        if (strcmp(t->name, "checkpoint1") == 0 && s->checkpoint < 1) {
            s->checkpoint = 1;
            say(s, "CHECKPOINT");
        } else if (strcmp(t->name, "checkpoint2") == 0 && s->checkpoint < 2) {
            s->checkpoint = 2;
            say(s, "CHECKPOINT - THE ROOF");
            quip(s, "Who parks a spaceship on a mall roof?");
        } else if (strcmp(t->name, "lift") == 0) {
            /* ride the freight lift: move the body, keep health, armour and fuse candles */
            cadv_player keep = s->fps.player;
            spawn_at_trigger(s, "roof_arrive", 0.5f * CADV_PI);
            s->fps.player.health = keep.health;
            s->fps.player.armor = keep.armor;
            s->fps.player.keys = keep.keys;
            sfx(s, CADV_SFX_DOOR, 1.0f);
        } else if (strcmp(t->name, "boss") == 0) {
            wake_boss(s);
        } else if (strcmp(t->name, "exit") == 0 && s->fps.star_taken && s->win_timer == 0) {
            s->win_timer = 180;
            sfx(s, CADV_SFX_STAR, 1.0f);
        } else {
            for (z = 0; z < sizeof zones / sizeof zones[0]; ++z) {
                if (strcmp(t->name, zones[z].name) == 0 && !(s->zones_seen & (1u << z))) {
                    s->zones_seen |= 1u << z;
                    quip(s, zones[z].line);
                }
            }
        }
    }
    for (i = 0; i < s->map.nsecret && i < 5; ++i) {
        const cadv_map_item *t = &s->map.secret[i];
        if (!(s->secrets_found & (1u << i)) && in_box(p, t->pos, t->hi)) {
            s->secrets_found |= 1u << i;
            say(s, "SECRET FOUND!");
            quip(s, "Ooh, a secret. Brick likes secrets.");
            sfx(s, CADV_SFX_BELL, 0.8f);
        }
    }
}

static void use_prop(p01_state *s, int prop)
{
    cadv_player *pl = &s->fps.player;
    const char *kind = s->map.prop[prop].name;
    if (strcmp(kind, "vending") == 0) {
        if (s->prop_uses[prop] >= 2 || pl->health >= 100) {
            say(s, s->prop_uses[prop] >= 2 ? "SOLD OUT" : "YOU'RE NOT THIRSTY");
            return;
        }
        s->prop_uses[prop]++;
        pl->health = cadv_mini(100, pl->health + 20);
        say(s, "SNOW CONE! +20");
        sfx(s, CADV_SFX_PICKUP, 1.0f);
    } else if (strcmp(kind, "fountain") == 0 || strcmp(kind, "sink") == 0) {
        if (pl->health >= 100) {
            say(s, "REFRESHING. BUT YOU'RE FINE.");
            return;
        }
        pl->health = cadv_mini(100, pl->health + 2);
        say(s, "SPLASH. +2");
    } else if (strcmp(kind, "arcade") == 0) {
        if (!(s->secrets_found & (1u << 5))) {
            s->secrets_found |= 1u << 5;
            say(s, "SECRET FOUND: NEW HIGH SCORE!");
            quip(s, "Still got it.");
            sfx(s, CADV_SFX_BELL, 0.8f);
        } else {
            say(s, "GAME OVER. INSERT COIN.");
        }
    }
}

static void check_marks(p01_state *s)
{
    int i;
    cadv_vec3 eye = cadv_player_eye(&s->fps.player);
    cadv_vec3 fwd = cadv_player_forward(&s->fps.player);
    for (i = 0; i < s->map.nmark; ++i) {
        int idx = (int)s->map.mark[i].a;
        cadv_vec3 to = cadv_v3_sub(s->map.mark[i].pos, eye);
        float d = cadv_v3_len(to);
        if (idx < 0 || idx > 15 || (s->marks_found & (1u << (unsigned)idx))) {
            continue;
        }
        if (d < 4.0f && cadv_v3_dot(cadv_v3_scale(to, 1.0f / d), fwd) > 0.85f &&
            cadv_world_line_of_sight(&s->fps.world, eye, cadv_v3_add(s->map.mark[i].pos, cadv_v3_scale(to, -0.3f / d)))) {
            s->marks_found |= 1u << (unsigned)idx;
            say(s, "A FRIEND'S MARK!");
        }
    }
}

/* Line of sight from a to b through the enabled solids, ignoring solid 'skip'. */
static bool los_ignoring(const cadv_world *w, cadv_vec3 a, cadv_vec3 b, int skip)
{
    cadv_vec3 d = cadv_v3_sub(b, a);
    float len = cadv_v3_len(d);
    int i;
    if (len < 1e-5f) {
        return true;
    }
    d = cadv_v3_scale(d, 1.0f / len);
    for (i = 0; i < w->count; ++i) {
        float t;
        if (i != skip && w->s[i].enabled && cadv_ray_aabb(a, d, w->s[i].box, len, &t, NULL)) {
            return false;
        }
    }
    return true;
}

static bool target_visible(const p01_state *s, int *which)
{
    int i;
    float best = 1e9f;
    cadv_vec3 eye = cadv_player_eye(&s->fps.player);
    bool found = false;
    for (i = 0; i < s->fps.breakable_count; ++i) {
        const cadv_breakable *b = &s->fps.breakables[i];
        cadv_aabb box = s->fps.world.s[b->solid].box;
        cadv_vec3 c = cadv_v3_scale(cadv_v3_add(box.lo, box.hi), 0.5f);
        float d = cadv_v3_dist(c, eye);
        bool clear;
        if (b->broken || b->group != 1) {
            continue;
        }
        clear = los_ignoring(&s->fps.world, eye, c, b->solid);
        if (clear && d < best && d < 30.0f) {
            best = d;
            if (which) {
                *which = i;
            }
            found = true;
        }
    }
    return found;
}

static void advance_route(p01_state *s)
{
    const cadv_map_item *w;
    cadv_vec3 p = s->fps.player.pos;
    float dx;
    float dz;
    if (s->route >= s->map.nwaypoint) {
        return;
    }
    w = &s->map.waypoint[s->route];
    if (cadv_absf(w->pos.y - p.y) > 5.0f) {
        /* the lift changed the floor level: continue with the next waypoint up there */
        int k = s->route;
        while (k < s->map.nwaypoint && cadv_absf(s->map.waypoint[k].pos.y - p.y) > 2.0f) {
            ++k;
        }
        if (k >= s->map.nwaypoint) {
            return;
        }
        s->route = k;
        w = &s->map.waypoint[s->route];
    }
    dx = w->pos.x - p.x;
    dz = w->pos.z - p.z;
    if (dx * dx + dz * dz > 1.0f || cadv_absf(w->pos.y - p.y) > 1.5f) {
        return;
    }
    if (strcmp(w->name, "use") == 0 && s->fps.doors[0].open < 1.0f) {
        return;
    }
    if (strcmp(w->name, "targets") == 0 && target_visible(s, NULL)) {
        return;
    }
    if (strcmp(w->name, "star") == 0 && !s->fps.star_taken) {
        return;
    }
    s->route++;
}

static void siphon_down(p01_state *s)
{
    cadv_fps *f = &s->fps;
    cadv_vec3 core = cadv_v3(0.0f, 13.0f, -44.0f);
    s->siphon_down = true;
    f->damage_scale *= 1.0f;
    cadv_fps_explode(f, core, 3.0f, 40, true);
    if (s->siphon_base_box >= 0 && s->links.box_solid[s->siphon_base_box] >= 0) {
        f->world.s[s->links.box_solid[s->siphon_base_box]].enabled = false;
    }
    s->star_pickup = cadv_fps_add_pickup(f, CADV_PICKUP_STAR, 0, cadv_v3(0.0f, 12.9f, -44.0f));
    s->collapse_ticks = 60 * (s->cfg.difficulty == CADV_DIFF_STORY ? 150 : (s->cfg.difficulty == CADV_DIFF_HARD ? 45 : 75));
    s->shake = 90;
    say(s, "THE SIPHON IS DOWN! GRAB THE STAR AND GET OUT!");
    quip(s, "Lights out, piggies.");
    sfx(s, CADV_SFX_EXPLOSION, 1.0f);
}

void cadv_p01_tick(void *state, const cadv_input_frame *in)
{
    static const char *const kill_lines[] = {"Naughty list. Permanently.", "Ho-ho-hold still!",
                                             "That one's on the house.", "Clean-up on aisle five.",
                                             "Gift-wrapped and delivered."};
    p01_state *s = (p01_state *)state;
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
    if (s->quip_timer > 0) {
        --s->quip_timer;
    }
    if (s->shake > 0) {
        --s->shake;
    }
    if (s->win_timer > 0) {
        if (--s->win_timer == 0 || (s->win_timer < 150 && cadv_input_pressed(in, CADV_ACT_USE))) {
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
    ev = cadv_fps_tick(f, in, P01_DT);
    if (ev & CADV_FPS_EV_SHOT) {
        sfx(s, CADV_SFX_SHOT, 0.7f);
    }
    if (ev & (CADV_FPS_EV_ENEMY_HIT | CADV_FPS_EV_BREAK)) {
        sfx(s, CADV_SFX_HIT, 0.8f);
    }
    if (ev & CADV_FPS_EV_EXPLOSION) {
        sfx(s, CADV_SFX_EXPLOSION, 1.0f);
        s->shake = s->shake > 20 ? s->shake : 20;
    }
    if (ev & CADV_FPS_EV_DOOR) {
        sfx(s, CADV_SFX_DOOR, 1.0f);
        say(s, "STAFF ONLY - WELCOME, STAFF");
    }
    if (ev & CADV_FPS_EV_DOOR_LOCKED) {
        char buf[72];
        int have = ((f->player.keys & 1u) != 0) + ((f->player.keys & 2u) != 0) + ((f->player.keys & 4u) != 0);
        snprintf(buf, sizeof buf, "STAFF ONLY - NEEDS 3 FUSE CANDLES (%d/3)", have);
        say(s, buf);
        sfx(s, CADV_SFX_DENIED, 1.0f);
    }
    if (ev & CADV_FPS_EV_PICKUP) {
        sfx(s, CADV_SFX_PICKUP, 1.0f);
    }
    if (ev & CADV_FPS_EV_USE_SOLID) {
        for (i = 0; i < s->map.nprop; ++i) {
            if (s->links.prop_solid[i] == f->use_solid) {
                use_prop(s, i);
            }
        }
    }
    if (ev & CADV_FPS_EV_STOMP) {
        quip(s, "Squeak. Squish. Season's greetings.");
    } else if (ev & CADV_FPS_EV_ENEMY_KILLED) {
        if (++s->kill_streak % 4 == 0) {
            quip(s, kill_lines[(s->kill_streak / 4) % 5]);
        }
        if (f->last_killed == s->boss_index) {
            say(s, "BOARLORD DEFEATED!");
        }
    }
    if (ev & CADV_FPS_EV_STAR) {
        sfx(s, CADV_SFX_STAR, 1.0f);
        say(s, "SHINING STAR! NOW RUN TO THE SLEIGH PAD!");
    }
    if (!s->low_health_quipped && f->player.health > 0 && f->player.health < 25) {
        s->low_health_quipped = true;
        quip(s, "I've had worse paper cuts.");
    }
    if ((ev & CADV_FPS_EV_PLAYER_DIED) || f->player.health <= 0) {
        s->death_timer = 120;
        say(s, "BRICK IS ON THE NAUGHTY LIST... TRY AGAIN");
        return;
    }
    if (!s->siphon_down && cadv_fps_broken_count(f, 1) >= 3) {
        siphon_down(s);
    }
    if (s->siphon_down && s->win_timer == 0 && s->collapse_ticks > 0 && --s->collapse_ticks == 0) {
        s->death_timer = 120;
        s->shake = 120;
        say(s, "THE DOME CAVED IN...");
        return;
    }
    advance_route(s);
    handle_triggers(s);
    check_marks(s);
}

cadv_portal_result cadv_p01_result(const void *state)
{
    const p01_state *s = (const p01_state *)state;
    cadv_portal_result r;
    int secrets = 0;
    int i;
    memset(&r, 0, sizeof r);
    r.outcome = s->outcome;
    r.star_collected = s->fps.star_taken;
    r.checkpoint = s->checkpoint;
    r.marks_found = s->marks_found;
    r.ticks = s->ticks;
    for (i = 0; i < CADV_P01_SECRETS; ++i) {
        secrets += (s->secrets_found >> i) & 1u;
    }
    if (s->outcome == CADV_OUTCOME_WON) {
        r.snowflakes = 1;
        if (s->ticks < 18 * 60 * 60 && s->cfg.difficulty != CADV_DIFF_STORY) {
            r.snowflakes++;
        }
        if (secrets == CADV_P01_SECRETS) {
            r.snowflakes++;
        }
    }
    return r;
}

bool cadv_p01_query(const void *state, const char *key, double *out)
{
    const p01_state *s = (const p01_state *)state;
    const cadv_player *p = &s->fps.player;
    int secrets = 0;
    int i;
    for (i = 0; i < CADV_P01_SECRETS; ++i) {
        secrets += (s->secrets_found >> i) & 1u;
    }
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
    } else if (strcmp(key, "keys") == 0) {
        *out = p->keys;
    } else if (strcmp(key, "enemies.alive") == 0) {
        *out = cadv_fps_alive_enemies(&s->fps);
    } else if (strcmp(key, "secrets") == 0) {
        *out = secrets;
    } else if (strcmp(key, "emitters") == 0) {
        *out = cadv_fps_broken_count(&s->fps, 1);
    } else if (strcmp(key, "siphon_down") == 0) {
        *out = s->siphon_down ? 1.0 : 0.0;
    } else if (strcmp(key, "boss_awake") == 0) {
        *out = s->boss_awake ? 1.0 : 0.0;
    } else if (strcmp(key, "star.taken") == 0) {
        *out = s->fps.star_taken ? 1.0 : 0.0;
    } else if (strcmp(key, "collapse") == 0) {
        *out = s->collapse_ticks;
    } else if (strcmp(key, "outcome") == 0) {
        *out = (double)s->outcome;
    } else if (strcmp(key, "marks") == 0) {
        *out = s->marks_found;
    } else if (strcmp(key, "ticks") == 0) {
        *out = s->ticks;
    } else if (strcmp(key, "route") == 0) {
        *out = s->route;
    } else if (strcmp(key, "jetpack") == 0) {
        *out = s->fps.jetpack ? 1.0 : 0.0;
    } else {
        return false;
    }
    return true;
}

void cadv_p01_teleport(void *state, float x, float y, float z, float yaw)
{
    p01_state *s = (p01_state *)state;
    s->fps.player.pos = cadv_v3(x, y, z);
    s->fps.player.vel = cadv_v3(0, 0, 0);
    s->fps.player.yaw = yaw;
    s->fps.player.pitch = 0.0f;
}

/* ---- completion bot --------------------------------------------------------------- */

static void aim(const cadv_player *p, cadv_vec3 target, cadv_input_frame *out, float *err)
{
    cadv_vec3 eye = cadv_player_eye(p);
    cadv_vec3 to = cadv_v3_sub(target, eye);
    float want_yaw = cadv_atan2f(-to.x, -to.z);
    float flat = cadv_sqrtf(to.x * to.x + to.z * to.z);
    float want_pitch = cadv_atan2f(to.y, flat);
    float dy = cadv_wrap_angle(want_yaw - p->yaw);
    out->look_x = cadv_clampf(dy, -0.3f, 0.3f);
    out->look_y = cadv_clampf(want_pitch - p->pitch, -0.2f, 0.2f);
    *err = cadv_absf(dy);
}

static void choose_weapon(const cadv_fps *f, float dist, cadv_input_frame *out)
{
    int want = P01_WEAPON_PISTOL;
    if (f->owned[P01_WEAPON_CHAINGUN] && f->ammo[0] > 0) {
        want = P01_WEAPON_CHAINGUN;
    } else if (f->owned[P01_WEAPON_SCATTER] && f->ammo[1] > 0 && dist < 9.0f) {
        want = P01_WEAPON_SCATTER;
    }
    if (f->weapon != want) {
        out->pressed |= CADV_ACT_BIT(CADV_ACT_WEAPON_1 + want);
        out->held |= CADV_ACT_BIT(CADV_ACT_WEAPON_1 + want);
    }
}

void cadv_p01_bot(const void *state, cadv_input_frame *out)
{
    const p01_state *s = (const p01_state *)state;
    const cadv_fps *f = &s->fps;
    const cadv_player *p = &f->player;
    cadv_vec3 eye = cadv_player_eye(p);
    const cadv_map_item *w;
    int i;
    int best = -1;
    float best_d = 22.0f;
    float err;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        if (s->win_timer < 140) {
            out->pressed = out->held = CADV_ACT_BIT(CADV_ACT_USE);
        }
        return;
    }
    /* 1. shoot visible enemies */
    for (i = 0; i < f->enemy_count; ++i) {
        const cadv_enemy *e = &f->enemies[i];
        const cadv_enemy_def *d = &f->edefs[e->type];
        float dd = cadv_v3_dist(e->pos, p->pos);
        if (!e->active || e->state == CADV_ENEMY_DEAD || dd >= best_d) {
            continue;
        }
        if (cadv_world_line_of_sight(&f->world, eye, cadv_v3_add(e->pos, cadv_v3(0, d->height * 0.6f, 0)))) {
            best_d = dd;
            best = i;
        }
    }
    if (best >= 0) {
        const cadv_enemy *e = &f->enemies[best];
        aim(p, cadv_v3_add(e->pos, cadv_v3(0, f->edefs[e->type].height * 0.6f, 0)), out, &err);
        choose_weapon(f, best_d, out);
        if (err < 0.12f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
        }
        out->move_x = (s->ticks / 45) % 2 ? 0.7f : -0.7f;
        return;
    }
    if (s->route >= s->map.nwaypoint) {
        return;
    }
    w = &s->map.waypoint[s->route];
    /* 2. objectives at the current waypoint */
    if (strcmp(w->name, "targets") == 0 && cadv_v3_dist(cadv_v3(w->pos.x, p->pos.y, w->pos.z), p->pos) < 1.2f) {
        int t = -1;
        if (target_visible(s, &t)) {
            cadv_aabb b = f->world.s[f->breakables[t].solid].box;
            aim(p, cadv_v3_scale(cadv_v3_add(b.lo, b.hi), 0.5f), out, &err);
            choose_weapon(f, 20.0f, out);
            if (err < 0.06f) {
                out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
            }
            return;
        }
    }
    if (strcmp(w->name, "use") == 0 && cadv_v3_dist(cadv_v3(w->pos.x, p->pos.y, w->pos.z), p->pos) < 1.2f) {
        aim(p, cadv_v3(w->pos.x, eye.y, w->pos.z - 5.0f), out, &err);
        if (err < 0.1f && (s->ticks % 15) == 0) {
            out->pressed |= CADV_ACT_BIT(CADV_ACT_USE);
            out->held |= CADV_ACT_BIT(CADV_ACT_USE);
        }
        return;
    }
    /* 3. walk the route */
    {
        float dx = w->pos.x - p->pos.x;
        float dz = w->pos.z - p->pos.z;
        float dy = cadv_wrap_angle(cadv_atan2f(-dx, -dz) - p->yaw);
        out->look_x = cadv_clampf(dy, -0.25f, 0.25f);
        out->look_y = cadv_clampf(-p->pitch, -0.1f, 0.1f);
        out->move_y = cadv_absf(dy) < 0.5f ? 1.0f : 0.15f;
        out->held |= CADV_ACT_BIT(CADV_ACT_RUN);
    }
}

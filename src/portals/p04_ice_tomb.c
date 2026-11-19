/* SPDX-License-Identifier: MIT */
#include "portals/p04_ice_tomb.h"

#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "core/mathx.h"
#include "hub/friends.h"
#include "portals/p04_internal.h"

/* ---- the tomb's layout (tiles, 1 m) -------------------------------------------------- */

enum {
    F_LEVER_HALL, F_GEAR1, F_LEVER_POOL, F_GEAR2, F_LEVER_VENT, F_ICE, F_GEAR3, F_GEARS_SET,
    F_CRYSTAL0, F_CRYSTAL1, F_CRYSTAL2, F_STAR, F_BLOCK, F_JADE0, F_JADE1, F_JADE2, F_BLADE_CLEAR, F_FLAGS
};

static const cadv_vec3 k_levers[P04_LEVERS] = {{24.5f, 9.0f, 61.5f}, {28.5f, 2.0f, 24.5f}, {36.5f, 6.0f, 31.5f}};
static const cadv_vec3 k_crystals[P04_CRYSTALS] = {{9.5f, 6.0f, 46.5f}, {27.5f, 6.0f, 34.5f}, {35.5f, 6.0f, 37.5f}};
static const cadv_vec3 k_gears[3] = {{2.5f, 6.0f, 51.5f}, {4.0f, 6.0f, 15.0f}, {34.5f, 6.0f, 6.5f}};
static const cadv_vec3 k_jade[CADV_P04_JADE] = {{19.5f, 6.0f, 89.5f}, {8.5f, 2.0f, 21.5f}, {22.5f, 9.0f, 56.5f}};
static const cadv_vec3 k_solar = {34.5f, 6.0f, 51.5f};
#define BLOCK_GOAL_X 21
#define BLOCK_GOAL_Z 61

cadv_vec3 cadv_p04_lever_pos(int lever) { return k_levers[lever]; }
cadv_vec3 cadv_p04_crystal_pos(int crystal) { return k_crystals[crystal]; }
cadv_vec3 cadv_p04_solar_door(void) { return k_solar; }

cadv_vec3 cadv_p04_plaque_pos(int plaque, float *yaw)
{
    switch (plaque) {
    case P04_PLAQUE_TALLY: /* scratched into the air pocket */
        *yaw = 0.0f;
        return cadv_v3(12.5f, 5.4f, 18.03f);
    case P04_PLAQUE_BLADE: /* carved beside the swinging blade */
        *yaw = 0.5f * CADV_PI;
        return cadv_v3(14.03f, 7.6f, 41.5f);
    default: /* the earlier explorer's journal, pinned at the entrance */
        *yaw = 0.5f * CADV_PI;
        return cadv_v3(10.03f, 5.6f, 91.5f);
    }
}

cadv_vec3 cadv_p04_blade_pos(const p04_state *s)
{
    float t = (float)s->ticks * P04_DT;
    return cadv_v3(16.0f + 1.8f * cadv_sinf(t * 2.0f * CADV_PI / 2.4f), 6.0f, 39.5f);
}

bool cadv_p04_flag(const p04_state *s, int flag)
{
    switch (flag) {
    case F_LEVER_HALL:
        return s->lever[P04_LEVER_HALL];
    case F_GEAR1:
        return (s->gear_bits & 1u) != 0;
    case F_LEVER_POOL:
        return s->lever[P04_LEVER_POOL];
    case F_GEAR2:
        return (s->gear_bits & 2u) != 0;
    case F_LEVER_VENT:
        return s->lever[P04_LEVER_VENT];
    case F_ICE:
        return s->ice_melted;
    case F_GEAR3:
        return (s->gear_bits & 4u) != 0;
    case F_GEARS_SET:
        return s->gears_set;
    case F_CRYSTAL0:
    case F_CRYSTAL1:
    case F_CRYSTAL2:
        return s->crystal_used[flag - F_CRYSTAL0];
    case F_STAR:
        return s->star_taken;
    case F_BLOCK:
        return cadv_tiles_get(&s->tiles, BLOCK_GOAL_X, 6, BLOCK_GOAL_Z) == CADV_TILE_BLOCK;
    case F_BLADE_CLEAR: {
        float bx = cadv_p04_blade_pos(s).x;
        float t1 = (float)(s->ticks + 1) * P04_DT;
        float next = 16.0f + 1.8f * cadv_sinf(t1 * 2.0f * CADV_PI / 2.4f);
        return cadv_absf(bx - s->me.pos.x) > 2.6f && cadv_absf(next - s->me.pos.x) >= cadv_absf(bx - s->me.pos.x) - 0.001f;
    }
    default:
        return (s->jade_bits & (1u << (unsigned)(flag - F_JADE0))) != 0;
    }
}

static void carve(cadv_tiles *t, int x0, int y0, int z0, int x1, int y1, int z1) { cadv_tiles_fill(t, x0, y0, z0, x1, y1, z1, CADV_TILE_AIR); }

static void build_tomb(p04_state *s)
{
    cadv_tiles *t = &s->tiles;
    cadv_tiles_init(t, P04_W, P04_H, P04_D, CADV_TILE_ROCK);
    /* A: entrance cavern (floor 4), pits, the 2-tile wall up to the raised floor (6) */
    carve(t, 10, 4, 81, 20, 10, 95);
    carve(t, 10, 2, 87, 20, 3, 87);            /* 1-tile pit: standing jump */
    carve(t, 10, 2, 83, 20, 3, 84);            /* 2-tile pit: running jump */
    cadv_tiles_fill(t, 18, 4, 89, 20, 5, 90, CADV_TILE_ROCK); /* jade ledge */
    carve(t, 10, 6, 72, 20, 10, 80);           /* raised floor, top 6 */
    carve(t, 13, 6, 66, 17, 10, 71);           /* corridor to the hall */
    /* B: Hall of Pillars (floor 6) */
    carve(t, 6, 6, 44, 26, 14, 65);
    cadv_tiles_fill(t, 22, 6, 60, 25, 8, 63, CADV_TILE_ROCK); /* lever platform, top 9 */
    cadv_tiles_fill(t, 22, 6, 56, 23, 8, 57, CADV_TILE_ROCK); /* jade pillar, top 9 */
    cadv_tiles_fill(t, 10, 6, 52, 10, 13, 52, CADV_TILE_ROCK); /* decorative pillars */
    cadv_tiles_fill(t, 16, 6, 47, 16, 13, 47, CADV_TILE_ROCK);
    cadv_tiles_fill(t, 12, 6, 60, 12, 13, 60, CADV_TILE_ROCK);
    cadv_tiles_set(t, 19, 6, 56, CADV_TILE_BLOCK);
    carve(t, 1, 6, 50, 4, 9, 52);              /* gear alcove */
    cadv_tiles_fill(t, 5, 6, 50, 5, 8, 52, CADV_TILE_DOOR);
    carve(t, 14, 6, 36, 17, 10, 43);           /* blade corridor */
    /* C: flooded crypt (walkway 6, water surface 6) */
    carve(t, 4, 6, 33, 30, 10, 35);
    carve(t, 16, 2, 22, 30, 10, 32);
    carve(t, 16, 2, 18, 19, 10, 21);
    cadv_tiles_fill(t, 16, 2, 18, 30, 5, 32, CADV_TILE_WATER);
    cadv_tiles_fill(t, 10, 2, 18, 15, 4, 19, CADV_TILE_WATER); /* the tunnel */
    carve(t, 12, 5, 18, 13, 6, 19);            /* air pocket */
    cadv_tiles_fill(t, 15, 2, 18, 15, 4, 19, CADV_TILE_GRATE);
    carve(t, 3, 2, 14, 9, 10, 23);
    cadv_tiles_fill(t, 3, 2, 14, 9, 5, 23, CADV_TILE_WATER);
    cadv_tiles_fill(t, 3, 2, 14, 5, 5, 16, CADV_TILE_ROCK); /* gear ledge, top 6 */
    cadv_tiles_fill(t, 10, 2, 18, 11, 4, 19, CADV_TILE_WATER);
    carve(t, 31, 6, 33, 31, 10, 35);           /* to the gallery */
    /* D: the frozen colossus gallery and the solar chamber */
    carve(t, 32, 6, 4, 37, 13, 44);
    cadv_tiles_fill(t, 32, 6, 28, 37, 12, 29, CADV_TILE_ICE);
    carve(t, 32, 4, 16, 37, 5, 17);            /* 2-tile pit */
    carve(t, 32, 6, 45, 37, 13, 52);
}

static void add_pickup(p04_state *s, int kind, int value, cadv_vec3 pos)
{
    p04_pickup *p = &s->pickups[s->npickups++];
    p->active = true;
    p->kind = kind;
    p->value = value;
    p->pos = pos;
}

static void add_enemy(p04_state *s, int type, cadv_vec3 pos)
{
    int i;
    static const int hp[P04_ENEMY_TYPES] = {30, 15, 120, 100000};
    for (i = 0; i < P04_MAX_ENEMIES; ++i) {
        if (!s->enemies[i].active) {
            p04_enemy *e = &s->enemies[i];
            memset(e, 0, sizeof *e);
            e->active = true;
            e->type = type;
            e->pos = e->home = pos;
            e->hp = hp[type];
            return;
        }
    }
}

/* Bounded copy: long lines are cut on purpose. */
static void copy_text(char *out, size_t cap, const char *text)
{
    size_t i;
    for (i = 0; text[i] && i + 1 < cap; ++i) {
        out[i] = text[i];
    }
    out[i] = '\0';
}

static void say(p04_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 200;
}

static void quip(p04_state *s, const char *text)
{
    copy_text(s->quip, sizeof s->quip, text);
    s->quip_timer = 240;
}

static void sfx(p04_state *s, cadv_sfx id, float vol)
{
    if (s->cfg.audio) {
        cadv_audio_sfx(s->cfg.audio, id, vol, 0.0f);
    }
}

static void open_door(p04_state *s, bool open)
{
    cadv_tiles_fill(&s->tiles, 5, 6, 50, 5, 8, 52, open ? CADV_TILE_AIR : CADV_TILE_DOOR);
}

static void take_gear(p04_state *s, int g)
{
    s->gear_bits |= 1u << (unsigned)g;
    s->gears = ((s->gear_bits & 1u) != 0) + ((s->gear_bits & 2u) != 0) + ((s->gear_bits & 4u) != 0);
    s->pickups[g].active = false;
}

static void place_spawn(p04_state *s, cadv_vec3 p, float yaw)
{
    s->respawn = p;
    s->respawn_yaw = yaw;
    cadv_climber_spawn(&s->me, &s->cdef, p, yaw);
}

bool cadv_p04_init(void *state, const cadv_portal_cfg *cfg)
{
    p04_state *s = (p04_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->checkpoint = cfg->checkpoint;
    s->crystal = -1;
    cadv_friends_default(&fr);
    if (cfg->friends) {
        for (i = 0; i < cfg->friend_count && i < CADV_MAX_FRIENDS; ++i) {
            snprintf(fr.name[i], CADV_FRIEND_NAME, "%s", cfg->friends[i]);
        }
        fr.count = cfg->friend_count < CADV_MAX_FRIENDS ? cfg->friend_count : CADV_MAX_FRIENDS;
    }
    for (i = 0; d && i < d->mark_count && i < 4; ++i) {
        cadv_friends_mark_text(&fr, &d->marks[i], s->mark_text[i], sizeof s->mark_text[i]);
    }
    build_tomb(s);
    cadv_climber_default_def(&s->cdef);
    s->hp = P04_HP;
    s->collapse_z = -1.0f;
    /* gears first so their index is their pickup index */
    for (i = 0; i < 3; ++i) {
        add_pickup(s, P04_PK_GEAR, i, k_gears[i]);
    }
    for (i = 0; i < CADV_P04_JADE; ++i) {
        add_pickup(s, P04_PK_JADE, i, k_jade[i]);
    }
    add_pickup(s, P04_PK_HEALTH, 40, cadv_v3(12.5f, 6.0f, 62.5f));
    add_pickup(s, P04_PK_HEALTH, 40, cadv_v3(8.5f, 6.0f, 34.5f));
    add_pickup(s, P04_PK_HEALTH, 40, cadv_v3(33.5f, 6.0f, 39.5f));
    add_enemy(s, P04_WOLF, cadv_v3(13.5f, 6.0f, 75.5f));
    add_enemy(s, P04_WOLF, cadv_v3(17.5f, 6.0f, 74.5f));
    add_enemy(s, P04_BAT, cadv_v3(15.0f, 10.0f, 50.0f));
    add_enemy(s, P04_BAT, cadv_v3(12.0f, 10.0f, 57.0f));
    add_enemy(s, P04_WOLF, cadv_v3(25.5f, 6.0f, 34.5f));
    add_enemy(s, P04_BEAR, cadv_v3(34.5f, 6.0f, 21.5f));
    add_enemy(s, P04_COLOSSUS, cadv_v3(34.5f, 6.0f, 3.0f));
    place_spawn(s, cadv_v3(15.5f, 4.0f, 93.5f), 0.0f);
    if (s->checkpoint >= 1) {
        take_gear(s, 0);
        s->door_jammed = true;
        open_door(s, true);
        s->lever[P04_LEVER_HALL] = true;
        cadv_tiles_set(&s->tiles, 19, 6, 56, CADV_TILE_AIR);
        cadv_tiles_set(&s->tiles, BLOCK_GOAL_X, 6, BLOCK_GOAL_Z, CADV_TILE_BLOCK);
        place_spawn(s, cadv_v3(9.5f, 6.0f, 45.5f), CADV_PI);
        s->enemies[0].active = s->enemies[1].active = false;
    }
    if (s->checkpoint >= 2) {
        take_gear(s, 1);
        s->lever[P04_LEVER_POOL] = true;
        cadv_tiles_fill(&s->tiles, 15, 2, 18, 15, 4, 19, CADV_TILE_WATER);
        place_spawn(s, cadv_v3(27.5f, 6.0f, 33.6f), -0.5f * CADV_PI);
        s->enemies[2].active = s->enemies[3].active = s->enemies[4].active = false;
    }
    s->route = 0;
    if (s->checkpoint >= 1) {
        int n;
        const p04_step *r = cadv_p04_route(&n);
        /* resume right after the save crystal the checkpoint spawns at */
        for (s->route = 0; s->route < n && !(r[s->route].kind == P04_STEP_USE && r[s->route].flag == (s->checkpoint >= 2 ? F_CRYSTAL1 : F_CRYSTAL0)); ++s->route) {
        }
        s->route = s->route < n ? s->route + 1 : 0;
    }
    s->route_base = s->route;
    quip(s, s->checkpoint >= 1 ? "Back in the cold. The gears are close." : "Three Sun Gears. One glacier. Let's go, Mara.");
    say(s, "F1 SHOWS THE CONTROLS");
    return true;
}

/* ---- rules ------------------------------------------------------------------------------- */

static void hurt(p04_state *s, int dmg)
{
    float k = s->cfg.difficulty == CADV_DIFF_STORY ? 0.4f : (s->cfg.difficulty == CADV_DIFF_HARD ? 1.4f : 1.0f);
    if (s->win_timer > 0 || s->hp <= 0) {
        return;
    }
    s->hp -= cadv_maxi(1, (int)((float)dmg * k + 0.5f));
    s->hurt_flash = 12;
    sfx(s, CADV_SFX_HIT, 0.8f);
}

static void reset_chase(p04_state *s)
{
    int i;
    /* the gallery floor comes back and the Colossus returns to its alcove */
    for (i = 0; i < P04_MAX_ENEMIES; ++i) {
        if (s->enemies[i].active && s->enemies[i].type == P04_COLOSSUS) {
            s->enemies[i].pos = s->enemies[i].home;
        }
    }
    cadv_tiles_fill(&s->tiles, 32, 0, 4, 37, 5, 44, CADV_TILE_ROCK);
    carve(&s->tiles, 32, 4, 16, 37, 5, 17);
    if (s->chase) {
        s->gear_bits &= ~4u;
        s->gears = ((s->gear_bits & 1u) != 0) + ((s->gear_bits & 2u) != 0);
        s->pickups[2].active = true;
    }
    s->chase = false;
    s->collapse_z = -1.0f;
}

static void die(p04_state *s, const char *why)
{
    ++s->deaths;
    reset_chase(s);
    s->hp = P04_HP;
    cadv_climber_spawn(&s->me, &s->cdef, s->respawn, s->respawn_yaw);
    say(s, why);
    sfx(s, CADV_SFX_DENIED, 1.0f);
    {
        int n;
        int i;
        const p04_step *r = cadv_p04_route(&n);
        /* the bot picks up its route right after the crystal it saved at */
        s->route = s->route_base;
        for (i = 0; i < n && s->crystal >= 0; ++i) {
            if (r[i].kind == P04_STEP_USE && r[i].flag == F_CRYSTAL0 + s->crystal) {
                s->route = i + 1;
            }
        }
    }
}

static bool facing_point(const p04_state *s, cadv_vec3 p, float range)
{
    cadv_vec3 e = cadv_climber_eye(&s->me, &s->cdef);
    cadv_vec3 to = cadv_v3_sub(p, e);
    float d = cadv_v3_len(to);
    cadv_vec3 f = cadv_climber_forward(&s->me);
    return d < range && (d < 1.2f || cadv_v3_dot(cadv_v3_scale(to, 1.0f / d), f) > 0.6f);
}

static void use_things(p04_state *s)
{
    int i;
    cadv_vec3 feet = s->me.pos;
    for (i = 0; i < P04_LEVERS; ++i) {
        if (!s->lever[i] && cadv_v3_dist(feet, k_levers[i]) < 1.8f && facing_point(s, cadv_v3_add(k_levers[i], cadv_v3(0, 0.6f, 0)), 2.5f)) {
            s->lever[i] = true;
            sfx(s, CADV_SFX_DOOR, 1.0f);
            if (i == P04_LEVER_HALL) {
                s->door_timer = s->cfg.difficulty == CADV_DIFF_STORY ? 25 * 60 : (s->cfg.difficulty == CADV_DIFF_HARD ? 11 * 60 : 14 * 60);
                open_door(s, true);
                say(s, "A DOOR GRINDS OPEN SOMEWHERE - HURRY!");
            } else if (i == P04_LEVER_POOL) {
                cadv_tiles_fill(&s->tiles, 15, 2, 18, 15, 4, 19, CADV_TILE_WATER);
                say(s, "THE GRATE IN THE TUNNEL SLIDES AWAY");
            } else {
                s->vent_timer = 150;
                say(s, "WARM AIR ROARS FROM THE VENTS...");
            }
            return;
        }
    }
    for (i = 0; i < P04_CRYSTALS; ++i) {
        if (cadv_v3_dist(feet, k_crystals[i]) < 1.8f) {
            ++s->crystal_uses;
            s->crystal = i;
            s->crystal_used[i] = true;
            s->respawn = cadv_v3(k_crystals[i].x, k_crystals[i].y, k_crystals[i].z - 1.0f);
            s->respawn_yaw = s->me.yaw;
            s->hp = P04_HP;
            say(s, "SAVE CRYSTAL - PROGRESS SAVED");
            sfx(s, CADV_SFX_SOCKET, 1.0f);
            return;
        }
    }
    {
        float yaw;
        cadv_vec3 j = cadv_p04_plaque_pos(P04_PLAQUE_JOURNAL, &yaw);
        if (cadv_v3_dist(cadv_climber_eye(&s->me, &s->cdef), j) < 2.5f) {
            say(s, "\"WARM AIR FROM THE VENTS MELTS THE ICE.\"");
            return;
        }
    }
    if (s->gears == 3 && !s->gears_set && cadv_v3_dist(feet, k_solar) < 2.8f) {
        s->gears_set = true;
        s->star_active = true;
        s->star_pos = cadv_v3(k_solar.x, k_solar.y + 1.4f, k_solar.z - 1.5f);
        say(s, "THE SUN GEARS LOCK IN - A STAR FORMS IN THE LIGHT");
        quip(s, "Spring, at last.");
        sfx(s, CADV_SFX_BELL, 1.0f);
    } else if (s->gears < 3 && cadv_v3_dist(feet, k_solar) < 2.8f) {
        char buf[64];
        snprintf(buf, sizeof buf, "THE SOLAR DOOR WANTS THREE SUN GEARS (%d/3)", s->gears);
        say(s, buf);
    }
}

static void pickups(p04_state *s)
{
    int i;
    cadv_vec3 c = cadv_v3_add(s->me.pos, cadv_v3(0, 0.8f, 0));
    for (i = 0; i < s->npickups; ++i) {
        p04_pickup *p = &s->pickups[i];
        if (!p->active || cadv_v3_dist(c, cadv_v3_add(p->pos, cadv_v3(0, 0.5f, 0))) > 1.1f) {
            continue;
        }
        if (p->kind == P04_PK_HEALTH && s->hp >= P04_HP) {
            continue;
        }
        p->active = false;
        sfx(s, CADV_SFX_PICKUP, 1.0f);
        if (p->kind == P04_PK_GEAR) {
            take_gear(s, p->value);
            if (p->value == 0) {
                s->door_jammed = true;
                open_door(s, true);
                say(s, "SUN GEAR 1 - THE DOOR JAMS OPEN");
                if (s->checkpoint < 1) {
                    s->checkpoint = 1;
                }
            } else if (p->value == 1) {
                say(s, "SUN GEAR 2!");
                if (s->checkpoint < 2) {
                    s->checkpoint = 2;
                }
            } else {
                s->chase = true;
                s->collapse_z = 4.0f;
                say(s, "SUN GEAR 3 - THE GALLERY SHAKES... RUN!");
                quip(s, "That is a very large frozen... RUN, MARA!");
                sfx(s, CADV_SFX_EXPLOSION, 1.0f);
            }
        } else if (p->kind == P04_PK_JADE) {
            s->jade_bits |= 1u << (unsigned)p->value;
            ++s->jade;
            say(s, "A JADE ORNAMENT - SECRET FOUND");
        } else {
            s->hp = cadv_mini(P04_HP, s->hp + p->value);
            say(s, "MEDICAL KIT");
        }
    }
}

static int auto_target(const p04_state *s)
{
    cadv_vec3 e = cadv_climber_eye(&s->me, &s->cdef);
    cadv_vec3 f = cadv_climber_forward(&s->me);
    int best = -1;
    float bd = 24.0f;
    int i;
    for (i = 0; i < P04_MAX_ENEMIES; ++i) {
        const p04_enemy *en = &s->enemies[i];
        cadv_vec3 c;
        cadv_vec3 to;
        float d;
        if (!en->active || en->type == P04_COLOSSUS) {
            continue;
        }
        c = cadv_v3_add(en->pos, cadv_v3(0, en->type == P04_BAT ? 0.0f : 0.6f, 0));
        to = cadv_v3_sub(c, e);
        d = cadv_v3_len(to);
        if (d < bd && cadv_v3_dot(cadv_v3_scale(to, 1.0f / d), f) > 0.75f && cadv_tiles_line_clear(&s->tiles, e, c)) {
            bd = d;
            best = i;
        }
    }
    return best;
}

static void shoot(p04_state *s, const cadv_input_frame *in)
{
    s->gun_cd -= P04_DT;
    if (!cadv_input_down(in, CADV_ACT_FIRE) || s->gun_cd > 0.0f || s->me.state == CADV_CLIMB_HANG ||
        s->me.state == CADV_CLIMB_PULLUP || s->me.state == CADV_CLIMB_SWIM) {
        return;
    }
    s->gun_cd = 0.2f;
    s->gun_side = 1 - s->gun_side;
    s->muzzle = 4;
    sfx(s, CADV_SFX_SHOT, 0.5f);
    {
        int t = auto_target(s);
        if (t >= 0) {
            p04_enemy *e = &s->enemies[t];
            e->hp -= 6;
            e->hurt = 6;
            if (e->hp <= 0) {
                e->active = false;
            }
        }
    }
}

static void tick_enemies(p04_state *s)
{
    cadv_vec3 me = s->me.pos;
    int i;
    for (i = 0; i < P04_MAX_ENEMIES; ++i) {
        p04_enemy *e = &s->enemies[i];
        cadv_vec3 to;
        float d;
        float speed;
        if (!e->active) {
            continue;
        }
        if (e->hurt > 0) {
            --e->hurt;
        }
        e->cooldown -= P04_DT;
        to = cadv_v3_sub(me, e->pos);
        d = cadv_v3_len(to);
        if (e->type == P04_COLOSSUS) {
            if (!s->chase) {
                continue;
            }
            speed = s->cfg.difficulty == CADV_DIFF_STORY ? 2.9f : (s->cfg.difficulty == CADV_DIFF_HARD ? 4.0f : 3.5f);
            if (e->pos.z < 43.0f) {
                e->pos.z += speed * P04_DT;
                e->pos.x += cadv_clampf(me.x - e->pos.x, -1.0f, 1.0f) * P04_DT;
            }
            e->yaw = 0.0f + CADV_PI;
            if (cadv_absf(me.z - e->pos.z) < 2.2f && cadv_absf(me.x - e->pos.x) < 3.0f && me.y > 3.0f) {
                die(s, "THE COLOSSUS CAUGHT YOU...");
                return;
            }
            continue;
        }
        if (e->type == P04_BAT) {
            cadv_vec3 goal = d < 14.0f && cadv_tiles_line_clear(&s->tiles, e->pos, cadv_v3_add(me, cadv_v3(0, 1.4f, 0)))
                                 ? cadv_v3_add(me, cadv_v3(0, 1.5f, 0))
                                 : e->home;
            cadv_vec3 step = cadv_v3_sub(goal, e->pos);
            float l = cadv_v3_len(step);
            if (l > 0.1f) {
                cadv_vec3 next = cadv_v3_add(e->pos, cadv_v3_scale(step, cadv_minf(l, 4.0f * P04_DT) / l));
                if (!cadv_tile_solid(cadv_tiles_at(&s->tiles, next))) {
                    e->pos = next;
                }
                e->yaw = cadv_atan2f(-step.x, -step.z);
            }
            if (d < 1.9f && e->cooldown <= 0.0f) {
                hurt(s, 5);
                e->cooldown = 1.2f;
            }
            continue;
        }
        /* wolves and the bear: chase on the floor they stand on, never over an edge */
        speed = e->type == P04_BEAR ? 2.4f : 3.3f;
        if (d < 12.0f && cadv_absf(to.y) < 1.5f && cadv_tiles_line_clear(&s->tiles, cadv_v3_add(e->pos, cadv_v3(0, 0.6f, 0)),
                                                                           cadv_v3_add(me, cadv_v3(0, 0.8f, 0)))) {
            float h = cadv_sqrtf(to.x * to.x + to.z * to.z);
            if (h > 1.1f) {
                cadv_vec3 next = cadv_v3(e->pos.x + to.x / h * speed * P04_DT, e->pos.y, e->pos.z + to.z / h * speed * P04_DT);
                if (cadv_tiles_floor(&s->tiles, next.x, e->pos.y + 0.2f, next.z) == e->pos.y &&
                    !cadv_tile_solid(cadv_tiles_at(&s->tiles, cadv_v3_add(next, cadv_v3(0, 0.3f, 0))))) {
                    e->pos = next;
                }
            }
            e->yaw = cadv_atan2f(-to.x, -to.z);
            if (h < 1.6f && e->cooldown <= 0.0f) {
                hurt(s, e->type == P04_BEAR ? 20 : 8);
                e->cooldown = e->type == P04_BEAR ? 1.6f : 1.0f;
            }
        }
    }
}

static void check_marks(p04_state *s)
{
    int i;
    cadv_vec3 e = cadv_climber_eye(&s->me, &s->cdef);
    cadv_vec3 f = cadv_climber_forward(&s->me);
    for (i = 0; i < P04_PLAQUES; ++i) {
        float yaw;
        cadv_vec3 p = cadv_p04_plaque_pos(i, &yaw);
        cadv_vec3 to = cadv_v3_sub(p, e);
        float d = cadv_v3_len(to);
        if (!(s->marks_found & (1u << (unsigned)i)) && d < 4.5f && cadv_v3_dot(cadv_v3_scale(to, 1.0f / d), f) > 0.7f) {
            char buf[96];
            s->marks_found |= 1u << (unsigned)i;
            snprintf(buf, sizeof buf, "\"%s\" - A FRIEND'S MARK!", s->mark_text[i]);
            say(s, buf);
        }
    }
}

static void world_rules(p04_state *s)
{
    cadv_vec3 b = cadv_p04_blade_pos(s);
    /* the timed door */
    if (s->door_timer > 0 && !s->door_jammed && --s->door_timer == 0) {
        int px = (int)s->me.pos.x;
        int pz = (int)s->me.pos.z;
        if (!(px == 5 && pz >= 50 && pz <= 52)) {
            open_door(s, false);
            s->lever[P04_LEVER_HALL] = false; /* the lever springs back */
            say(s, "THE DOOR SLAMS SHUT - PULL THE LEVER AGAIN");
        } else {
            s->door_timer = 1;
        }
    }
    if (s->vent_timer > 0 && --s->vent_timer == 0) {
        cadv_tiles_fill(&s->tiles, 32, 6, 28, 37, 12, 29, CADV_TILE_AIR);
        s->ice_melted = true;
        say(s, "THE ICE WALL MELTS AWAY!");
        sfx(s, CADV_SFX_DOOR, 1.0f);
    }
    /* the swinging blade */
    if (cadv_absf(s->me.pos.x - b.x) < 0.65f && cadv_absf(s->me.pos.z - b.z) < 0.55f && s->me.pos.y < 8.0f && s->me.pos.y > 5.0f &&
        (s->ticks % 20) == 0) {
        hurt(s, 35);
        say(s, "THE BLADE!");
    }
    /* the gallery collapses behind the Colossus */
    if (s->chase && s->collapse_z >= 0.0f) {
        float speed = s->cfg.difficulty == CADV_DIFF_STORY ? 2.8f : (s->cfg.difficulty == CADV_DIFF_HARD ? 3.8f : 3.3f);
        int before = (int)s->collapse_z;
        s->collapse_z = cadv_minf(43.0f, s->collapse_z + speed * P04_DT);
        if ((int)s->collapse_z > before) {
            carve(&s->tiles, 32, 0, before, 37, 5, (int)s->collapse_z - 1);
        }
    }
    if (s->chase && s->me.pos.z > 44.5f) {
        s->chase = false;
        s->collapse_z = -1.0f;
        say(s, "SAFE - THE SOLAR CHAMBER HOLDS");
    }
}

void cadv_p04_tick(void *state, const cadv_input_frame *in)
{
    p04_state *s = (p04_state *)state;
    uint32_t ev;
    int n;
    const p04_step *route = cadv_p04_route(&n);
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
    if (s->hurt_flash > 0) {
        --s->hurt_flash;
    }
    if (s->muzzle > 0) {
        --s->muzzle;
    }
    if (s->win_timer > 0) {
        if (--s->win_timer == 0 || (s->win_timer < 180 && cadv_input_pressed(in, CADV_ACT_USE))) {
            s->outcome = CADV_OUTCOME_WON;
        }
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_PAUSE)) {
        s->outcome = CADV_OUTCOME_QUIT;
        return;
    }
    ev = cadv_climber_tick(&s->me, &s->cdef, in, &s->tiles, P04_DT);
    if (ev & CADV_CLIMB_EV_LAND) {
        if (s->me.last_fall > 4.5f) {
            hurt(s, (int)((s->me.last_fall - 4.5f) * 30.0f) + 10);
        }
        sfx(s, CADV_SFX_LAND, 0.6f);
    }
    if (ev & CADV_CLIMB_EV_JUMP) {
        sfx(s, CADV_SFX_JUMP, 0.5f);
    }
    if (ev & CADV_CLIMB_EV_PUSHED) {
        sfx(s, CADV_SFX_DOOR, 0.5f);
    }
    if (ev & CADV_CLIMB_EV_SPLASH) {
        sfx(s, CADV_SFX_LAND, 0.8f);
    }
    if ((ev & CADV_CLIMB_EV_DROWNING) && s->ticks % 30 == 0) {
        hurt(s, 8);
    }
    if (ev & CADV_CLIMB_EV_ABYSS) {
        die(s, "SWALLOWED BY THE GLACIER...");
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_USE) && (s->me.state == CADV_CLIMB_GROUND || s->me.state == CADV_CLIMB_SWIM)) {
        use_things(s);
    }
    shoot(s, in);
    pickups(s);
    tick_enemies(s);
    world_rules(s);
    check_marks(s);
    if (s->hp <= 0) {
        die(s, "MARA FALLS... BACK TO THE LAST CRYSTAL");
        return;
    }
    if (s->star_active && cadv_v3_dist(cadv_v3_add(s->me.pos, cadv_v3(0, 1.0f, 0)), s->star_pos) < 1.6f) {
        s->star_active = false;
        s->star_taken = true;
        s->win_timer = 300;
        say(s, "SHINING STAR! THE WINTER KING'S TOMB IS OPEN");
        sfx(s, CADV_SFX_STAR, 1.0f);
    }
    /* advance the bot's route */
    while (s->route < n) {
        const p04_step *st = &route[s->route];
        bool done;
        if (st->done == P04_DONE_FLAG) {
            done = cadv_p04_flag(s, st->flag);
        } else {
            float r = st->kind == P04_STEP_JUMP || st->kind == P04_STEP_OUT ? 1.1f : (st->kind == P04_STEP_SWIM ? 0.8f : 0.4f);
            done = (s->me.state == CADV_CLIMB_GROUND || st->kind == P04_STEP_SWIM) &&
                   cadv_absf(s->me.pos.x - st->x) < r && cadv_absf(s->me.pos.z - st->z) < r &&
                   cadv_absf(s->me.pos.y - st->y) < (st->kind == P04_STEP_SWIM ? 0.9f : 0.3f);
        }
        if (!done) {
            break;
        }
        ++s->route;
    }
}

/* ---- results & scripting ------------------------------------------------------------------ */

cadv_portal_result cadv_p04_result(const void *state)
{
    const p04_state *s = (const p04_state *)state;
    cadv_portal_result r;
    memset(&r, 0, sizeof r);
    r.outcome = s->outcome;
    r.star_collected = s->star_taken;
    r.checkpoint = s->checkpoint;
    r.marks_found = s->marks_found;
    r.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        r.snowflakes = 1 + (s->crystal_uses <= 3 ? 1 : 0) + (s->jade == CADV_P04_JADE ? 1 : 0);
    }
    return r;
}

bool cadv_p04_query(const void *state, const char *key, double *out)
{
    const p04_state *s = (const p04_state *)state;
    struct kv { const char *k; double v; };
    const struct kv table[] = {
        {"player.x", s->me.pos.x},
        {"player.y", s->me.pos.y},
        {"player.z", s->me.pos.z},
        {"player.yaw", s->me.yaw},
        {"player.health", s->hp},
        {"player.state", s->me.state},
        {"air", s->me.air},
        {"gears", s->gears},
        {"jade", s->jade},
        {"crystals", s->crystal_uses},
        {"deaths", s->deaths},
        {"door.open", cadv_tiles_get(&s->tiles, 5, 6, 51) == CADV_TILE_AIR},
        {"ice.melted", s->ice_melted},
        {"chase", s->chase},
        {"gears.set", s->gears_set},
        {"star.taken", s->star_taken},
        {"route", s->route},
        {"block", cadv_p04_flag(s, F_BLOCK)},
        {"outcome", (double)s->outcome},
        {"marks", s->marks_found},
        {"ticks", s->ticks},
    };
    size_t i;
    for (i = 0; i < sizeof table / sizeof table[0]; ++i) {
        if (strcmp(key, table[i].k) == 0) {
            *out = table[i].v;
            return true;
        }
    }
    return false;
}

void cadv_p04_teleport(void *state, float x, float y, float z, float yaw)
{
    p04_state *s = (p04_state *)state;
    cadv_climber_spawn(&s->me, &s->cdef, cadv_v3(x, y, z), yaw);
}

/* ---- the explorer's route (the bot follows it; tick() advances it) ------------------------- */

#define PI_ CADV_PI
#define W(x, y, z, run) {P04_STEP_WALK, x, y, z, 0.0f, run, P04_DONE_NEAR, 0}
#define J(x, y, z, yaw, run) {P04_STEP_JUMP, x, y, z, yaw, run, P04_DONE_NEAR, 0}
#define C(x, y, z, yaw) {P04_STEP_CLIMB, x, y, z, yaw, 0, P04_DONE_NEAR, 0}
#define PUSH(x, y, z, yaw, done, flag) {P04_STEP_PUSH, x, y, z, yaw, 0, done, flag}
#define USE(x, y, z, yaw, flag) {P04_STEP_USE, x, y, z, yaw, 0, P04_DONE_FLAG, flag}
#define S(x, y, z) {P04_STEP_SWIM, x, y, z, 0.0f, 0, P04_DONE_NEAR, 0}
#define SF(x, y, z, flag) {P04_STEP_SWIM, x, y, z, 0.0f, 0, P04_DONE_FLAG, flag}
#define OUT(x, y, z, yaw) {P04_STEP_OUT, x, y, z, yaw, 0, P04_DONE_NEAR, 0}
#define WF(x, y, z, flag) {P04_STEP_WALK, x, y, z, 0.0f, 0, P04_DONE_FLAG, flag}
#define WAIT(flag) {P04_STEP_WAIT, 0, 0, 0, 0.0f, 0, P04_DONE_FLAG, flag}

static const p04_step k_route[] = {
    /* A: the jade ledge, the two pits, the wall */
    W(19.5f, 4.0f, 91.4f, 0), C(19.5f, 6.0f, 90.5f, 0.0f), WF(19.5f, 6.0f, 89.6f, F_JADE0),
    W(17.0f, 4.0f, 89.5f, 0), W(15.5f, 4.0f, 88.15f, 0), J(15.5f, 4.0f, 86.2f, 0.0f, 0),
    W(15.5f, 4.0f, 85.1f, 0), J(15.5f, 4.0f, 82.2f, 0.0f, 1), W(15.5f, 4.0f, 81.4f, 0), C(15.5f, 6.0f, 80.5f, 0.0f),
    W(15.5f, 6.0f, 72.0f, 0), W(15.5f, 6.0f, 64.0f, 0),
    /* B: the block puzzle */
    W(17.5f, 6.0f, 55.0f, 0), W(19.5f, 6.0f, 55.6f, 0), PUSH(19.5f, 6.0f, 60.68f, PI_, P04_DONE_NEAR, 0),
    W(18.4f, 6.0f, 60.4f, 0), W(18.6f, 6.0f, 61.5f, 0), PUSH(18.6f, 6.0f, 61.5f, -0.5f * PI_, P04_DONE_FLAG, F_BLOCK),
    C(21.5f, 7.0f, 61.5f, -0.5f * PI_), C(22.5f, 9.0f, 61.5f, -0.5f * PI_),
    /* jade on the pillar: running jumps there and back */
    W(22.5f, 9.0f, 62.5f, 0), W(22.5f, 9.0f, 60.1f, 1), J(22.5f, 9.0f, 57.1f, 0.0f, 1), WF(22.5f, 9.0f, 56.6f, F_JADE2),
    W(22.5f, 9.0f, 57.9f, 0), J(22.5f, 9.0f, 60.9f, PI_, 1),
    /* the lever and the race to the timed door */
    W(23.8f, 9.0f, 61.5f, 0), USE(23.8f, 9.0f, 61.5f, -0.5f * PI_, F_LEVER_HALL), W(23.5f, 9.0f, 60.3f, 1),
    W(23.5f, 6.0f, 58.6f, 1), W(9.0f, 6.0f, 50.5f, 1), W(6.0f, 6.0f, 51.5f, 1), WF(2.6f, 6.0f, 51.5f, F_GEAR1),
    W(7.5f, 6.0f, 49.0f, 0), W(9.5f, 6.0f, 47.3f, 0), USE(9.5f, 6.0f, 47.3f, PI_, F_CRYSTAL0),
    /* the blade corridor */
    W(14.5f, 6.0f, 43.0f, 0), W(14.5f, 6.0f, 41.2f, 0), WAIT(F_BLADE_CLEAR), W(14.5f, 6.0f, 37.6f, 1),
    /* C: the pool lever, the tunnel, gear 2, back up */
    W(14.5f, 6.0f, 34.5f, 0), W(20.5f, 6.0f, 33.4f, 0), S(20.5f, 4.0f, 31.0f), S(28.5f, 2.3f, 25.3f),
    {P04_STEP_USE, 28.5f, 2.3f, 25.3f, 0.0f, 0, P04_DONE_FLAG, F_LEVER_POOL},
    S(18.0f, 3.0f, 19.0f), S(14.5f, 3.0f, 19.0f), S(10.5f, 3.0f, 19.0f), S(8.5f, 3.0f, 19.5f), SF(8.5f, 2.3f, 21.3f, F_JADE1),
    S(6.5f, 4.6f, 15.5f), OUT(5.5f, 6.0f, 15.5f, 0.5f * PI_), WF(4.2f, 6.0f, 15.2f, F_GEAR2),
    W(5.6f, 6.0f, 15.5f, 0), S(7.0f, 3.5f, 16.5f), S(8.5f, 3.0f, 19.0f), S(14.5f, 3.0f, 19.0f), S(18.0f, 3.0f, 19.0f),
    S(20.5f, 4.6f, 32.4f), OUT(20.5f, 6.0f, 33.5f, PI_),
    W(27.5f, 6.0f, 33.6f, 0), USE(27.5f, 6.0f, 33.6f, -0.5f * PI_, F_CRYSTAL1),
    /* D: crystal, vent lever, the ice melts, north over the pit, gear 3 */
    W(31.5f, 6.0f, 34.5f, 0), W(35.5f, 6.0f, 36.8f, 0), USE(35.5f, 6.0f, 36.8f, PI_, F_CRYSTAL2),
    W(35.6f, 6.0f, 32.4f, 0), USE(35.6f, 6.0f, 32.4f, -0.5f * PI_, F_LEVER_VENT), WAIT(F_ICE),
    W(34.5f, 6.0f, 26.0f, 0), W(34.5f, 6.0f, 18.15f, 0), J(34.5f, 6.0f, 15.2f, 0.0f, 1), W(34.5f, 6.0f, 8.0f, 0),
    WF(34.5f, 6.0f, 6.6f, F_GEAR3),
    /* the chase back to the solar door */
    W(34.5f, 6.0f, 15.85f, 1), J(34.5f, 6.0f, 18.8f, PI_, 1), W(34.5f, 6.0f, 46.5f, 1),
    W(34.5f, 6.0f, 49.8f, 0), USE(34.5f, 6.0f, 49.8f, PI_, F_GEARS_SET), WF(34.5f, 6.0f, 50.0f, F_STAR),
};

const p04_step *cadv_p04_route(int *count)
{
    *count = (int)(sizeof k_route / sizeof k_route[0]);
    return k_route;
}

/* ---- the bot ------------------------------------------------------------------------------- */

static void bot_press(cadv_input_frame *out, cadv_action a)
{
    out->pressed |= CADV_ACT_BIT(a);
    out->held |= CADV_ACT_BIT(a);
}

static float bot_turn(const p04_state *s, float want_yaw, float want_pitch, cadv_input_frame *out)
{
    float dy = cadv_wrap_angle(want_yaw - s->me.yaw);
    float dp = want_pitch - s->me.pitch;
    out->look_x = cadv_clampf(dy, -0.2f, 0.2f);
    out->look_y = cadv_clampf(dp, -0.12f, 0.12f);
    return cadv_absf(dy);
}

static float bot_go(const p04_state *s, float x, float z, int run, cadv_input_frame *out)
{
    float dx = x - s->me.pos.x;
    float dz = z - s->me.pos.z;
    float d = cadv_sqrtf(dx * dx + dz * dz);
    float err = bot_turn(s, cadv_atan2f(-dx, -dz), 0.0f, out);
    if (d > 0.05f && err < 0.5f) {
        out->move_y = cadv_minf(1.0f, d * (run ? 1.2f : 2.0f));
        if (run && d > 0.6f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_RUN);
        }
    }
    return d;
}

void cadv_p04_bot(const void *state, cadv_input_frame *out)
{
    const p04_state *s = (const p04_state *)state;
    const cadv_climber *me = &s->me;
    int n;
    const p04_step *route = cadv_p04_route(&n);
    const p04_step *st;
    int i;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        if (s->win_timer < 170) {
            bot_press(out, CADV_ACT_USE);
        }
        return;
    }
    if (s->route >= n) {
        return;
    }
    st = &route[s->route];
    /* fight wolves, bats and the bear when they come close (never mid-chase) */
    if (!s->chase && me->state == CADV_CLIMB_GROUND && st->kind != P04_STEP_JUMP) {
        cadv_vec3 eye = cadv_climber_eye(me, &s->cdef);
        for (i = 0; i < P04_MAX_ENEMIES; ++i) {
            const p04_enemy *e = &s->enemies[i];
            cadv_vec3 c;
            cadv_vec3 to;
            if (!e->active || e->type == P04_COLOSSUS) {
                continue;
            }
            c = cadv_v3_add(e->pos, cadv_v3(0, e->type == P04_BAT ? 0.0f : 0.6f, 0));
            to = cadv_v3_sub(c, eye);
            if (cadv_v3_len(to) < 11.0f && cadv_tiles_line_clear(&s->tiles, eye, c)) {
                float err = bot_turn(s, cadv_atan2f(-to.x, -to.z), cadv_atan2f(to.y, cadv_sqrtf(to.x * to.x + to.z * to.z)), out);
                if (err < 0.3f) {
                    out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
                }
                return;
            }
        }
    }
    switch (st->kind) {
    case P04_STEP_WALK:
        if (me->state == CADV_CLIMB_SWIM) {
            out->move_y = 1.0f; /* step out of the water first */
        }
        bot_go(s, st->x, st->z, st->run, out);
        break;
    case P04_STEP_JUMP:
        if (me->state == CADV_CLIMB_GROUND) {
            if (bot_turn(s, st->yaw, 0.0f, out) < 0.05f) {
                out->move_y = 1.0f;
                if (st->run) {
                    out->held |= CADV_ACT_BIT(CADV_ACT_RUN);
                }
                bot_press(out, CADV_ACT_JUMP);
            }
        }
        break;
    case P04_STEP_CLIMB:
        out->held |= CADV_ACT_BIT(CADV_ACT_USE);
        if (me->state == CADV_CLIMB_HANG) {
            out->move_y = 1.0f;
        } else if (me->state == CADV_CLIMB_GROUND && bot_turn(s, st->yaw, 0.0f, out) < 0.05f) {
            out->move_y = 0.4f; /* lean into the wall */
            if (s->ticks % 20 == 0) {
                bot_press(out, CADV_ACT_JUMP);
            }
        }
        break;
    case P04_STEP_PUSH:
        if (me->state == CADV_CLIMB_GROUND && bot_turn(s, st->yaw, 0.0f, out) < 0.05f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_USE);
            out->move_y = 1.0f;
        }
        break;
    case P04_STEP_USE:
        if (me->state == CADV_CLIMB_SWIM) {
            cadv_vec3 lever = cadv_p04_lever_pos(P04_LEVER_POOL);
            cadv_vec3 to = cadv_v3_sub(cadv_v3_add(lever, cadv_v3(0, 0.6f, 0)), cadv_climber_eye(me, &s->cdef));
            if (bot_turn(s, cadv_atan2f(-to.x, -to.z), cadv_atan2f(to.y, cadv_sqrtf(to.x * to.x + to.z * to.z)), out) < 0.1f &&
                s->ticks % 10 == 0) {
                bot_press(out, CADV_ACT_USE);
            }
            if (me->pos.y > 2.6f) {
                out->held |= CADV_ACT_BIT(CADV_ACT_CROUCH);
            }
        } else if (bot_go(s, st->x, st->z, 0, out) < 0.4f) {
            out->move_y = 0.0f;
            if (bot_turn(s, st->yaw, -0.45f, out) < 0.08f && s->ticks % 10 == 0) {
                bot_press(out, CADV_ACT_USE);
            }
        }
        break;
    case P04_STEP_SWIM:
        if (me->state != CADV_CLIMB_SWIM) {
            bot_go(s, st->x, st->z, 0, out); /* walk off into the water */
            out->move_y = 1.0f;
        } else {
            cadv_vec3 to = cadv_v3_sub(cadv_v3(st->x, st->y, st->z), me->pos);
            float h = cadv_sqrtf(to.x * to.x + to.z * to.z);
            float err = bot_turn(s, cadv_atan2f(-to.x, -to.z), cadv_clampf(cadv_atan2f(to.y, h), -0.9f, 0.9f), out);
            if (err < 0.6f && h > 0.2f) {
                out->move_y = cadv_minf(1.0f, h + 0.3f);
            }
            if (to.y > 0.3f) {
                out->held |= CADV_ACT_BIT(CADV_ACT_JUMP);
            } else if (to.y < -0.3f) {
                out->held |= CADV_ACT_BIT(CADV_ACT_CROUCH);
            }
        }
        break;
    case P04_STEP_OUT:
        if (me->state == CADV_CLIMB_SWIM && bot_turn(s, st->yaw, 0.0f, out) < 0.08f) {
            out->move_y = 0.5f;
            out->held |= CADV_ACT_BIT(CADV_ACT_JUMP);
            if (s->ticks % 15 == 0) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_JUMP);
            }
        }
        break;
    default: /* wait */
        break;
    }
}

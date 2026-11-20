/* SPDX-License-Identifier: MIT */
/* Day 7 - Tinsel Strike: best-of-9 rounds of 5v5 in a snowy market town:
 * buy phase and economy, a glitter bomb for two sites, recoil and movement
 * inaccuracy, flash and smoke, footsteps, bots with roles and callouts. */
#include "portals/p07_tinsel_strike.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "hub/friends.h"
#include "portals/p07_internal.h"

/* ---- tables (original weapons) -------------------------------------------------------- */

static const p07_weapon_def k_weapons[P07_WEAPONS] = {
    /* name         price dmg pel rate  auto   mag res  range spread move   head  speed recoil */
    {"CANDY CANE", 0, 55, 1, 0.5f, true, 0, 0, 1.8f, 0.0f, 0.0f, 1.0f, 1.0f, {0.0f, 0.0f, 1}},
    {"PFEFFER P9", 0, 26, 1, 0.16f, false, 12, 36, 60.0f, 0.012f, 0.05f, 3.0f, 1.0f, {0.012f, 0.01f, 4}},
    {"GLOCKE", 700, 48, 1, 0.3f, false, 7, 28, 70.0f, 0.01f, 0.07f, 3.0f, 0.96f, {0.03f, 0.01f, 3}},
    {"LEBKUCHEN SMG", 1250, 22, 1, 0.075f, true, 30, 90, 50.0f, 0.02f, 0.035f, 2.5f, 0.96f, {0.008f, 0.02f, 8}},
    {"NUSSKNACKER", 1100, 14, 8, 0.85f, false, 7, 28, 22.0f, 0.06f, 0.03f, 1.5f, 0.92f, {0.05f, 0.0f, 1}},
    {"STOLLEN RIFLE", 2700, 34, 1, 0.1f, true, 30, 90, 90.0f, 0.006f, 0.06f, 4.0f, 0.9f, {0.014f, 0.03f, 9}},
    {"EISZAPFEN", 4750, 115, 1, 1.4f, false, 5, 20, 120.0f, 0.08f, 0.15f, 1.5f, 0.84f, {0.06f, 0.0f, 1}},
};

static const char *const k_buy_names[P07_BUY_ITEMS] = {"GLOCKE PISTOL", "LEBKUCHEN SMG", "NUSSKNACKER", "STOLLEN RIFLE",
                                                       "EISZAPFEN", "KEVLAR VEST", "DEFUSE KIT", "FLASHBANG", "SMOKE"};
static const int k_buy_price[P07_BUY_ITEMS] = {700, 1250, 1100, 2700, 4750, 650, 400, 200, 300};

const p07_weapon_def *cadv_p07_weapons(void) { return k_weapons; }
const char *cadv_p07_buy_name(int item) { return item >= 0 && item < P07_BUY_ITEMS ? k_buy_names[item] : "?"; }
int cadv_p07_buy_price(int item) { return item >= 0 && item < P07_BUY_ITEMS ? k_buy_price[item] : 0; }

/* walkable streets (cells, inclusive): x0, z0, x1, z1 */
static const int k_streets[][4] = {
    {2, 1, 37, 3},    /* north street (Guard side) */
    {16, 1, 23, 6},   /* Guard yard */
    {2, 4, 11, 12},   /* B plaza */
    {28, 4, 37, 12},  /* A plaza */
    {11, 9, 15, 11},  /* B short */
    {24, 9, 28, 11},  /* A short */
    {14, 10, 25, 20}, /* the market square */
    {18, 20, 21, 27}, /* square lane */
    {14, 26, 25, 30}, /* Humbug yard */
    {2, 28, 37, 30},  /* south street */
    {2, 12, 4, 28},   /* B alley */
    {34, 12, 37, 28}, /* A long */
    {19, 6, 20, 10},  /* the window passage */
};
static const int k_crates[][2] = {{6, 8}, {7, 8}, {4, 6}, {9, 11}, {32, 7}, {33, 7}, {30, 10}, {35, 10}, {16, 12}, {23, 12},
                                  {16, 18}, {23, 18}, {36, 17}, {35, 24}, {3, 20}, {28, 2}, {11, 2}};
/* the bomb sites (cells) */
static const int k_sites[2][4] = {{29, 5, 36, 11}, {3, 5, 10, 11}};
static const int k_spawn[2][P07_TEAM][2] = {
    {{15, 28}, {18, 29}, {21, 29}, {24, 28}, {19, 27}}, /* Humbug */
    {{17, 2}, {19, 2}, {21, 2}, {18, 4}, {20, 4}},      /* Guard */
};
/* where Guard anchors wait (cells) and where they watch */
static const int k_hold[6][2] = {{31, 9}, {35, 6}, {6, 10}, {9, 6}, {20, 11}, {17, 3}};
static const char *const k_filler[4] = {"BAUBLE", "NOGGIN", "TWINKLE", "SPRAT"};

/* ---- helpers ---------------------------------------------------------------------- */

static void copy_text(char *out, size_t cap, const char *text)
{
    size_t i;
    for (i = 0; text[i] && i + 1 < cap; ++i) {
        out[i] = text[i];
    }
    out[i] = '\0';
}

static void say(p07_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 180;
}

static void radio(p07_state *s, const char *text)
{
    copy_text(s->radio, sizeof s->radio, text);
    s->radio_timer = 180;
}

static void sfx_at(p07_state *s, cadv_sfx id, float vol, cadv_vec3 at)
{
    if (s->cfg.audio) {
        float d = cadv_v3_dist(at, s->a[0].body.pos);
        cadv_audio_sfx(s->cfg.audio, id, vol * cadv_clampf(1.0f - d / 50.0f, 0.05f, 1.0f), 0.0f);
    }
}

static void feed(p07_state *s, const char *text)
{
    int i;
    for (i = P07_FEED - 1; i > 0; --i) {
        memcpy(s->feed[i], s->feed[i - 1], sizeof s->feed[i]);
        s->feed_t[i] = s->feed_t[i - 1];
    }
    copy_text(s->feed[0], sizeof s->feed[0], text);
    s->feed_t[0] = 360;
}

static float cell_c(int c) { return ((float)c + 0.5f) * P07_CELL; }

static float flat(cadv_vec3 a, cadv_vec3 b) { return cadv_sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

static float yaw_to(cadv_vec3 from, cadv_vec3 to) { return cadv_atan2f(-(to.x - from.x), -(to.z - from.z)); }

int cadv_p07_side_of(const p07_state *s, int team) { return team == 0 ? s->player_side : 1 - s->player_side; }

static int team_of_side(const p07_state *s, int side) { return side == s->player_side ? 0 : 1; }

cadv_vec3 cadv_p07_eye(const p07_state *s, int agent) { return cadv_player_eye(&s->a[agent].body); }

int cadv_p07_alive(const p07_state *s, int team)
{
    int i;
    int n = 0;
    for (i = 0; i < P07_AGENTS; ++i) {
        n += s->a[i].alive && s->a[i].team == team;
    }
    return n;
}

bool cadv_p07_in_site(const p07_state *s, cadv_vec3 p, int *site)
{
    int k;
    (void)s;
    for (k = 0; k < 2; ++k) {
        if (p.x >= (float)k_sites[k][0] * P07_CELL && p.x <= (float)(k_sites[k][2] + 1) * P07_CELL && p.z >= (float)k_sites[k][1] * P07_CELL &&
            p.z <= (float)(k_sites[k][3] + 1) * P07_CELL) {
            if (site) {
                *site = k;
            }
            return true;
        }
    }
    return false;
}

const char *cadv_p07_area(const p07_state *s, cadv_vec3 p)
{
    static const char *const names[] = {"NORTH STREET", "GUARD YARD", "B SITE", "A SITE", "B SHORT", "A SHORT", "THE SQUARE",
                                        "SQUARE LANE", "HUMBUG YARD", "SOUTH STREET", "B ALLEY", "A LONG", "THE WINDOW"};
    int cx = (int)(p.x / P07_CELL);
    int cz = (int)(p.z / P07_CELL);
    int k;
    int best = -1;
    (void)s;
    /* the smallest street that holds the point names it */
    for (k = 0; k < (int)(sizeof k_streets / sizeof k_streets[0]); ++k) {
        if (cx >= k_streets[k][0] && cx <= k_streets[k][2] && cz >= k_streets[k][1] && cz <= k_streets[k][3]) {
            if (best < 0 || (k_streets[k][2] - k_streets[k][0]) * (k_streets[k][3] - k_streets[k][1]) <
                                (k_streets[best][2] - k_streets[best][0]) * (k_streets[best][3] - k_streets[best][1])) {
                best = k;
            }
        }
    }
    return best >= 0 ? names[best] : "THE MARKET";
}

static bool smoke_blocks(const p07_state *s, cadv_vec3 a, cadv_vec3 b)
{
    int i;
    for (i = 0; i < P07_SMOKES; ++i) {
        if (s->smokes[i].ticks > 0 && cadv_segment_sphere(a, b, s->smokes[i].pos, s->smokes[i].ticks > 60 ? 4.2f : 2.0f)) {
            return true;
        }
    }
    return false;
}

/* Eye to chest or head, through no wall and no smoke. */
bool cadv_p07_can_see(const p07_state *s, int from, int to)
{
    cadv_vec3 e = cadv_p07_eye(s, from);
    cadv_vec3 t = s->a[to].body.pos;
    cadv_vec3 head = cadv_p07_eye(s, to);
    t.y += 1.1f;
    if (smoke_blocks(s, e, head) && smoke_blocks(s, e, t)) {
        return false;
    }
    return cadv_world_line_of_sight(&s->world, e, head) || cadv_world_line_of_sight(&s->world, e, t);
}

/* ---- the market of Glühstadt ------------------------------------------------------------ */

static void build_market(p07_state *s)
{
    int x;
    int z;
    int k;
    memset(s->cells, 1, sizeof s->cells);
    for (k = 0; k < (int)(sizeof k_streets / sizeof k_streets[0]); ++k) {
        for (z = k_streets[k][1]; z <= k_streets[k][3]; ++z) {
            for (x = k_streets[k][0]; x <= k_streets[k][2]; ++x) {
                s->cells[z * P07_W + x] = 0;
            }
        }
    }
    for (k = 0; k < (int)(sizeof k_crates / sizeof k_crates[0]); ++k) {
        s->cells[k_crates[k][1] * P07_W + k_crates[k][0]] = 2;
    }
    for (z = 15; z <= 16; ++z) {
        for (x = 19; x <= 20; ++x) {
            s->cells[z * P07_W + x] = 3; /* the giant tree */
        }
    }
    cadv_world_clear(&s->world);
    cadv_world_add(&s->world, cadv_v3(0, -1, 0), cadv_v3((float)P07_W * P07_CELL, 0, (float)P07_H * P07_CELL), 0);
    for (z = 0; z < P07_H; ++z) {
        for (x = 0; x < P07_W;) {
            int c = s->cells[z * P07_W + x];
            int x1 = x;
            if (c == 0) {
                ++x;
                continue;
            }
            while (x1 + 1 < P07_W && s->cells[z * P07_W + x1 + 1] == c) {
                ++x1;
            }
            cadv_world_add(&s->world, cadv_v3((float)x * P07_CELL, 0, (float)z * P07_CELL),
                           cadv_v3((float)(x1 + 1) * P07_CELL, c == 1 ? P07_HOUSE_H : c == 2 ? P07_CRATE_H : 7.6f, (float)(z + 1) * P07_CELL), c);
            x = x1 + 1;
        }
    }
    cadv_nav_init(&s->nav, P07_W, P07_H, P07_CELL);
    for (z = 0; z < P07_H; ++z) {
        for (x = 0; x < P07_W; ++x) {
            cadv_nav_set(&s->nav, x, z, s->cells[z * P07_W + x] == 0);
        }
    }
    cadv_nav_mark_cover(&s->nav);
    s->star_pos = cadv_v3(cell_c(19) + 1.0f, 10.6f, cell_c(15) + 1.0f);
}

/* ---- buying ------------------------------------------------------------------------------- */

static bool buy(p07_state *s, int i, int item)
{
    p07_agent *a = &s->a[i];
    int price = k_buy_price[item];
    int side = cadv_p07_side_of(s, a->team);
    if (a->money < price) {
        return false;
    }
    switch (item) {
    case P07_BUY_DEAGLE:
        if (a->pistol == P07_DEAGLE) {
            return false;
        }
        a->pistol = P07_DEAGLE;
        a->mag[1] = k_weapons[P07_DEAGLE].mag;
        a->reserve[1] = k_weapons[P07_DEAGLE].reserve;
        a->slot = P07_SLOT_PISTOL;
        break;
    case P07_BUY_ARMOUR:
        if (a->body.armor >= 100) {
            return false;
        }
        a->body.armor = 100;
        break;
    case P07_BUY_KIT:
        if (side != P07_GUARD || a->kit) {
            return false;
        }
        a->kit = true;
        break;
    case P07_BUY_FLASH:
        if (a->flashes >= 2) {
            return false;
        }
        ++a->flashes;
        break;
    case P07_BUY_SMOKE:
        if (a->smokes >= 1) {
            return false;
        }
        ++a->smokes;
        break;
    default: {
        int w = item == P07_BUY_SMG ? P07_SMG : item == P07_BUY_SHOTGUN ? P07_SHOTGUN : item == P07_BUY_RIFLE ? P07_RIFLE : P07_SNIPER;
        if (a->primary == w) {
            return false;
        }
        a->primary = w;
        a->mag[2] = k_weapons[w].mag;
        a->reserve[2] = k_weapons[w].reserve;
        a->slot = P07_SLOT_PRIMARY;
        break;
    }
    }
    a->money -= price;
    return true;
}

/* Bots shop by budget: a rifle and a vest if they can, else an SMG, else save. */
static void bot_buy(p07_state *s, int i)
{
    p07_agent *a = &s->a[i];
    if (a->primary < 0) {
        if (a->money >= 3350) {
            buy(s, i, (a->name % 4 == 3 && a->money >= 5400) ? P07_BUY_SNIPER : P07_BUY_RIFLE);
        } else if (a->money >= 1900 && s->round != 1 && s->round != P07_HALF + 1) {
            buy(s, i, P07_BUY_SMG);
        }
    }
    if (a->money >= 650 && (a->primary >= 0 || s->round == 1 || s->round == P07_HALF + 1)) {
        buy(s, i, P07_BUY_ARMOUR);
    }
    if (a->money >= 900 && cadv_p07_side_of(s, a->team) == P07_GUARD) {
        buy(s, i, P07_BUY_KIT);
    }
    if (a->money >= 500 && a->primary >= 0) {
        buy(s, i, a->role == P07_ROLE_ENTRY || a->role == P07_ROLE_MID ? P07_BUY_SMOKE : P07_BUY_FLASH);
    }
}

/* ---- rounds --------------------------------------------------------------------------------- */

static void strip(p07_agent *a)
{
    a->primary = -1;
    a->pistol = P07_PISTOL;
    a->body.armor = 0;
    a->kit = false;
    a->flashes = 0;
    a->smokes = 0;
}

static void start_round(p07_state *s)
{
    int i;
    int k[2] = {0, 0};
    int carrier_pick;
    s->phase = P07_PH_FREEZE;
    s->timer = P07_FREEZE;
    s->planted = false;
    s->bomb_dropped = false;
    s->winner = -1;
    s->clutch_chance = false;
    s->buy_open = false;
    s->spectate = 0;
    memset(s->smokes, 0, sizeof s->smokes);
    memset(s->nades, 0, sizeof s->nades);
    memset(s->tracers, 0, sizeof s->tracers);
    s->site_choice = (int)cadv_rng_below(&s->rng, 2);
    s->alert_site = -1;
    carrier_pick = (int)cadv_rng_below(&s->rng, P07_TEAM);
    s->carrier = -1;
    for (i = 0; i < P07_AGENTS; ++i) {
        p07_agent *a = &s->a[i];
        int side = cadv_p07_side_of(s, a->team);
        int n = k[side]++;
        int armour = a->body.armor;
        cadv_player_spawn(&a->body, cadv_v3(cell_c(k_spawn[side][n][0]), 0.0f, cell_c(k_spawn[side][n][1])), side == P07_HUMBUG ? 0.0f : CADV_PI);
        a->body.tune.walk_speed = 5.0f;
        a->body.tune.run_speed = 2.6f; /* holding RUN walks quietly */
        if (!a->alive) {
            strip(a);
        } else {
            a->body.armor = armour;
        }
        a->alive = true;
        a->round_kills = 0;
        a->knife_kill = false;
        a->plant_t = a->defuse_t = 0.0f;
        a->blind = 0.0f;
        a->scoped = false;
        a->target = -1;
        a->path_len = a->path_pos = 0;
        a->hold = 0.0f;
        a->heard_t = 0.0f;
        a->stuck = 0;
        a->shot_cd = 0.0f;
        a->slot = a->primary >= 0 ? P07_SLOT_PRIMARY : P07_SLOT_PISTOL;
        a->mag[1] = k_weapons[a->pistol].mag;
        a->reserve[1] = k_weapons[a->pistol].reserve;
        if (a->primary >= 0) {
            a->mag[2] = k_weapons[a->primary].mag;
            a->reserve[2] = k_weapons[a->primary].reserve;
        }
        /* roles: attackers split entry, support and a lurker; defenders anchor */
        if (side == P07_HUMBUG) {
            a->role = n == 0 ? P07_ROLE_ENTRY : n == 3 ? P07_ROLE_LURK : P07_ROLE_SUPPORT;
            if (n == carrier_pick) {
                s->carrier = i;
            }
        } else {
            a->role = n < 2 ? P07_ROLE_ANCHOR_A : n < 4 ? P07_ROLE_ANCHOR_B : P07_ROLE_MID;
        }
        a->look_yaw = a->body.yaw;
    }
    if (s->a[0].role == P07_ROLE_ENTRY) {
        /* the player follows the entry rather than leading it */
        s->a[0].role = s->a[1].role;
        s->a[1].role = P07_ROLE_ENTRY;
    }
    for (i = 1; i < P07_AGENTS; ++i) {
        bot_buy(s, i);
    }
    {
        char buf[72];
        snprintf(buf, sizeof buf, "ROUND %d - %s", s->round, s->player_side == P07_HUMBUG ? "PLANT THE GLITTER BOMB" : "GUARD THE MARKET");
        say(s, buf);
    }
}

static int encode_cp(const p07_state *s) { return 1 + s->score[0] * 16 + s->score[1]; }

static void end_round(p07_state *s, int side, cadv_win_reason why)
{
    static const char *const how[] = {"ALL TAGGED", "THE GLITTER BOMB WENT OFF", "THE BOMB WAS DEFUSED", "TIME RAN OUT"};
    int team = team_of_side(s, side);
    int half = s->round <= P07_HALF ? 0 : 1;
    int i;
    char buf[96];
    s->winner = team;
    s->win_reason = (int)why;
    ++s->score[team];
    ++s->half_won[half][team];
    ++s->side_wins[side];
    s->loss_streak[team] = 0;
    ++s->loss_streak[1 - team];
    for (i = 0; i < P07_AGENTS; ++i) {
        p07_agent *a = &s->a[i];
        bool won = a->team == team;
        bool attacker = cadv_p07_side_of(s, a->team) == P07_HUMBUG;
        a->money = cadv_econ_add(&s->econ, a->money,
                                 cadv_econ_round_reward(&s->econ, won, why, s->loss_streak[a->team], s->planted, attacker));
    }
    if (team == 0 && s->clutch_chance && s->a[0].alive) {
        s->clutch = true;
        say(s, "CLUTCH!");
    }
    if ((s->round == 1 || s->round == P07_HALF + 1) && s->a[0].round_kills >= P07_TEAM) {
        s->ace = true;
    }
    snprintf(buf, sizeof buf, "%s WIN - %s", side == P07_HUMBUG ? "HUMBUG SYNDICATE" : "NUTCRACKER GUARD", how[why]);
    say(s, buf);
    sfx_at(s, CADV_SFX_BELL, 1.0f, s->a[0].body.pos);
    s->phase = P07_PH_AFTER;
    s->timer = P07_AFTER;
    if (s->round == P07_HALF && s->checkpoint == 0 && s->score[0] < P07_TO_WIN && s->score[1] < P07_TO_WIN) {
        s->checkpoint = encode_cp(s);
    }
}

static void build_steps(p07_state *s)
{
    int i;
    cadv_vec3 c = cadv_v3(cell_c(19) + 1.0f, 0, cell_c(15) + 1.0f);
    for (i = 0; i < P07_STEPS; ++i) {
        float a = 2.6f - 0.62f * (float)i;
        float y = 0.7f * (float)(i + 1);
        cadv_vec3 p = cadv_v3(c.x + cadv_cosf(a) * 3.6f, y, c.z + cadv_sinf(a) * 3.6f);
        s->step_pos[i] = p;
        cadv_world_add(&s->world, cadv_v3(p.x - 1.0f, y - 0.3f, p.z - 1.0f), cadv_v3(p.x + 1.0f, y, p.z + 1.0f), 4);
    }
    {
        float top = 0.7f * (float)(P07_STEPS + 1); /* one more step, over the trunk */
        s->star_pos = cadv_v3(c.x, top + 1.0f, c.z);
        cadv_world_add(&s->world, cadv_v3(c.x - 1.7f, top - 0.3f, c.z - 1.7f), cadv_v3(c.x + 1.7f, top, c.z + 1.7f), 4);
    }
    s->steps = true;
}

static void next_round(p07_state *s)
{
    int i;
    if (s->score[0] >= P07_TO_WIN) {
        /* the celebration: a way up the market's giant tree opens */
        s->phase = P07_PH_CELEBRATE;
        s->timer = 90 * 60;
        build_steps(s);
        for (i = 1; i < P07_AGENTS; ++i) {
            s->a[i].alive = s->a[i].team == 0;
        }
        s->a[0].alive = true;
        s->a[0].body.health = 100;
        s->a[0].body.pos = cadv_v3(cell_c(19) + 1.0f, 0.0f, cell_c(19));
        s->a[0].body.yaw = 0.0f;
        say(s, "MATCH WON! CLIMB THE TREE TO THE SHINING STAR");
        radio(s, "THE WHOLE MARKET IS CHEERING - THE TREE STEPS ARE OPEN!");
        sfx_at(s, CADV_SFX_STAR, 1.0f, s->a[0].body.pos);
        return;
    }
    if (s->score[1] >= P07_TO_WIN) {
        s->phase = P07_PH_OVER;
        s->end_timer = 240;
        say(s, "MATCH LOST");
        return;
    }
    ++s->round;
    if (s->round == P07_HALF + 1) {
        /* half time: the teams swap sides, money back to the start */
        s->player_side = 1 - s->player_side;
        for (i = 0; i < P07_AGENTS; ++i) {
            strip(&s->a[i]);
            s->a[i].money = s->econ.start;
            s->a[i].alive = true;
        }
        s->loss_streak[0] = s->loss_streak[1] = 0;
        radio(s, "HALF TIME - SWITCHING SIDES");
    }
    start_round(s);
}

/* ---- combat --------------------------------------------------------------------------------- */

static void tag(p07_state *s, int victim, int killer, int weapon)
{
    p07_agent *v = &s->a[victim];
    char buf[48];
    v->alive = false;
    ++v->deaths;
    s->confetti_pos = v->body.pos;
    s->confetti_t = 60;
    sfx_at(s, CADV_SFX_EXPLOSION, 0.35f, v->body.pos);
    if (killer >= 0) {
        p07_agent *k = &s->a[killer];
        ++k->kills;
        ++k->round_kills;
        k->money = cadv_econ_add(&s->econ, k->money, weapon == P07_KNIFE ? s->econ.knife_kill : s->econ.kill);
        snprintf(buf, sizeof buf, "%.10s > %.10s", s->names[killer], s->names[victim]);
    } else {
        snprintf(buf, sizeof buf, "%.10s POPPED", s->names[victim]);
    }
    feed(s, buf);
    if (victim == s->carrier) {
        s->carrier = -1;
        s->bomb_dropped = true;
        s->bomb_pos = v->body.pos;
    }
    if (victim == 0) {
        int i;
        for (i = 1; i < P07_AGENTS; ++i) {
            if (s->a[i].alive && s->a[i].team == 0) {
                s->spectate = i;
                break;
            }
        }
        say(s, "TAGGED! SPECTATING - E CYCLES");
    }
    if (s->a[0].alive && cadv_p07_alive(s, 0) == 1 && cadv_p07_alive(s, 1) >= 3) {
        s->clutch_chance = true;
    }
}

static void hurt(p07_state *s, int victim, int shooter, int damage, int weapon)
{
    p07_agent *v = &s->a[victim];
    if (!v->alive || damage <= 0) {
        return;
    }
    if (victim == 0) {
        s->hurt_flash = 10;
    }
    if (cadv_player_damage(&v->body, damage)) {
        tag(s, victim, shooter, weapon);
    }
    /* the hit gives the shooter away */
    v->heard = s->a[shooter].body.pos;
    v->heard_t = 3.0f;
}

static void add_tracer(p07_state *s, cadv_vec3 a, cadv_vec3 b)
{
    int i;
    int best = 0;
    for (i = 0; i < 16; ++i) {
        if (s->tracers[i].ttl < s->tracers[best].ttl) {
            best = i;
        }
    }
    s->tracers[best].a = a;
    s->tracers[best].b = b;
    s->tracers[best].ttl = 4;
}

static float rnd_pm(p07_state *s) { return cadv_rng_float01(&s->rng) * 2.0f - 1.0f; }

static void shoot(p07_state *s, int i)
{
    p07_agent *a = &s->a[i];
    int w = a->slot == P07_SLOT_PRIMARY ? a->primary : a->pistol;
    const p07_weapon_def *d = &k_weapons[w];
    int ms = a->slot == P07_SLOT_PRIMARY ? 2 : 1;
    float speed = cadv_sqrtf(a->body.vel.x * a->body.vel.x + a->body.vel.z * a->body.vel.z);
    float spread = cadv_spread(d->spread, d->move_spread, speed, 5.0f, !a->body.on_ground, a->body.crouching);
    float ry0;
    float rp0;
    float ry;
    float rp;
    cadv_vec3 e = cadv_p07_eye(s, i);
    int p;
    if (a->since_shot > 0.4f) {
        a->spray = 0;
    }
    if (w == P07_SNIPER && a->scoped) {
        spread = 0.0015f;
    }
    if (a->blind > 0.5f) {
        spread += 0.15f;
    }
    cadv_recoil_offset(&d->recoil, a->spray, &ry, &rp);
    cadv_recoil_offset(&d->recoil, a->spray > 0 ? a->spray - 1 : 0, &ry0, &rp0);
    for (p = 0; p < d->pellets; ++p) {
        float yaw = a->body.yaw + ry + rnd_pm(s) * spread;
        float pitch = a->body.pitch + rp + rnd_pm(s) * spread;
        cadv_vec3 dir = cadv_v3_from_yaw_pitch(yaw, pitch);
        float wall = d->range;
        float best = d->range;
        int hit = -1;
        int k;
        cadv_world_raycast(&s->world, e, dir, d->range, &wall, NULL, NULL);
        best = wall;
        for (k = 0; k < P07_AGENTS; ++k) {
            const p07_agent *t = &s->a[k];
            float tt;
            if (k == i || !t->alive || t->team == a->team) {
                continue;
            }
            if (cadv_ray_cylinder(e, dir, t->body.pos, 0.35f, cadv_player_height(&t->body), best, &tt)) {
                best = tt;
                hit = k;
            }
        }
        add_tracer(s, cadv_v3_add(e, cadv_v3_scale(dir, 0.6f)), cadv_v3_add(e, cadv_v3_scale(dir, best)));
        if (hit >= 0) {
            const p07_agent *t = &s->a[hit];
            float hy = e.y + dir.y * best - t->body.pos.y;
            bool head = hy > cadv_player_height(&t->body) - 0.3f;
            float dmg = (float)d->damage * (head ? d->head : 1.0f) * (1.0f - 0.3f * best / d->range);
            hurt(s, hit, i, (int)(dmg + 0.5f), w);
        }
    }
    /* the view kicks with part of the pattern; the rest is in the bullets */
    a->body.pitch = cadv_clampf(a->body.pitch + (rp - rp0) * 0.35f, -1.4f, 1.4f);
    a->body.yaw = cadv_wrap_angle(a->body.yaw + (ry - ry0) * 0.35f);
    ++a->spray;
    a->since_shot = 0.0f;
    a->shot_cd = d->rate;
    --a->mag[ms];
    if (a->mag[ms] <= 0 && a->reserve[ms] > 0) {
        int take = cadv_mini(d->mag, a->reserve[ms]);
        a->mag[ms] = take;
        a->reserve[ms] -= take;
        a->shot_cd = 2.2f; /* reloading */
    }
    sfx_at(s, CADV_SFX_SHOT, w == P07_SNIPER || w == P07_SHOTGUN ? 0.9f : 0.5f, a->body.pos);
    /* gunfire is heard far */
    for (p = 0; p < P07_AGENTS; ++p) {
        if (s->a[p].alive && s->a[p].team != a->team && flat(s->a[p].body.pos, a->body.pos) < 35.0f) {
            s->a[p].heard = a->body.pos;
            s->a[p].heard_t = 2.0f;
        }
    }
}

static void stab(p07_state *s, int i)
{
    p07_agent *a = &s->a[i];
    cadv_vec3 e = cadv_p07_eye(s, i);
    cadv_vec3 dir = cadv_player_forward(&a->body);
    int k;
    a->shot_cd = k_weapons[P07_KNIFE].rate;
    for (k = 0; k < P07_AGENTS; ++k) {
        float t;
        if (k != i && s->a[k].alive && s->a[k].team != a->team &&
            cadv_ray_cylinder(e, dir, s->a[k].body.pos, 0.45f, cadv_player_height(&s->a[k].body), k_weapons[P07_KNIFE].range, &t)) {
            hurt(s, k, i, k_weapons[P07_KNIFE].damage, P07_KNIFE);
            return;
        }
    }
}

static void throw_nade(p07_state *s, int i, bool smoke)
{
    p07_agent *a = &s->a[i];
    cadv_vec3 f = cadv_player_forward(&a->body);
    int k;
    for (k = 0; k < P07_NADES; ++k) {
        if (!s->nades[k].active) {
            s->nades[k].active = true;
            s->nades[k].smoke = smoke;
            s->nades[k].owner = i;
            s->nades[k].pos = cadv_p07_eye(s, i);
            s->nades[k].vel = cadv_v3(f.x * 13.0f, f.y * 13.0f + 3.0f, f.z * 13.0f);
            s->nades[k].fuse = smoke ? 1.8f : 1.4f;
            break;
        }
    }
    if (smoke) {
        --a->smokes;
    } else {
        --a->flashes;
    }
    a->slot = a->primary >= 0 ? P07_SLOT_PRIMARY : P07_SLOT_PISTOL;
    a->shot_cd = 0.4f;
}

static void tick_nades(p07_state *s)
{
    int k;
    int i;
    for (k = 0; k < P07_SMOKES; ++k) {
        if (s->smokes[k].ticks > 0) {
            --s->smokes[k].ticks;
        }
    }
    for (k = 0; k < P07_NADES; ++k) {
        p07_nade *n = &s->nades[k];
        cadv_vec3 next;
        float d;
        if (!n->active) {
            continue;
        }
        n->vel.y -= 14.0f * P07_DT;
        next = cadv_v3_add(n->pos, cadv_v3_scale(n->vel, P07_DT));
        if (!cadv_world_line_of_sight(&s->world, n->pos, next)) {
            /* bounce off walls and the ground, losing speed */
            cadv_vec3 dir = cadv_v3_norm(n->vel);
            cadv_vec3 nrm;
            float dist;
            if (cadv_world_raycast(&s->world, n->pos, dir, cadv_v3_len(n->vel) * P07_DT + 0.1f, &dist, &nrm, NULL)) {
                float vn = cadv_v3_dot(n->vel, nrm);
                n->vel = cadv_v3_scale(cadv_v3_sub(n->vel, cadv_v3_scale(nrm, 1.6f * vn)), 0.45f);
            }
            next = n->pos;
        }
        n->pos = next;
        n->fuse -= P07_DT;
        if (n->fuse > 0.0f) {
            continue;
        }
        n->active = false;
        if (n->smoke) {
            for (i = 0; i < P07_SMOKES; ++i) {
                if (s->smokes[i].ticks <= 0) {
                    s->smokes[i].pos = cadv_v3(n->pos.x, 1.6f, n->pos.z);
                    s->smokes[i].ticks = 18 * 60;
                    break;
                }
            }
            sfx_at(s, CADV_SFX_DOOR, 0.6f, n->pos);
            continue;
        }
        /* flash: everyone who sees it and faces it goes white */
        sfx_at(s, CADV_SFX_EXPLOSION, 0.6f, n->pos);
        for (i = 0; i < P07_AGENTS; ++i) {
            p07_agent *a = &s->a[i];
            cadv_vec3 e;
            cadv_vec3 to;
            if (!a->alive) {
                continue;
            }
            e = cadv_p07_eye(s, i);
            d = cadv_v3_dist(e, n->pos);
            if (d > 22.0f || !cadv_world_line_of_sight(&s->world, e, n->pos) || smoke_blocks(s, e, n->pos)) {
                continue;
            }
            to = cadv_v3_norm(cadv_v3_sub(n->pos, e));
            {
                float facing = cadv_v3_dot(to, cadv_player_forward(&a->body));
                float t = facing > 0.3f ? 3.2f * (1.0f - d / 26.0f) : 0.6f;
                a->blind = cadv_maxf(a->blind, t);
            }
        }
    }
}

/* ---- one agent, driven by an input frame ------------------------------------------------- */

static void agent_step(p07_state *s, int i, const cadv_input_frame *in)
{
    p07_agent *a = &s->a[i];
    cadv_input_frame m = *in;
    bool frozen = s->phase == P07_PH_FREEZE;
    int side = cadv_p07_side_of(s, a->team);
    int w;
    uint32_t ev;
    if (!a->alive) {
        return;
    }
    a->since_shot += P07_DT;
    a->shot_cd -= P07_DT;
    if (a->blind > 0.0f) {
        a->blind -= P07_DT;
    }
    if (a->heard_t > 0.0f) {
        a->heard_t -= P07_DT;
    }
    /* weapon slots */
    if (!(i == 0 && s->buy_open)) {
        if (cadv_input_pressed(in, CADV_ACT_WEAPON_1) && a->primary >= 0) {
            a->slot = P07_SLOT_PRIMARY;
        }
        if (cadv_input_pressed(in, CADV_ACT_WEAPON_2)) {
            a->slot = P07_SLOT_PISTOL;
        }
        if (cadv_input_pressed(in, CADV_ACT_WEAPON_3)) {
            a->slot = P07_SLOT_KNIFE;
        }
        if (cadv_input_pressed(in, CADV_ACT_WEAPON_4) && a->flashes > 0) {
            a->slot = P07_SLOT_FLASH;
        }
        if (cadv_input_pressed(in, CADV_ACT_WEAPON_5) && a->smokes > 0) {
            a->slot = P07_SLOT_SMOKE;
        }
        if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
            int step = cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) ? 1 : 2;
            int k;
            for (k = 0; k < 3; ++k) {
                a->slot = (a->slot + step) % 3;
                if (a->slot != P07_SLOT_PRIMARY || a->primary >= 0) {
                    break;
                }
            }
        }
    }
    if (a->slot != P07_SLOT_PRIMARY || a->primary != P07_SNIPER) {
        a->scoped = false;
    } else if (cadv_input_pressed(in, CADV_ACT_ALT_FIRE)) {
        a->scoped = !a->scoped;
    }
    w = a->slot == P07_SLOT_PRIMARY ? a->primary : a->slot == P07_SLOT_PISTOL ? a->pistol : P07_KNIFE;
    if (w < 0) {
        w = P07_KNIFE;
    }
    a->body.tune.walk_speed = (a->scoped ? 2.6f : 5.0f) * k_weapons[w].speed;
    /* planting and defusing pin you in place */
    if (frozen || a->plant_t > 0.0f || a->defuse_t > 0.0f) {
        m.move_x = m.move_y = 0.0f;
        m.pressed &= ~CADV_ACT_BIT(CADV_ACT_JUMP);
    }
    if (i == 0 && s->buy_open) {
        m.look_x = m.look_y = 0.0f;
    }
    ev = cadv_player_tick(&a->body, &s->world, &m, P07_DT);
    if ((ev & CADV_PEV_STEP) && !cadv_input_down(in, CADV_ACT_RUN) && !a->body.crouching) {
        /* footsteps carry: walking (RUN held) or crouching is silent */
        int k;
        for (k = 0; k < P07_AGENTS; ++k) {
            if (s->a[k].alive && s->a[k].team != a->team && flat(s->a[k].body.pos, a->body.pos) < 16.0f) {
                s->a[k].heard = a->body.pos;
                s->a[k].heard_t = 1.5f;
            }
        }
        if (i != 0 && a->team != s->a[0].team) {
            sfx_at(s, CADV_SFX_STEP, 0.5f, a->body.pos);
        }
    }
    if (frozen || (i == 0 && s->buy_open)) {
        return;
    }
    /* the bomb: pick up, plant, defuse */
    if (side == P07_HUMBUG && s->bomb_dropped && flat(a->body.pos, s->bomb_pos) < 1.2f) {
        s->bomb_dropped = false;
        s->carrier = i;
        if (i == 0) {
            say(s, "YOU HAVE THE GLITTER BOMB");
        }
    }
    {
        int site;
        if (i == s->carrier && !s->planted && cadv_input_down(in, CADV_ACT_USE) && a->body.on_ground &&
            cadv_p07_in_site(s, a->body.pos, &site)) {
            a->plant_t += P07_DT;
            if (a->plant_t >= (float)P07_PLANT * P07_DT) {
                s->planted = true;
                s->bomb_site = site;
                s->bomb_pos = a->body.pos;
                s->bomb_timer = P07_BOMB;
                s->carrier = -1;
                a->plant_t = 0.0f;
                say(s, site == 0 ? "THE GLITTER BOMB IS PLANTED AT A" : "THE GLITTER BOMB IS PLANTED AT B");
                sfx_at(s, CADV_SFX_BELL, 1.0f, a->body.pos);
            }
        } else {
            a->plant_t = 0.0f;
        }
        if (side == P07_GUARD && s->planted && cadv_input_down(in, CADV_ACT_USE) && flat(a->body.pos, s->bomb_pos) < 1.8f) {
            a->defuse_t += P07_DT;
            if (a->defuse_t >= (float)(a->kit ? P07_DEFUSE_KIT : P07_DEFUSE) * P07_DT) {
                a->defuse_t = 0.0f;
                s->planted = false;
                end_round(s, P07_GUARD, CADV_WIN_DEFUSE);
                return;
            }
        } else {
            a->defuse_t = 0.0f;
        }
    }
    /* triggers */
    if (a->shot_cd > 0.0f || !(cadv_input_down(in, CADV_ACT_FIRE))) {
        return;
    }
    if (a->slot == P07_SLOT_KNIFE) {
        stab(s, i);
    } else if (a->slot == P07_SLOT_FLASH || a->slot == P07_SLOT_SMOKE) {
        if (cadv_input_pressed(in, CADV_ACT_FIRE)) {
            throw_nade(s, i, a->slot == P07_SLOT_SMOKE);
        }
    } else if (a->mag[a->slot == P07_SLOT_PRIMARY ? 2 : 1] > 0 && (k_weapons[w].automatic || cadv_input_pressed(in, CADV_ACT_FIRE))) {
        shoot(s, i);
    }
}

/* ---- bots ---------------------------------------------------------------------------------- */

static float reaction(const p07_state *s, int i)
{
    if (s->even) {
        return 0.34f; /* bot-vs-bot balance runs: everyone alike */
    }
    if (i == 0) {
        return 0.12f; /* the completion bot */
    }
    if (s->a[i].team == 0) {
        return 0.3f;
    }
    return s->cfg.difficulty == CADV_DIFF_STORY ? 0.6f : s->cfg.difficulty == CADV_DIFF_HARD ? 0.24f : 0.38f;
}

static float aim_skill(const p07_state *s, int i)
{
    if (s->even) {
        return 0.03f;
    }
    if (i == 0) {
        return 0.0f;
    }
    if (s->a[i].team == 0) {
        return 0.025f;
    }
    return s->cfg.difficulty == CADV_DIFF_STORY ? 0.06f : s->cfg.difficulty == CADV_DIFF_HARD ? 0.02f : 0.035f;
}

static cadv_nav_cell site_cell(int site, int k)
{
    cadv_nav_cell c;
    c.x = (int8_t)((k_sites[site][0] + k_sites[site][2]) / 2 + (k % 3) - 1);
    c.z = (int8_t)((k_sites[site][1] + k_sites[site][3]) / 2 + (k / 3) % 2);
    return c;
}

static cadv_nav_cell bot_goal(p07_state *s, int i)
{
    p07_agent *a = &s->a[i];
    int side = cadv_p07_side_of(s, a->team);
    cadv_nav_cell c = cadv_nav_cell_at(&s->nav, a->body.pos.x, a->body.pos.z);
    if (s->phase == P07_PH_CELEBRATE) {
        return c;
    }
    if (side == P07_HUMBUG) {
        if (s->planted) {
            /* guard the bomb from cover nearby */
            cadv_nav_cell b = cadv_nav_cell_at(&s->nav, s->bomb_pos.x, s->bomb_pos.z);
            cadv_nav_cell cov;
            b.x = (int8_t)(b.x + (i % 3) - 1);
            b.z = (int8_t)(b.z + (i % 2) * 2 - 1);
            if (cadv_nav_nearest_cover(&s->nav, b, 3, &cov)) {
                return cov;
            }
            return cadv_nav_cell_at(&s->nav, s->bomb_pos.x, s->bomb_pos.z);
        }
        if (s->bomb_dropped) {
            /* the closest attacker fetches the bomb */
            int k;
            int best = -1;
            for (k = 0; k < P07_AGENTS; ++k) {
                if (s->a[k].alive && s->a[k].team == a->team &&
                    (best < 0 || flat(s->a[k].body.pos, s->bomb_pos) < flat(s->a[best].body.pos, s->bomb_pos))) {
                    best = k;
                }
            }
            if (best == i) {
                return cadv_nav_cell_at(&s->nav, s->bomb_pos.x, s->bomb_pos.z);
            }
        }
        if (a->role == P07_ROLE_LURK && s->ticks % (60 * 60) < 30 * 60 && s->timer > 60 * 60) {
            /* the lurker waits at the far long, then joins */
            return site_cell(1 - s->site_choice, i);
        }
        return site_cell(s->site_choice, i);
    }
    if (s->planted) {
        return cadv_nav_cell_at(&s->nav, s->bomb_pos.x, s->bomb_pos.z);
    }
    if (s->alert_site >= 0 && (a->role == P07_ROLE_MID || (a->role == P07_ROLE_ANCHOR_A) != (s->alert_site == 0))) {
        /* rotate to the site under attack */
        return site_cell(s->alert_site, i);
    }
    {
        int h = a->role == P07_ROLE_ANCHOR_A ? (i % 2) : a->role == P07_ROLE_ANCHOR_B ? 2 + (i % 2) : 4;
        c.x = (int8_t)k_hold[h][0];
        c.z = (int8_t)k_hold[h][1];
        return c;
    }
}

static void callout(p07_state *s, int i, cadv_vec3 where)
{
    char buf[96];
    if (s->a[i].team != 0 || s->radio_timer > 60) {
        return;
    }
    snprintf(buf, sizeof buf, "%.12s: ENEMY SPOTTED, %s!", s->names[i], cadv_p07_area(s, where));
    radio(s, buf);
}

static void bot_scan(p07_state *s, int i)
{
    p07_agent *a = &s->a[i];
    int best = -1;
    float bd = 1e9f;
    int k;
    cadv_vec3 f = cadv_player_forward(&a->body);
    if (a->blind > 1.0f) {
        a->target = -1;
        return;
    }
    for (k = 0; k < P07_AGENTS; ++k) {
        p07_agent *t = &s->a[k];
        cadv_vec3 to;
        float d;
        if (!t->alive || t->team == a->team) {
            continue;
        }
        to = cadv_v3_sub(t->body.pos, a->body.pos);
        d = cadv_v3_len(to);
        if (d > 70.0f) {
            continue;
        }
        /* a 120 degree view, except for the one already engaged (the
         * completion bot checks its corners like a careful human) */
        if (k != a->target && d > 2.0f && !(i == 0 && !s->even) && (to.x * f.x + to.z * f.z) / (d * cadv_sqrtf(f.x * f.x + f.z * f.z) + 1e-5f) < 0.5f) {
            continue;
        }
        if (cadv_p07_can_see(s, i, k) && d < bd) {
            bd = d;
            best = k;
        }
    }
    if (best != a->target) {
        if (best >= 0 && a->target < 0) {
            int k2;
            a->react = reaction(s, i) * (0.8f + 0.4f * cadv_rng_float01(&s->rng));
            callout(s, i, s->a[best].body.pos);
            /* defenders call for help at the site the attackers come to */
            for (k2 = 0; k2 < 2 && cadv_p07_side_of(s, a->team) == P07_GUARD && s->alert_site < 0; ++k2) {
                cadv_vec3 c = cadv_nav_centre(&s->nav, site_cell(k2, 4));
                if (flat(s->a[best].body.pos, c) < 22.0f) {
                    s->alert_site = k2;
                }
            }
        }
        a->target = best;
    }
}

static void bot_brain(p07_state *s, int i, cadv_input_frame *out)
{
    p07_agent *a = &s->a[i];
    int side = cadv_p07_side_of(s, a->team);
    cadv_vec3 want_dir = cadv_v3(0, 0, 0);
    float want_yaw = a->look_yaw;
    float want_pitch = 0.0f;
    bool engage = false;
    memset(out, 0, sizeof *out);
    if (!a->alive || s->phase != P07_PH_LIVE || (s->bench && i != 0)) {
        return;
    }
    if ((s->ticks + i) % 5 == 0) {
        bot_scan(s, i);
    }
    a->react -= P07_DT;
    /* movement along the path to the goal */
    {
        cadv_nav_cell goal = bot_goal(s, i);
        cadv_nav_cell here = cadv_nav_cell_at(&s->nav, a->body.pos.x, a->body.pos.z);
        if (goal.x != a->goal.x || goal.z != a->goal.z || a->path_len == 0 || (s->ticks + i * 7) % 90 == 0) {
            int n = cadv_nav_path(&s->nav, here, goal, a->path, P07_PATH);
            a->goal = goal;
            a->path_len = n > 0 ? n : 0;
            a->path_pos = 0;
        }
        while (a->path_pos < a->path_len) {
            cadv_vec3 c = cadv_nav_centre(&s->nav, a->path[a->path_pos]);
            if (flat(c, a->body.pos) < 0.8f) {
                ++a->path_pos;
            } else {
                want_dir = cadv_v3_norm(cadv_v3(c.x - a->body.pos.x, 0, c.z - a->body.pos.z));
                want_yaw = yaw_to(a->body.pos, c);
                break;
            }
        }
        if (a->path_pos >= a->path_len) {
            /* arrived: watch the way enemies will come */
            cadv_vec3 g = cadv_nav_centre(&s->nav, goal);
            if (flat(g, a->body.pos) > 0.5f) {
                want_dir = cadv_v3_norm(cadv_v3(g.x - a->body.pos.x, 0, g.z - a->body.pos.z));
            }
            want_yaw = side == P07_GUARD ? CADV_PI * 0.0f + a->look_yaw : a->look_yaw;
            if (side == P07_GUARD) {
                want_yaw = yaw_to(a->body.pos, cadv_v3(cell_c(20), 0, cell_c(24))); /* towards the attackers' side */
            }
        }
    }
    /* someone heard but not seen: look there */
    if (a->target < 0 && a->heard_t > 0.0f) {
        want_yaw = yaw_to(a->body.pos, a->heard);
    }
    if (a->target >= 0 && s->a[a->target].alive) {
        p07_agent *t = &s->a[a->target];
        cadv_vec3 e = cadv_p07_eye(s, i);
        cadv_vec3 aim = t->body.pos;
        float d = flat(e, aim);
        float wob = aim_skill(s, i);
        aim.y += (i == 0 || (a->name % 3 == 0)) ? cadv_player_height(&t->body) - 0.15f : cadv_player_height(&t->body) * 0.7f;
        want_yaw = yaw_to(e, aim) + wob * cadv_sinf((float)s->ticks * 0.11f + (float)i);
        want_pitch = cadv_atan2f(aim.y - e.y, d) + wob * 0.6f * cadv_sinf((float)s->ticks * 0.07f + (float)i * 2.0f);
        engage = a->react <= 0.0f;
        /* the knife lover closes in */
        if (s->names[i][0] != '\0' && i != 0 && a->name == 4 && d < 7.0f) {
            out->pressed |= a->slot != P07_SLOT_KNIFE ? CADV_ACT_BIT(CADV_ACT_WEAPON_3) : 0u;
            want_dir = cadv_v3_norm(cadv_v3(aim.x - e.x, 0, aim.z - e.z));
        } else if (engage) {
            /* stop to shoot straight, with a little strafing; defenders on
             * their spot crouch behind it */
            a->strafe += P07_DT;
            want_dir = cadv_v3(0, 0, 0);
            if (side == P07_GUARD && a->path_pos >= a->path_len && !s->planted) {
                out->held |= CADV_ACT_BIT(CADV_ACT_CROUCH);
            }
            if (cadv_sinf(a->strafe * 2.0f + (float)i) > 0.6f && i != 0) {
                out->move_x = 0.5f;
            }
        }
    } else {
        a->target = -1;
        if (a->slot == P07_SLOT_KNIFE) {
            out->pressed |= CADV_ACT_BIT(a->primary >= 0 ? CADV_ACT_WEAPON_1 : CADV_ACT_WEAPON_2);
        }
    }
    /* turn and aim within a turn rate */
    {
        float dy = cadv_wrap_angle(want_yaw - a->body.yaw);
        float dp = want_pitch - a->body.pitch;
        float rate = (i == 0 ? 14.0f : 8.0f) * P07_DT;
        out->look_x = cadv_clampf(dy, -rate, rate);
        out->look_y = cadv_clampf(dp, -rate, rate);
        if (engage) {
            float err = cadv_absf(dy) + cadv_absf(dp);
            int w = a->slot == P07_SLOT_PRIMARY ? a->primary : a->pistol;
            float tol = w == P07_SNIPER ? 0.01f : 0.05f;
            if (w == P07_SNIPER && !a->scoped && a->slot == P07_SLOT_PRIMARY) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
            } else if (i == 0 && !s->even && a->slot != P07_SLOT_KNIFE) {
                /* the completion bot taps single, crouched, accurate first shots */
                out->held |= CADV_ACT_BIT(CADV_ACT_CROUCH);
                if (err < tol * 0.5f && a->since_shot > 0.42f) {
                    out->pressed |= CADV_ACT_BIT(CADV_ACT_FIRE);
                    out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
                }
            } else if (a->slot == P07_SLOT_KNIFE || err < tol) {
                a->burst += P07_DT;
                /* bursts of fire, released now and then to reset the spray */
                if (a->burst < 0.45f || a->slot != P07_SLOT_PRIMARY) {
                    out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
                    if (((s->ticks + i) % 9) == 0 || a->slot == P07_SLOT_KNIFE) {
                        out->pressed |= CADV_ACT_BIT(CADV_ACT_FIRE);
                    }
                } else if (a->burst > 0.7f) {
                    a->burst = 0.0f;
                }
            }
        }
    }
    /* grenades: smoke the site entrance, flash a heard enemy */
    if (!engage && a->smokes > 0 && side == P07_HUMBUG && a->role == P07_ROLE_ENTRY && !s->planted) {
        cadv_vec3 sc = cadv_nav_centre(&s->nav, site_cell(s->site_choice, 4));
        if (flat(a->body.pos, sc) < 18.0f && flat(a->body.pos, sc) > 9.0f) {
            if (a->slot != P07_SLOT_SMOKE) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_WEAPON_5);
            } else {
                want_yaw = yaw_to(a->body.pos, sc);
                out->look_x = cadv_clampf(cadv_wrap_angle(want_yaw - a->body.yaw), -0.2f, 0.2f);
                out->look_y = cadv_clampf(0.25f - a->body.pitch, -0.1f, 0.1f);
                if (cadv_absf(cadv_wrap_angle(want_yaw - a->body.yaw)) < 0.1f) {
                    out->pressed |= CADV_ACT_BIT(CADV_ACT_FIRE);
                    out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
                }
            }
        }
    } else if (!engage && a->flashes > 0 && a->heard_t > 1.0f && flat(a->body.pos, a->heard) < 15.0f && flat(a->body.pos, a->heard) > 6.0f) {
        if (a->slot != P07_SLOT_FLASH) {
            out->pressed |= CADV_ACT_BIT(CADV_ACT_WEAPON_4);
        } else if (cadv_absf(cadv_wrap_angle(yaw_to(a->body.pos, a->heard) - a->body.yaw)) < 0.15f) {
            out->pressed |= CADV_ACT_BIT(CADV_ACT_FIRE);
            out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
        }
    } else if ((a->slot == P07_SLOT_FLASH || a->slot == P07_SLOT_SMOKE) && !(a->slot == P07_SLOT_SMOKE && a->smokes > 0 && a->role == P07_ROLE_ENTRY)) {
        out->pressed |= CADV_ACT_BIT(a->primary >= 0 ? CADV_ACT_WEAPON_1 : CADV_ACT_WEAPON_2);
    }
    /* the bomb */
    {
        int site;
        if (i == s->carrier && cadv_p07_in_site(s, a->body.pos, &site) && site == s->site_choice && a->target < 0 &&
            flat(a->body.pos, cadv_nav_centre(&s->nav, a->goal)) < 2.5f) {
            out->held |= CADV_ACT_BIT(CADV_ACT_USE);
            want_dir = cadv_v3(0, 0, 0);
        }
        if (side == P07_GUARD && s->planted && flat(a->body.pos, s->bomb_pos) < 1.6f && (a->target < 0 || s->bomb_timer < 6 * 60)) {
            out->held |= CADV_ACT_BIT(CADV_ACT_USE);
            want_dir = cadv_v3(0, 0, 0);
        }
    }
    /* steering into the player's frame */
    if (cadv_v3_len(want_dir) > 0.1f) {
        float y = a->body.yaw;
        cadv_vec3 f = cadv_v3(-cadv_sinf(y), 0, -cadv_cosf(y));
        cadv_vec3 r = cadv_v3(cadv_cosf(y), 0, -cadv_sinf(y));
        out->move_y = cadv_v3_dot(want_dir, f);
        out->move_x = cadv_v3_dot(want_dir, r);
        /* stuck on a corner: hop */
        if (flat(a->body.pos, a->last_pos) < 0.02f) {
            if (++a->stuck > 30) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_JUMP);
                a->path_len = 0;
                a->stuck = 0;
            }
        } else {
            a->stuck = 0;
        }
    }
    /* lurkers and anchors walk quietly once near */
    if (a->target < 0 && (a->role == P07_ROLE_LURK || (side == P07_GUARD && a->path_len - a->path_pos < 4))) {
        out->held |= CADV_ACT_BIT(CADV_ACT_RUN);
    }
    a->last_pos = a->body.pos;
}

/* ---- tick ---------------------------------------------------------------------------------- */

static void check_marks(p07_state *s)
{
    int i;
    if (!s->a[0].alive) {
        return;
    }
    for (i = 1; i < P07_AGENTS; ++i) {
        p07_agent *a = &s->a[i];
        int f = a->name;
        if (f >= 1 && f <= 5 && !(s->marks_found & (1u << f)) && a->alive &&
            (a->team == 0 ? flat(a->body.pos, s->a[0].body.pos) < 12.0f : (flat(a->body.pos, s->a[0].body.pos) < 45.0f && cadv_p07_can_see(s, 0, i)))) {
            char buf[96];
            s->marks_found |= 1u << f;
            snprintf(buf, sizeof buf, "%s!", s->mark_text[f]);
            feed(s, buf);
        }
    }
    /* the spray tag on the wall of the square */
    if (!(s->marks_found & 1u) && flat(s->a[0].body.pos, cadv_v3(cell_c(16), 0, 20.0f)) < 8.0f) {
        char buf[96];
        s->marks_found |= 1u;
        snprintf(buf, sizeof buf, "A SPRAY TAG: \"%s\"", s->mark_text[0]);
        say(s, buf);
    }
}

void cadv_p07_tick(void *state, const cadv_input_frame *in)
{
    p07_state *s = (p07_state *)state;
    int i;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
    s->map_held = cadv_input_down(in, CADV_ACT_MAP);
    if (s->message_timer > 0) {
        --s->message_timer;
    }
    if (s->radio_timer > 0) {
        --s->radio_timer;
    }
    if (s->hurt_flash > 0) {
        --s->hurt_flash;
    }
    if (s->confetti_t > 0) {
        --s->confetti_t;
    }
    for (i = 0; i < P07_FEED; ++i) {
        if (s->feed_t[i] > 0) {
            --s->feed_t[i];
        }
    }
    for (i = 0; i < 16; ++i) {
        if (s->tracers[i].ttl > 0) {
            --s->tracers[i].ttl;
        }
    }
    if (s->end_timer > 0) {
        if (--s->end_timer == 0) {
            s->outcome = s->phase == P07_PH_OVER ? CADV_OUTCOME_LOST : CADV_OUTCOME_WON;
        }
        if (s->phase == P07_PH_CELEBRATE) {
            agent_step(s, 0, in);
        }
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_PAUSE)) {
        if (s->buy_open) {
            s->buy_open = false;
        } else {
            s->outcome = CADV_OUTCOME_QUIT;
            return;
        }
    }
    if (s->phase == P07_PH_CELEBRATE) {
        agent_step(s, 0, in);
        if (!s->star_taken && cadv_v3_dist(cadv_v3_add(s->a[0].body.pos, cadv_v3(0, 0.9f, 0)), s->star_pos) < 1.6f) {
            s->star_taken = true;
            s->end_timer = 180;
            say(s, "THE SHINING STAR OF GLUHSTADT!");
            sfx_at(s, CADV_SFX_STAR, 1.0f, s->a[0].body.pos);
        }
        if (--s->timer <= 0) {
            s->end_timer = 1;
        }
        return;
    }
    /* buy menu: in the spawn during the freeze */
    if (s->phase == P07_PH_FREEZE && s->a[0].alive) {
        if (cadv_input_pressed(in, CADV_ACT_WEAPON_6)) {
            s->buy_open = !s->buy_open;
        }
        if (s->buy_open) {
            if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_RADIAL)) {
                s->buy_cursor = (s->buy_cursor + 1) % P07_BUY_ITEMS;
            }
            if (cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
                s->buy_cursor = (s->buy_cursor + P07_BUY_ITEMS - 1) % P07_BUY_ITEMS;
            }
            if (cadv_input_pressed(in, CADV_ACT_FIRE) || cadv_input_pressed(in, CADV_ACT_USE)) {
                char buf[72];
                bool ok = buy(s, 0, s->buy_cursor);
                snprintf(buf, sizeof buf, "%s %s", ok ? "BOUGHT" : "CANNOT BUY", k_buy_names[s->buy_cursor]);
                say(s, buf);
            }
        }
    } else {
        s->buy_open = false;
    }
    /* the player, or the camera when tagged */
    if (s->a[0].alive) {
        agent_step(s, 0, in);
    } else if (cadv_input_pressed(in, CADV_ACT_USE)) {
        int k;
        for (k = 1; k < P07_AGENTS; ++k) {
            int c = (s->spectate + k) % P07_AGENTS;
            if (c != 0 && s->a[c].alive && s->a[c].team == 0) {
                s->spectate = c;
                break;
            }
        }
    }
    if (s->phase == P07_PH_AFTER) {
        if (--s->timer <= 0) {
            next_round(s);
        }
        return;
    }
    for (i = 1; i < P07_AGENTS && s->phase != P07_PH_AFTER; ++i) {
        cadv_input_frame b;
        bot_brain(s, i, &b);
        agent_step(s, i, &b);
    }
    if (s->spectate != 0 && !s->a[s->spectate].alive) {
        int k;
        for (k = 1; k < P07_AGENTS; ++k) {
            if (s->a[k].alive && s->a[k].team == 0) {
                s->spectate = k;
                break;
            }
        }
    }
    if (s->phase == P07_PH_FREEZE) {
        if (--s->timer <= 0) {
            s->phase = P07_PH_LIVE;
            s->timer = P07_ROUND;
            s->buy_open = false;
            radio(s, "GO GO GO!");
        }
        return;
    }
    if (s->phase != P07_PH_LIVE) {
        return;
    }
    tick_nades(s);
    check_marks(s);
    --s->timer;
    if (s->planted) {
        if (--s->bomb_timer <= 0) {
            /* glitter everywhere: everyone close pops */
            for (i = 0; i < P07_AGENTS; ++i) {
                if (s->a[i].alive && flat(s->a[i].body.pos, s->bomb_pos) < 12.0f) {
                    tag(s, i, -1, P07_KNIFE);
                }
            }
            s->confetti_pos = s->bomb_pos;
            s->confetti_t = 120;
            sfx_at(s, CADV_SFX_EXPLOSION, 1.0f, s->bomb_pos);
            s->planted = false;
            end_round(s, P07_HUMBUG, CADV_WIN_BOMB);
            return;
        }
    }
    {
        int t_team = team_of_side(s, P07_HUMBUG);
        if (cadv_p07_alive(s, 1 - t_team) == 0) {
            end_round(s, P07_HUMBUG, CADV_WIN_ELIMINATION);
        } else if (cadv_p07_alive(s, t_team) == 0 && !s->planted) {
            end_round(s, P07_GUARD, CADV_WIN_ELIMINATION);
        } else if (s->timer <= 0 && !s->planted) {
            end_round(s, P07_GUARD, CADV_WIN_TIME);
        }
    }
}

/* ---- init ------------------------------------------------------------------------------------ */

bool cadv_p07_init(void *state, const cadv_portal_cfg *cfg)
{
    p07_state *s = (p07_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    cadv_rng_seed(&s->rng, cfg->seed, 7u);
    cadv_friends_default(&fr);
    if (cfg->friends) {
        for (i = 0; i < cfg->friend_count && i < CADV_MAX_FRIENDS; ++i) {
            snprintf(fr.name[i], CADV_FRIEND_NAME, "%s", cfg->friends[i]);
        }
        fr.count = cfg->friend_count < CADV_MAX_FRIENDS ? cfg->friend_count : CADV_MAX_FRIENDS;
    }
    for (i = 0; d && i < d->mark_count && i < 6; ++i) {
        cadv_friends_mark_text(&fr, &d->marks[i], s->mark_text[i], sizeof s->mark_text[i]);
    }
    build_market(s);
    s->econ = cadv_econ_default();
    /* the rosters: friends split across both teams, the rest are townsfolk */
    {
        static const int roster[P07_AGENTS] = {0, 1, 2, -1, -2, 3, 4, 5, -3, -4};
        for (i = 0; i < P07_AGENTS; ++i) {
            p07_agent *a = &s->a[i];
            int r = roster[i];
            a->team = i < P07_TEAM ? 0 : 1;
            a->name = r > 0 ? r : 0;
            a->money = s->econ.start;
            a->alive = true;
            strip(a);
            if (i == 0) {
                copy_text(s->names[i], sizeof s->names[i], "YOU");
            } else if (r > 0) {
                copy_text(s->names[i], sizeof s->names[i], fr.name[r < fr.count ? r : 0]);
            } else {
                copy_text(s->names[i], sizeof s->names[i], k_filler[-r - 1]);
                a->name = 5 + (-r); /* townsfolk ids beyond the friends */
            }
        }
    }
    s->player_side = P07_HUMBUG;
    s->round = 1;
    s->checkpoint = cfg->checkpoint;
    if (cfg->checkpoint > 0) {
        /* resume at half time with the score as it stood */
        int c = cfg->checkpoint - 1;
        s->score[0] = cadv_mini(c / 16, P07_HALF);
        s->score[1] = cadv_mini(c % 16, P07_HALF);
        s->round = P07_HALF + 1;
        s->player_side = P07_GUARD;
    }
    start_round(s);
    return true;
}

/* ---- results & scripting --------------------------------------------------------------------- */

cadv_portal_result cadv_p07_result(const void *state)
{
    const p07_state *s = (const p07_state *)state;
    cadv_portal_result res;
    memset(&res, 0, sizeof res);
    res.outcome = s->outcome;
    res.star_collected = s->star_taken;
    res.checkpoint = s->checkpoint;
    res.marks_found = s->marks_found;
    res.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        bool halves = s->half_won[0][0] > s->half_won[0][1] && s->half_won[1][0] > s->half_won[1][1];
        res.snowflakes = 1 + (halves ? 1 : 0) + (s->clutch || s->ace ? 1 : 0);
    }
    return res;
}

bool cadv_p07_query(const void *state, const char *key, double *out)
{
    const p07_state *s = (const p07_state *)state;
    cadv_vec3 e = cadv_p07_eye(s, 0);
    const p07_agent *p = &s->a[0];
    struct kv { const char *k; double v; };
    const struct kv table[] = {
        {"player.x", e.x},
        {"player.y", e.y},
        {"player.z", e.z},
        {"player.yaw", p->body.yaw},
        {"player.health", p->alive ? p->body.health : 0},
        {"armour", p->body.armor},
        {"money", p->money},
        {"round", s->round},
        {"phase", s->phase},
        {"score.us", s->score[0]},
        {"score.them", s->score[1]},
        {"side", s->player_side},
        {"alive.us", cadv_p07_alive(s, 0)},
        {"alive.them", cadv_p07_alive(s, 1)},
        {"planted", s->planted},
        {"carrier", s->carrier},
        {"kills", p->kills},
        {"primary", p->primary},
        {"slot", p->slot},
        {"buy.open", s->buy_open},
        {"smokes", s->smokes[0].ticks > 0},
        {"blind", p->blind},
        {"clutch", s->clutch},
        {"star.taken", s->star_taken},
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

void cadv_p07_teleport(void *state, float x, float y, float z, float yaw)
{
    p07_state *s = (p07_state *)state;
    s->a[0].body.pos = cadv_v3(x, y, z);
    s->a[0].body.vel = cadv_v3(0, 0, 0);
    s->a[0].body.yaw = yaw;
    s->a[0].body.pitch = 0.0f;
}

/* ---- completion bot ----------------------------------------------------------------------------- */

void cadv_p07_bot(const void *state, cadv_input_frame *out)
{
    p07_state *s = (p07_state *)(uintptr_t)state; /* the brain keeps its notes in the agent */
    p07_agent *a = &s->a[0];
    memset(out, 0, sizeof *out);
    if (s->end_timer > 0) {
        return;
    }
    if (s->phase == P07_PH_FREEZE && a->alive) {
        /* shop through the menu: a rifle and a vest when affordable */
        int want = -1;
        if (a->primary < 0 && a->money >= 2700 + (a->body.armor < 100 ? 650 : 0)) {
            want = P07_BUY_RIFLE;
        } else if (a->body.armor < 100 && a->money >= 650) {
            want = P07_BUY_ARMOUR;
        } else if (cadv_p07_side_of(s, 0) == P07_GUARD && !a->kit && a->money >= 400) {
            want = P07_BUY_KIT;
        }
        if (want < 0) {
            if (s->buy_open && s->ticks % 2 == 0) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_WEAPON_6);
            }
            return;
        }
        if (!s->buy_open) {
            if (s->ticks % 2 == 0) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_WEAPON_6);
            }
        } else if (s->buy_cursor != want) {
            out->pressed |= CADV_ACT_BIT(CADV_ACT_NEXT_WEAPON);
        } else if (s->ticks % 2 == 0) {
            out->pressed |= CADV_ACT_BIT(CADV_ACT_FIRE);
        }
        return;
    }
    if (s->phase == P07_PH_CELEBRATE) {
        /* up the tree, step by step */
        int k;
        cadv_vec3 to;
        if (a->body.on_ground) {
            /* standing: the next step is the one above the one underfoot */
            s->climb = 0;
            for (k = 0; k < P07_STEPS; ++k) {
                if (cadv_absf(a->body.pos.y - s->step_pos[k].y) < 0.2f && flat(a->body.pos, s->step_pos[k]) < 1.5f) {
                    s->climb = k + 1;
                }
            }
            if (a->body.pos.y > s->step_pos[P07_STEPS - 1].y + 0.3f) {
                s->climb = P07_STEPS;
            }
        }
        to = s->climb < P07_STEPS ? s->step_pos[s->climb] : s->star_pos;
        {
            float d = flat(a->body.pos, to);
            float y = a->body.yaw;
            cadv_vec3 dir = d > 0.05f ? cadv_v3((to.x - a->body.pos.x) / d, 0, (to.z - a->body.pos.z) / d) : cadv_v3(0, 0, 0);
            float sp = cadv_minf(1.0f, d * 0.7f + 0.15f);
            /* look up at the star, walk towards the next step in any direction */
            out->look_x = cadv_clampf(cadv_wrap_angle(yaw_to(a->body.pos, s->star_pos) - y), -0.1f, 0.1f);
            out->move_y = (dir.x * -cadv_sinf(y) + dir.z * -cadv_cosf(y)) * sp;
            out->move_x = (dir.x * cadv_cosf(y) + dir.z * -cadv_sinf(y)) * sp;
            if (d < 2.2f && a->body.on_ground && to.y > a->body.pos.y + 0.2f) {
                out->pressed |= CADV_ACT_BIT(CADV_ACT_JUMP);
            }
        }
        return;
    }
    if (!a->alive) {
        return;
    }
    bot_brain(s, 0, out);
}

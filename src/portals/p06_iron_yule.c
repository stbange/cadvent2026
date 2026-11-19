/* SPDX-License-Identifier: MIT */
/* Day 6 - Iron Yule: a walker cockpit with torso twist, heat, weapon groups
 * and component damage; a three-mission campaign with Santa Claude. */
#include "portals/p06_iron_yule.h"

#include <stdio.h>
#include <string.h>

#include "audio/audio.h"
#include "hub/friends.h"
#include "portals/p06_internal.h"

/* ---- tables (original machines and weapons) ------------------------------------------ */

static const p06_weapon_def k_weapons[P06_WEAPONS] = {
    /* name          group               heat  dmg n  cool  range  speed  ammo tons */
    {"LASER", P06_G_ENERGY, 6.0f, 7, 1, 0.75f, 300.0f, 0.0f, 0, 1},
    {"PPC", P06_G_ENERGY, 14.0f, 18, 1, 2.0f, 380.0f, 0.0f, 0, 3},
    {"AUTOCANNON", P06_G_BALLISTIC, 1.5f, 8, 1, 0.45f, 340.0f, 280.0f, 90, 3},
    {"MISSILES", P06_G_MISSILE, 4.0f, 3, 5, 2.4f, 420.0f, 110.0f, 24, 2},
};

static const cadv_walker_def k_player_def = {
    16.0f, 0.5f, 6.0f, 0.8f, 0.9f, 1.6f, 2.2f, 9.0f, 0.9f, 18.0f, 2.5f, 18.0f, 9.8f, 100.0f, 9.0f, 45.0f,
    {30, 84, 58, 58, 44, 44, 66, 66},
};
static const cadv_walker_def k_jackal_def = {
    18.0f, 0.5f, 8.0f, 1.5f, 1.4f, 1.6f, 3.0f, 7.0f, 0.9f, 16.0f, 1.5f, 10.0f, 9.8f, 100.0f, 12.0f, 40.0f,
    {10, 30, 20, 20, 14, 14, 22, 22},
};
static const cadv_walker_def k_krampus_def = {
    13.0f, 0.5f, 6.0f, 1.2f, 1.0f, 1.6f, 2.5f, 10.0f, 0.9f, 20.0f, 3.0f, 14.0f, 9.8f, 100.0f, 8.0f, 45.0f,
    {60, 240, 160, 160, 110, 110, 170, 170},
};

/* the convoy route through the ambush canyon, south to north */
static const float k_path[P06_PATH_NODES][2] = {{256, 488}, {252, 440}, {285, 392}, {290, 345}, {262, 300},
                                                {225, 262}, {232, 218}, {250, 185}, {256, 165}};
static const int k_ambush_node[3] = {2, 4, 6};
static const float k_pumps[4][2] = {{165, 65}, {188, 70}, {168, 96}, {192, 93}};
static const float k_turrets[3][2] = {{150, 82}, {206, 82}, {180, 114}};
static const float k_vents[P06_VENTS][2] = {{390, 80}, {368, 60}, {410, 102}, {366, 102}, {414, 58}};

const p06_weapon_def *cadv_p06_weapons(void) { return k_weapons; }

const cadv_walker_def *cadv_p06_walker_def(int kind)
{
    return kind == P06_K_JACKAL ? &k_jackal_def : kind == P06_K_KRAMPUS ? &k_krampus_def : &k_player_def;
}

/* ---- small helpers ----------------------------------------------------------------- */

static void copy_text(char *out, size_t cap, const char *text)
{
    size_t i;
    for (i = 0; text[i] && i + 1 < cap; ++i) {
        out[i] = text[i];
    }
    out[i] = '\0';
}

static void say(p06_state *s, const char *text)
{
    copy_text(s->message, sizeof s->message, text);
    s->message_timer = 180;
}

/* Santa Claude (and the wingmen) on the radio, subtitled. */
static void radio(p06_state *s, const char *text)
{
    copy_text(s->radio, sizeof s->radio, text);
    s->radio_timer = 300;
}

static void sfx(p06_state *s, cadv_sfx id, float vol)
{
    if (s->cfg.audio) {
        cadv_audio_sfx(s->cfg.audio, id, vol, 0.0f);
    }
}

static float dmg_scale(const p06_state *s)
{
    return s->cfg.difficulty == CADV_DIFF_STORY ? 0.6f : s->cfg.difficulty == CADV_DIFF_HARD ? 1.2f : 1.0f;
}

static float flat_dist(cadv_vec3 a, cadv_vec3 b) { return cadv_sqrtf((a.x - b.x) * (a.x - b.x) + (a.z - b.z) * (a.z - b.z)); }

static float yaw_to(cadv_vec3 from, cadv_vec3 to) { return cadv_atan2f(-(to.x - from.x), -(to.z - from.z)); }

static bool is_walker(int kind) { return kind == P06_K_JACKAL || kind == P06_K_KRAMPUS || kind == P06_K_WINGMAN; }

cadv_vec3 cadv_p06_unit_pos(const p06_state *s, int i)
{
    return i == P06_PLAYER ? s->mech.pos : (is_walker(s->u[i].kind) ? s->u[i].w.pos : s->u[i].pos);
}

static float unit_radius(int kind)
{
    switch (kind) {
    case P06_K_PUMP:
        return 5.0f;
    case P06_K_CRAWLER:
        return 6.0f;
    case P06_K_KRAMPUS:
        return 3.8f;
    case P06_K_JACKAL:
    case P06_K_HOVER:
        return 2.8f;
    default:
        return 3.2f;
    }
}

static float unit_mid(int kind)
{
    switch (kind) {
    case P06_K_PUMP:
        return 6.0f;
    case P06_K_CRAWLER:
    case P06_K_HOVER:
        return 2.0f;
    case P06_K_TURRET:
        return 2.5f;
    case P06_K_JACKAL:
        return 4.0f;
    default:
        return 5.5f;
    }
}

static cadv_vec3 unit_centre(const p06_state *s, int i)
{
    cadv_vec3 p = cadv_p06_unit_pos(s, i);
    p.y += i == P06_PLAYER ? 5.0f : unit_mid(s->u[i].kind);
    return p;
}

static bool targetable(const p06_state *s, int i)
{
    const p06_unit *u = &s->u[i];
    return u->active && !u->dead && u->kind != P06_K_WRECK;
}

int cadv_p06_enemies_alive(const p06_state *s)
{
    int i;
    int n = 0;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        n += targetable(s, i) && s->u[i].team == P06_TEAM_GRAIL;
    }
    return n;
}

int cadv_p06_tonnage(const p06_state *s)
{
    int i;
    int t = 0;
    for (i = 0; i < P06_SLOTS; ++i) {
        t += k_weapons[s->loadout[i]].tons;
    }
    return t;
}

const char *cadv_p06_paint_name(const p06_state *s, int paint)
{
    return paint == 0 ? "HOLLOWFROST WHITE" : paint == 1 ? "SLEIGHBELL RED" : s->mark_text[0];
}

cadv_vec3 cadv_p06_eye(const p06_state *s) { return cadv_walker_eye(&s->mech, &k_player_def); }

/* ---- the Winterhold valley ----------------------------------------------------------- */

static float hash2(int x, int z, uint32_t seed)
{
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)z * 668265263u + seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xffffu) / 65535.0f;
}

static float vnoise(float x, float z, uint32_t seed)
{
    int ix = x >= 0.0f ? (int)x : (int)x - 1;
    int iz = z >= 0.0f ? (int)z : (int)z - 1;
    float fx = x - (float)ix;
    float fz = z - (float)iz;
    float a = hash2(ix, iz, seed);
    float b = hash2(ix + 1, iz, seed);
    float c = hash2(ix, iz + 1, seed);
    float d = hash2(ix + 1, iz + 1, seed);
    fx = fx * fx * (3.0f - 2.0f * fx);
    fz = fz * fz * (3.0f - 2.0f * fz);
    return (a + (b - a) * fx) + ((c + (d - c) * fx) - (a + (b - a) * fx)) * fz;
}

static float smooth01(float e0, float e1, float x)
{
    float t = cadv_clampf((x - e0) / (e1 - e0), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

static float path_dist(float x, float z)
{
    float best = 1e9f;
    int i;
    for (i = -1; i + 1 < P06_PATH_NODES; ++i) {
        /* node -1 extends the canyon to the southern edge */
        float ax = i < 0 ? 256.0f : k_path[i][0];
        float az = i < 0 ? 520.0f : k_path[i][1];
        float bx = k_path[i + 1][0];
        float bz = k_path[i + 1][1];
        float vx = bx - ax;
        float vz = bz - az;
        float t = cadv_clampf(((x - ax) * vx + (z - az) * vz) / (vx * vx + vz * vz), 0.0f, 1.0f);
        float dx = x - (ax + vx * t);
        float dz = z - (az + vz * t);
        best = cadv_minf(best, cadv_sqrtf(dx * dx + dz * dz));
    }
    return best;
}

static float valley_height(float x, float z)
{
    float base = 36.0f + 8.0f * vnoise(x / 90.0f, z / 90.0f, 1u) + 3.0f * vnoise(x / 30.0f, z / 30.0f, 2u);
    float basin = smooth01(212.0f, 186.0f, z) * smooth01(30.0f, 62.0f, x) * smooth01(482.0f, 452.0f, x) * smooth01(6.0f, 30.0f, z);
    float floor_b = 9.0f + 1.5f * vnoise(x / 40.0f, z / 40.0f, 3u);
    float h = base + (floor_b - base) * basin;
    float wc = 1.0f - smooth01(18.0f, 42.0f, path_dist(x, z));
    float floor_c = 8.0f + 0.8f * vnoise(x / 25.0f, z / 25.0f, 4u);
    float r;
    h = h + (floor_c - h) * wc;
    /* the Winterhold reservoir behind the ice dam */
    r = cadv_sqrtf((x - 110.0f) * (x - 110.0f) + (z - 70.0f) * (z - 70.0f));
    h = h + (0.0f - h) * (1.0f - smooth01(32.0f, 54.0f, r));
    /* the frozen crater: a steep rim with a gap in the west */
    r = cadv_sqrtf((x - 390.0f) * (x - 390.0f) + (z - 80.0f) * (z - 80.0f));
    {
        float ang = cadv_atan2f(z - 80.0f, x - 390.0f);
        float gap = smooth01(0.25f, 0.45f, cadv_absf(cadv_wrap_angle(ang - CADV_PI)));
        float inside = 1.0f - smooth01(55.0f, 64.0f, r);
        float bowl = 7.0f + 6.0f * (r / 62.0f) * (r / 62.0f);
        h = h + (bowl - h) * inside;
        float k = cadv_maxf(0.0f, 1.0f - ((r - 63.0f) / 10.0f) * ((r - 63.0f) / 10.0f));
        h += 16.0f * gap * k * k;
    }
    return h;
}

static void build_valley(p06_state *s)
{
    cadv_terrain *t = &s->terrain;
    int x;
    int z;
    cadv_terrain_init(t, P06_N, P06_CELL, -2.0f, 70.0f, P06_SEA);
    for (z = 0; z < P06_N; ++z) {
        for (x = 0; x < P06_N; ++x) {
            t->h[z * P06_N + x] = cadv_clampf(valley_height((float)x * P06_CELL, (float)z * P06_CELL), -2.0f, 70.0f);
        }
    }
    cadv_terrain_mark_all_dirty(t);
    for (x = 0; x < P06_PATH_NODES; ++x) {
        s->path[x] = cadv_v3(k_path[x][0], 0, k_path[x][1]);
        s->path[x].y = cadv_terrain_height(t, s->path[x].x, s->path[x].z);
    }
    for (x = 0; x < P06_VENTS; ++x) {
        s->vents[x] = cadv_v3(k_vents[x][0], 0, k_vents[x][1]);
        s->vents[x].y = cadv_terrain_height(t, s->vents[x].x, s->vents[x].z);
    }
    s->pump_station = cadv_v3(178, 0, 82);
    s->crater = cadv_v3(390, 0, 80);
    s->depot = s->path[P06_PATH_NODES - 1];
    s->wreck_pos = cadv_v3(300, cadv_terrain_height(t, 300, 352), 352);
}

/* ---- units ------------------------------------------------------------------------- */

static float ground_at(const p06_state *s, float x, float z) { return cadv_terrain_height(&s->terrain, x, z); }

static int spawn(p06_state *s, int kind, int team, float x, float z, float yaw)
{
    int i;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        p06_unit *u = &s->u[i];
        if (!u->active) {
            memset(u, 0, sizeof *u);
            u->active = true;
            u->kind = kind;
            u->team = team;
            u->yaw = yaw;
            u->target = -1;
            u->strafe = (i % 2) ? 1.0f : -1.0f;
            u->pos = cadv_v3(x, ground_at(s, x, z), z);
            if (is_walker(kind)) {
                cadv_walker_spawn(&u->w, cadv_p06_walker_def(kind), u->pos, yaw);
                if (kind == P06_K_KRAMPUS) {
                    int k;
                    float m = s->cfg.difficulty == CADV_DIFF_STORY ? 0.7f : s->cfg.difficulty == CADV_DIFF_HARD ? 1.1f : 1.0f;
                    for (k = 0; k < CADV_WZ_COUNT; ++k) {
                        u->w.hp[k] = (int)((float)u->w.hp[k] * m);
                    }
                }
            }
            switch (kind) {
            case P06_K_HOVER:
                u->hp = u->hp_max = 70;
                break;
            case P06_K_TURRET:
                u->hp = u->hp_max = 140;
                break;
            case P06_K_PUMP:
                u->hp = u->hp_max = 220;
                break;
            case P06_K_WINGMAN:
                u->hp = u->hp_max = 400;
                break;
            case P06_K_CRAWLER:
                u->hp = u->hp_max = s->cfg.difficulty == CADV_DIFF_STORY ? 1000 : s->cfg.difficulty == CADV_DIFF_HARD ? 550 : 700;
                break;
            default:
                u->hp = u->hp_max = 1;
                break;
            }
            return i;
        }
    }
    return -1; /* GCOVR_EXCL_LINE: the missions never fill the table */
}

static void clear_units(p06_state *s, bool keep_us)
{
    int i;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (!keep_us || s->u[i].team != P06_TEAM_US) {
            s->u[i].active = false;
        }
    }
    memset(s->proj, 0, sizeof s->proj);
    memset(s->beam, 0, sizeof s->beam);
    s->lock = -1;
    s->lock_t = 0.0f;
}

static void repair_player(p06_state *s, float x, float z, float yaw)
{
    int keep = s->mech.shutdowns;
    int i;
    cadv_walker_spawn(&s->mech, &k_player_def, cadv_v3(x, ground_at(s, x, z), z), yaw);
    s->mech.shutdowns = keep;
    for (i = 0; i < P06_SLOTS; ++i) {
        s->ammo[i] = k_weapons[s->loadout[i]].ammo;
        s->cd[i] = 0.0f;
    }
    s->coolant_left = 1;
    s->coolant_timer = 0;
    s->coolant_down = false;
    s->cool_boost = 0;
}

static void spawn_wingmen(p06_state *s)
{
    cadv_vec3 p = s->mech.pos;
    float ty = s->mech.legs_yaw;
    float rx = cadv_cosf(ty);
    float rz = -cadv_sinf(ty);
    s->wingmen[0] = spawn(s, P06_K_WINGMAN, P06_TEAM_US, p.x - rx * 18.0f, p.z - rz * 18.0f, ty);
    s->wingmen[1] = spawn(s, P06_K_WINGMAN, P06_TEAM_US, p.x + rx * 18.0f, p.z + rz * 18.0f, ty);
}

static void setup_m1(p06_state *s, int from_node)
{
    cadv_vec3 c = s->path[from_node];
    float yaw = yaw_to(s->path[from_node], s->path[from_node + 1]);
    clear_units(s, false);
    s->mission = P06_M1;
    s->crawler = spawn(s, P06_K_CRAWLER, P06_TEAM_US, c.x, c.z, yaw);
    s->path_next = from_node + 1;
    s->crawler_cp = from_node;
    repair_player(s, c.x + cadv_sinf(yaw) * 30.0f, c.z + cadv_cosf(yaw) * 30.0f, yaw);
    spawn_wingmen(s);
    spawn(s, P06_K_WRECK, P06_TEAM_GRAIL, s->wreck_pos.x, s->wreck_pos.z, 0.7f);
}

static void setup_m2(p06_state *s)
{
    int i;
    clear_units(s, false);
    s->crawler = -1;
    s->krampus = -1;
    s->mission = P06_M2;
    s->intro = 240;
    repair_player(s, 240, 160, yaw_to(cadv_v3(240, 0, 160), s->pump_station));
    spawn_wingmen(s);
    s->pumps_left = 4;
    s->reinforced = false;
    for (i = 0; i < 4; ++i) {
        spawn(s, P06_K_PUMP, P06_TEAM_GRAIL, k_pumps[i][0], k_pumps[i][1], 0.0f);
    }
    for (i = 0; i < 3; ++i) {
        spawn(s, P06_K_TURRET, P06_TEAM_GRAIL, k_turrets[i][0], k_turrets[i][1], 0.0f);
    }
    for (i = 0; i < (s->cfg.difficulty == CADV_DIFF_STORY ? 2 : 3); ++i) {
        spawn(s, P06_K_JACKAL, P06_TEAM_GRAIL, 150.0f + 22.0f * (float)i, 125.0f, 3.0f);
    }
    radio(s, "SANTA CLAUDE: The pumps are draining the reservoir. I'm relaying their positions to your radar.");
}

static void setup_m3(p06_state *s)
{
    clear_units(s, false);
    s->mission = P06_M3;
    s->intro = 240;
    repair_player(s, 300, 80, -0.5f * CADV_PI);
    s->crawler = spawn(s, P06_K_CRAWLER, P06_TEAM_US, 318, 40, 0.0f);
    s->krampus = spawn(s, P06_K_KRAMPUS, P06_TEAM_GRAIL, 425, 80, 0.5f * CADV_PI);
    s->lost_at_m3 = s->mech.lost;
    s->m3_component_lost = false;
    radio(s, "SANTA CLAUDE: Krampus waits in the crater. The steam vents cool you - and I've coolant if you ask nicely.");
}

/* ---- init ---------------------------------------------------------------------------- */

bool cadv_p06_init(void *state, const cadv_portal_cfg *cfg)
{
    p06_state *s = (p06_state *)state;
    const cadv_portal_desc *d = cadv_portal_get(cfg->day);
    cadv_friends fr;
    int i;
    memset(s, 0, sizeof *s);
    s->cfg = *cfg;
    s->checkpoint = cfg->checkpoint;
    cadv_rng_seed(&s->rng, cfg->seed, 6u);
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
    build_valley(s);
    s->loadout[0] = P06_W_LASER;
    s->loadout[1] = P06_W_AC;
    s->loadout[2] = P06_W_MISSILE;
    s->loadout[3] = P06_W_LASER;
    s->lock = -1;
    s->krampus = -1;
    s->crawler = -1;
    s->wingmen[0] = s->wingmen[1] = -1;
    if (s->checkpoint >= 2) {
        setup_m3(s);
    } else if (s->checkpoint == 1) {
        setup_m2(s);
    } else {
        /* the mech lab first, the convoy waits at the canyon mouth */
        setup_m1(s, 0);
        s->mission = P06_LAB;
        say(s, "MECH LAB: 1-4 HARDPOINT, Q SWAPS, R-MOUSE PAINT, E LAUNCH");
    }
    return true;
}

/* ---- damage --------------------------------------------------------------------------- */

static void explode_fx(p06_state *s, cadv_vec3 p, float vol)
{
    float d = flat_dist(p, s->mech.pos);
    sfx(s, CADV_SFX_EXPLOSION, vol * cadv_clampf(1.0f - d / 400.0f, 0.1f, 1.0f));
    if (d < 60.0f) {
        s->shake = 12;
    }
}

static void unit_destroyed(p06_state *s, int i)
{
    p06_unit *u = &s->u[i];
    u->dead = true;
    explode_fx(s, cadv_p06_unit_pos(s, i), 1.0f);
    if (u->kind == P06_K_PUMP) {
        --s->pumps_left;
        say(s, s->pumps_left > 0 ? "PUMP DESTROYED" : "THE PUMP STATION IS DOWN");
    }
    if (s->lock == i) {
        s->lock = -1;
    }
}

static void player_hit(p06_state *s, cadv_vec3 from, int amount)
{
    /* an impact on the side facing the shooter, at a random height */
    float ty = cadv_walker_torso_yaw(&s->mech);
    float rx = cadv_cosf(ty);
    float rz = -cadv_sinf(ty);
    float side = ((from.x - s->mech.pos.x) * rx + (from.z - s->mech.pos.z) * rz) > 0.0f ? 1.0f : -1.0f;
    float lat = side * cadv_rng_float01(&s->rng) * 2.4f;
    cadv_vec3 p = cadv_v3(s->mech.pos.x + rx * lat, s->mech.pos.y + 0.5f + cadv_rng_float01(&s->rng) * 9.0f, s->mech.pos.z + rz * lat);
    cadv_walker_zone z = cadv_walker_hit_zone(&s->mech, &k_player_def, p);
    int lost = s->mech.lost;
    if (!cadv_walker_alive(&s->mech) || s->win_timer > 0) {
        return;
    }
    amount = (int)((float)amount * dmg_scale(s) + 0.5f);
    if (amount < 1) {
        amount = 1;
    }
    s->hurt_flash = 8;
    sfx(s, CADV_SFX_HIT, 0.5f);
    if (cadv_walker_damage(&s->mech, z, amount)) {
        s->lose_timer = 180;
        say(s, "CRITICAL DAMAGE - EJECTING!");
        radio(s, "SANTA CLAUDE: Punch out, Sable! I'll pick you up.");
        explode_fx(s, s->mech.pos, 1.0f);
    } else if (s->mech.lost > lost) {
        char buf[72];
        snprintf(buf, sizeof buf, "%s DESTROYED", cadv_walker_zone_name(z));
        say(s, buf);
    }
}

static void unit_hit(p06_state *s, int i, cadv_vec3 at, int amount)
{
    p06_unit *u = &s->u[i];
    if (!targetable(s, i)) {
        return;
    }
    u->hurt = 6;
    if (u->kind == P06_K_WINGMAN) {
        u->hp = (int)cadv_maxf(60.0f, (float)(u->hp - amount)); /* wingmen limp on, never fall */
        return;
    }
    if (is_walker(u->kind)) {
        cadv_walker_zone z = cadv_walker_hit_zone(&u->w, cadv_p06_walker_def(u->kind), at);
        if (cadv_walker_damage(&u->w, z, amount)) {
            unit_destroyed(s, i);
        }
        return;
    }
    u->hp -= amount;
    if (u->hp <= 0) {
        u->hp = 0;
        if (u->kind == P06_K_CRAWLER) {
            u->dead = true;
            explode_fx(s, u->pos, 1.0f);
        } else {
            unit_destroyed(s, i);
        }
    }
}

static void hit_target(p06_state *s, int target, cadv_vec3 from, cadv_vec3 at, int amount)
{
    if (target == P06_PLAYER) {
        player_hit(s, from, amount);
    } else if (target >= 0) {
        unit_hit(s, target, at, amount);
    }
}

/* ---- shots ------------------------------------------------------------------------------ */

static void add_beam(p06_state *s, cadv_vec3 a, cadv_vec3 b, int kind)
{
    int i;
    int oldest = 0;
    for (i = 0; i < P06_MAX_BEAMS; ++i) {
        if (s->beam[i].ttl <= 0) {
            oldest = i;
            break;
        }
        if (s->beam[i].ttl < s->beam[oldest].ttl) {
            oldest = i;
        }
    }
    s->beam[oldest].a = a;
    s->beam[oldest].b = b;
    s->beam[oldest].kind = kind;
    s->beam[oldest].ttl = kind == P06_W_PPC ? 14 : 8;
}

static void add_proj(p06_state *s, int kind, int team, int owner, int target, cadv_vec3 pos, cadv_vec3 vel, int damage, float life)
{
    int i;
    for (i = 0; i < P06_MAX_PROJ; ++i) {
        p06_proj *p = &s->proj[i];
        if (!p->active) {
            p->active = true;
            p->kind = kind;
            p->team = team;
            p->owner = owner;
            p->target = target;
            p->pos = pos;
            p->vel = vel;
            p->damage = damage;
            p->life = life;
            return;
        }
    }
}

static float ray_sphere(cadv_vec3 o, cadv_vec3 d, cadv_vec3 c, float r)
{
    cadv_vec3 oc = cadv_v3_sub(c, o);
    float t = cadv_v3_dot(oc, d);
    cadv_vec3 p = cadv_v3_add(o, cadv_v3_scale(d, t));
    return t > 0.0f && cadv_v3_dist(p, c) <= r ? t : -1.0f;
}

/* The hostile under the crosshair (within 450 m, in sight), or -1. */
int cadv_p06_aim_target(const p06_state *s)
{
    cadv_vec3 e = cadv_p06_eye(s);
    cadv_vec3 f = cadv_walker_aim(&s->mech);
    float best = 450.0f;
    int hit = -1;
    int i;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (targetable(s, i) && s->u[i].team == P06_TEAM_GRAIL) {
            float t = ray_sphere(e, f, unit_centre(s, i), unit_radius(s->u[i].kind) * 1.4f);
            if (t > 0.0f && t < best && cadv_terrain_line_clear(&s->terrain, e, unit_centre(s, i))) {
                best = t;
                hit = i;
            }
        }
    }
    return hit;
}

/* A beam along the aim: the first hostile it meets, else the ground. */
static void player_beam(p06_state *s, int slot, cadv_vec3 from, cadv_vec3 dir)
{
    const p06_weapon_def *w = &k_weapons[s->loadout[slot]];
    float best = w->range;
    int hit = -1;
    int i;
    cadv_vec3 end;
    cadv_vec3 ground;
    if (cadv_terrain_raycast(&s->terrain, from, dir, w->range, &ground)) {
        best = cadv_v3_dist(ground, from);
    }
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (targetable(s, i) && s->u[i].team == P06_TEAM_GRAIL) {
            float t = ray_sphere(from, dir, unit_centre(s, i), unit_radius(s->u[i].kind));
            if (t > 0.0f && t < best) {
                best = t;
                hit = i;
            }
        }
    }
    end = cadv_v3_add(from, cadv_v3_scale(dir, best));
    add_beam(s, from, end, s->loadout[slot]);
    if (hit >= 0) {
        unit_hit(s, hit, end, w->damage);
    }
}

static bool slot_ok(const p06_state *s, int slot)
{
    static const cadv_walker_zone mount[P06_SLOTS] = {CADV_WZ_LA, CADV_WZ_RA, CADV_WZ_LT, CADV_WZ_RT};
    const p06_weapon_def *w = &k_weapons[s->loadout[slot]];
    return cadv_walker_has(&s->mech, mount[slot]) && s->cd[slot] <= 0.0f && (w->ammo == 0 || s->ammo[slot] > 0);
}

static cadv_vec3 muzzle(const p06_state *s, int slot)
{
    float ty = cadv_walker_torso_yaw(&s->mech);
    float rx = cadv_cosf(ty);
    float rz = -cadv_sinf(ty);
    float side = (slot % 2) ? 1.0f : -1.0f;
    float out = slot < 2 ? 2.6f : 1.4f;
    cadv_vec3 e = cadv_p06_eye(s);
    return cadv_v3(e.x + rx * side * out, e.y - (slot < 2 ? 1.6f : 0.4f), e.z + rz * side * out);
}

static void fire_slot(p06_state *s, int slot)
{
    const p06_weapon_def *w = &k_weapons[s->loadout[slot]];
    cadv_vec3 m = muzzle(s, slot);
    cadv_vec3 e = cadv_p06_eye(s);
    cadv_vec3 dir = cadv_walker_aim(&s->mech);
    /* converge on whatever the crosshair points at */
    cadv_vec3 aim_pt = cadv_v3_add(e, cadv_v3_scale(dir, w->range));
    cadv_vec3 hit;
    int t = cadv_p06_aim_target(s);
    int k;
    if (t >= 0) {
        aim_pt = unit_centre(s, t);
    } else if (cadv_terrain_raycast(&s->terrain, e, dir, w->range, &hit)) {
        aim_pt = hit;
    }
    dir = cadv_v3_norm(cadv_v3_sub(aim_pt, m));
    s->cd[slot] = w->cooldown;
    if (w->ammo > 0) {
        --s->ammo[slot];
    }
    cadv_walker_add_heat(&s->mech, &k_player_def, w->heat);
    if (w->speed <= 0.0f) {
        player_beam(s, slot, m, dir);
        sfx(s, CADV_SFX_SHOT, w->group == P06_G_ENERGY && s->loadout[slot] == P06_W_PPC ? 0.9f : 0.5f);
        return;
    }
    for (k = 0; k < w->count; ++k) {
        cadv_vec3 v = dir;
        if (w->count > 1) {
            v.x += ((float)k - 2.0f) * 0.02f;
            v.y += 0.04f + (float)(k % 2) * 0.02f;
            v = cadv_v3_norm(v);
        }
        add_proj(s, w->group == P06_G_MISSILE ? P06_PROJ_MISSILE : P06_PROJ_SHELL, P06_TEAM_US, P06_PLAYER,
                 w->group == P06_G_MISSILE ? s->lock : -1, m, cadv_v3_scale(v, w->speed), w->damage, w->range / w->speed + 0.5f);
    }
    sfx(s, CADV_SFX_SHOT, 0.6f);
}

static void fire_group(p06_state *s, int group, bool chain)
{
    int i;
    for (i = 0; i < P06_SLOTS; ++i) {
        int slot = chain ? (s->chain_next + i) % P06_SLOTS : i;
        if (k_weapons[s->loadout[slot]].group == group && slot_ok(s, slot)) {
            fire_slot(s, slot);
            if (chain) {
                s->chain_next = (slot + 1) % P06_SLOTS;
                for (i = 0; i < P06_SLOTS; ++i) {
                    if (k_weapons[s->loadout[i]].group == group && i != slot) {
                        s->cd[i] = cadv_maxf(s->cd[i], 0.25f); /* chain fire staggers the group */
                    }
                }
                return;
            }
        }
    }
}

/* Enemy and wingman weapons: beams roll to hit, shells and missiles fly. */
static void ai_beam(p06_state *s, int shooter, int target, int damage, float range, int kind)
{
    cadv_vec3 from = unit_centre(s, shooter);
    cadv_vec3 to = unit_centre(s, target);
    float d = cadv_v3_dist(from, to);
    float moving = target == P06_PLAYER ? cadv_absf(s->mech.speed) : 0.0f;
    float chance = 0.9f - d / 700.0f - moving / 45.0f;
    if (d > range || !cadv_terrain_line_clear(&s->terrain, from, to)) {
        return;
    }
    from.y += 2.0f;
    if (cadv_rng_float01(&s->rng) < chance) {
        add_beam(s, from, to, kind);
        hit_target(s, target, from, to, damage);
    } else {
        add_beam(s, from, cadv_v3(to.x + 6.0f * (cadv_rng_float01(&s->rng) - 0.5f), to.y + 4.0f, to.z + 6.0f * (cadv_rng_float01(&s->rng) - 0.5f)), kind);
    }
}

static void ai_shell(p06_state *s, int shooter, int target, int kind, int count, int damage, float speed, float range)
{
    cadv_vec3 from = unit_centre(s, shooter);
    cadv_vec3 to = unit_centre(s, target);
    float d = cadv_v3_dist(from, to);
    int k;
    if (d > range || !cadv_terrain_line_clear(&s->terrain, from, to)) {
        return;
    }
    if (target == P06_PLAYER && kind == P06_PROJ_SHELL) {
        /* lead the target a little */
        float lead = d / speed;
        to.x -= cadv_sinf(s->mech.legs_yaw) * s->mech.speed * lead * 0.7f;
        to.z -= cadv_cosf(s->mech.legs_yaw) * s->mech.speed * lead * 0.7f;
    }
    from.y += 2.0f;
    for (k = 0; k < count; ++k) {
        cadv_vec3 v = cadv_v3_norm(cadv_v3_sub(to, from));
        v.x += ((float)k - (float)(count - 1) * 0.5f) * 0.03f;
        v.y += kind == P06_PROJ_MISSILE ? 0.08f : 0.0f;
        add_proj(s, kind, s->u[shooter].team, shooter, kind == P06_PROJ_MISSILE ? target : -1, from, cadv_v3_scale(cadv_v3_norm(v), speed),
                 damage, range / speed + 1.0f);
    }
}

static void tick_proj(p06_state *s)
{
    int i;
    int k;
    for (i = 0; i < P06_MAX_PROJ; ++i) {
        p06_proj *p = &s->proj[i];
        cadv_vec3 next;
        bool boom = false;
        int victim = -1;
        if (!p->active) {
            continue;
        }
        if (p->kind == P06_PROJ_CANISTER) {
            continue;
        }
        p->life -= P06_DT;
        if (p->kind == P06_PROJ_MISSILE && p->target != -1) {
            bool live = p->target == P06_PLAYER ? cadv_walker_alive(&s->mech) : targetable(s, p->target);
            if (live) {
                cadv_vec3 want = cadv_v3_norm(cadv_v3_sub(unit_centre(s, p->target), p->pos));
                float sp = cadv_v3_len(p->vel);
                cadv_vec3 dir = cadv_v3_norm(cadv_v3_add(cadv_v3_scale(cadv_v3_norm(p->vel), 1.0f), cadv_v3_scale(want, 2.2f * P06_DT * 3.0f)));
                p->vel = cadv_v3_scale(dir, sp);
            }
        }
        next = cadv_v3_add(p->pos, cadv_v3_scale(p->vel, P06_DT));
        if (next.y <= ground_at(s, next.x, next.z) || p->life <= 0.0f) {
            boom = true;
        }
        if (p->team == P06_TEAM_GRAIL) {
            if (cadv_v3_dist(next, unit_centre(s, P06_PLAYER)) < 4.5f) {
                victim = P06_PLAYER;
            }
        }
        for (k = 0; k < P06_MAX_UNITS && victim == -1; ++k) {
            if (targetable(s, k) && s->u[k].team != p->team && cadv_v3_dist(next, unit_centre(s, k)) < unit_radius(s->u[k].kind) + 1.0f) {
                victim = k;
            }
        }
        if (victim != -1) {
            cadv_vec3 from = p->owner == P06_PLAYER ? s->mech.pos : cadv_p06_unit_pos(s, p->owner >= 0 ? p->owner : 0);
            hit_target(s, victim, from, next, p->damage);
            boom = true;
        }
        p->pos = next;
        if (boom) {
            p->active = false;
            if (p->kind == P06_PROJ_MISSILE && flat_dist(p->pos, s->mech.pos) < 200.0f) {
                sfx(s, CADV_SFX_HIT, 0.25f);
            }
        }
    }
    for (i = 0; i < P06_MAX_BEAMS; ++i) {
        if (s->beam[i].ttl > 0) {
            --s->beam[i].ttl;
        }
    }
}

/* ---- the lab -------------------------------------------------------------------------- */

static void lab(p06_state *s, const cadv_input_frame *in)
{
    int i;
    for (i = 0; i < P06_SLOTS; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i))) {
            s->lab_slot = i;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_PREV_WEAPON) || cadv_input_pressed(in, CADV_ACT_RADIAL)) {
        int step = cadv_input_pressed(in, CADV_ACT_PREV_WEAPON) ? P06_WEAPONS - 1 : 1;
        int old = s->loadout[s->lab_slot];
        int k;
        for (k = 0; k < P06_WEAPONS; ++k) {
            s->loadout[s->lab_slot] = (s->loadout[s->lab_slot] + step) % P06_WEAPONS;
            if (cadv_p06_tonnage(s) <= P06_TONNAGE) {
                break;
            }
        }
        if (s->loadout[s->lab_slot] != old) {
            say(s, k_weapons[s->loadout[s->lab_slot]].name);
            sfx(s, CADV_SFX_DOOR, 0.4f);
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_ALT_FIRE)) {
        s->paint = (s->paint + 1) % 3;
        say(s, cadv_p06_paint_name(s, s->paint));
    }
    if (cadv_input_pressed(in, CADV_ACT_USE) || cadv_input_pressed(in, CADV_ACT_FIRE)) {
        float yaw = s->mech.legs_yaw;
        cadv_vec3 p = s->mech.pos;
        if (s->paint == 2) {
            s->marks_found |= 1u; /* the friend's paint scheme */
        }
        repair_player(s, p.x, p.z, yaw);
        s->mission = P06_M1;
        s->intro = 240;
        say(s, "LAUNCH!");
        sfx(s, CADV_SFX_BELL, 0.8f);
        radio(s, "SANTA CLAUDE: I've considered three routes; the safest one is also the most scenic. Ho ho ho!");
    }
}

/* ---- the cockpit ------------------------------------------------------------------------ */

static float cooling(const p06_state *s)
{
    float c = 1.0f;
    int i;
    if (ground_at(s, s->mech.pos.x, s->mech.pos.z) < P06_SEA) {
        c *= 2.0f; /* wading in the reservoir */
    }
    if (s->mission == P06_M3) {
        for (i = 0; i < P06_VENTS; ++i) {
            if (flat_dist(s->mech.pos, s->vents[i]) < 10.0f) {
                c *= 3.5f;
                break;
            }
        }
    }
    if (s->cool_boost > 0) {
        c *= 2.5f;
    }
    return c;
}

static void call_coolant(p06_state *s)
{
    cadv_vec3 f;
    if (s->coolant_left <= 0 || s->coolant_timer > 0 || s->coolant_down) {
        radio(s, "SANTA CLAUDE: I'm all out of coolant, Sable. Try a steam vent.");
        return;
    }
    if (s->crawler < 0 || !s->u[s->crawler].active || s->u[s->crawler].dead || s->mission == P06_M2) {
        radio(s, "SANTA CLAUDE: Sleighbell is too far away for a drop, I'm afraid.");
        return;
    }
    --s->coolant_left;
    f = cadv_v3(-cadv_sinf(s->mech.legs_yaw), 0, -cadv_cosf(s->mech.legs_yaw));
    s->coolant_pos = cadv_v3(s->mech.pos.x + f.x * 20.0f, 0, s->mech.pos.z + f.z * 20.0f);
    s->coolant_pos.y = ground_at(s, s->coolant_pos.x, s->coolant_pos.z);
    s->coolant_timer = 180;
    radio(s, "SANTA CLAUDE: Coolant on its way. Mind your head. Ho ho ho!");
}

static void cockpit(p06_state *s, const cadv_input_frame *in)
{
    cadv_walker_ctl c;
    int i;
    memset(&c, 0, sizeof c);
    c.throttle = cadv_clampf(in->move_y, -1, 1);
    c.turn = cadv_clampf(in->move_x, -1, 1);
    c.twist = in->look_x;
    c.pitch = in->look_y;
    c.stop = cadv_input_pressed(in, CADV_ACT_CROUCH);
    c.centre = cadv_input_down(in, CADV_ACT_RUN);
    c.jump = cadv_input_down(in, CADV_ACT_JUMP);
    for (i = 0; i < P06_GROUPS; ++i) {
        if (cadv_input_pressed(in, (cadv_action)(CADV_ACT_WEAPON_1 + i))) {
            s->group = i;
        }
    }
    if (cadv_input_pressed(in, CADV_ACT_NEXT_WEAPON) || cadv_input_pressed(in, CADV_ACT_RADIAL)) {
        s->group = (s->group + 1) % P06_GROUPS;
    }
    if (cadv_input_pressed(in, CADV_ACT_PREV_WEAPON)) {
        s->group = (s->group + P06_GROUPS - 1) % P06_GROUPS;
    }
    if (cadv_input_pressed(in, CADV_ACT_WEAPON_4)) {
        s->chain = !s->chain;
        say(s, s->chain ? "CHAIN FIRE" : "GROUP FIRE");
    }
    if (cadv_input_pressed(in, CADV_ACT_MAP)) {
        s->map_open = !s->map_open;
    }
    if (cadv_input_pressed(in, CADV_ACT_USE)) {
        call_coolant(s);
    }
    if (s->mech.shutdown != s->was_shutdown) {
        /* reported a tick late: weapons and jets can trip it mid-tick */
        s->was_shutdown = s->mech.shutdown;
        if (s->mech.shutdown) {
            say(s, "HEAT CRITICAL - SHUTDOWN");
            sfx(s, CADV_SFX_DENIED, 1.0f);
        } else {
            say(s, "RESTART - SYSTEMS ONLINE");
        }
    }
    cadv_walker_tick(&s->mech, &k_player_def, &c, &s->terrain, cooling(s), P06_DT);
    for (i = 0; i < P06_SLOTS; ++i) {
        s->cd[i] -= P06_DT;
    }
    if (s->mech.shutdown || !cadv_walker_alive(&s->mech)) {
        return;
    }
    if (cadv_input_down(in, CADV_ACT_ALT_FIRE)) {
        for (i = 0; i < P06_GROUPS; ++i) {
            fire_group(s, i, false); /* alpha strike */
        }
    } else if (cadv_input_down(in, CADV_ACT_FIRE)) {
        fire_group(s, s->group, s->chain);
    }
    /* missile lock: hold a hostile under the crosshair */
    {
        int t = cadv_p06_aim_target(s);
        if (t >= 0 && t == s->lock) {
            s->lock_t = 1.0f;
        } else if (t >= 0) {
            static const float k_lock = 0.8f;
            s->lock_t += P06_DT;
            if (s->lock_t >= k_lock) {
                s->lock = t;
                sfx(s, CADV_SFX_PICKUP, 0.3f);
            } else if (s->lock >= 0 && s->lock_t < P06_DT * 1.5f) {
                s->lock_t = P06_DT;
            }
        } else {
            s->lock_t = 0.0f;
            if (s->lock >= 0) {
                cadv_vec3 to = cadv_v3_norm(cadv_v3_sub(unit_centre(s, s->lock), cadv_p06_eye(s)));
                if (!targetable(s, s->lock) || cadv_v3_dot(to, cadv_walker_aim(&s->mech)) < 0.95f) {
                    s->lock = -1;
                }
            }
        }
    }
}

/* ---- AI --------------------------------------------------------------------------------- */

static cadv_walker_ctl steer(const cadv_walker *w, cadv_vec3 goal, float want_speed, float max_speed)
{
    cadv_walker_ctl c;
    float dy = cadv_wrap_angle(yaw_to(w->pos, goal) - w->legs_yaw);
    float want_throttle = cadv_clampf(want_speed / max_speed, -1.0f, 1.0f);
    memset(&c, 0, sizeof c);
    c.turn = cadv_clampf(-dy * 2.0f, -1.0f, 1.0f);
    if (cadv_absf(dy) > 1.2f) {
        want_throttle = cadv_minf(want_throttle, 0.3f);
    }
    c.throttle = cadv_clampf((want_throttle - w->throttle) * 4.0f, -1.0f, 1.0f);
    if (w->blocked) {
        c.jump = true;
    }
    return c;
}

static void aim_torso(cadv_walker *w, cadv_walker_ctl *c, cadv_vec3 eye, cadv_vec3 at)
{
    float want = cadv_wrap_angle(yaw_to(w->pos, at) - w->legs_yaw);
    float d = cadv_sqrtf((at.x - eye.x) * (at.x - eye.x) + (at.z - eye.z) * (at.z - eye.z));
    c->twist = cadv_wrap_angle(want - w->twist);
    c->pitch = cadv_atan2f(at.y - eye.y, d) - w->pitch;
}

static int pick_target(p06_state *s, int i)
{
    p06_unit *u = &s->u[i];
    cadv_vec3 me = cadv_p06_unit_pos(s, i);
    int best = -1;
    float bd = 1e9f;
    int k;
    if (u->team == P06_TEAM_GRAIL) {
        float r = cadv_rng_float01(&s->rng);
        if (s->mission == P06_M1 && s->crawler >= 0 && targetable(s, s->crawler) && r < 0.4f &&
            flat_dist(me, s->u[s->crawler].pos) < 300.0f) {
            return s->crawler;
        }
        if (r > 0.8f && s->wingmen[0] >= 0 && s->u[s->wingmen[0]].active && s->mission != P06_M3) {
            return s->wingmen[r > 0.9f ? 1 : 0];
        }
        return cadv_walker_alive(&s->mech) ? P06_PLAYER : -1;
    }
    for (k = 0; k < P06_MAX_UNITS; ++k) {
        if (targetable(s, k) && s->u[k].team == P06_TEAM_GRAIL && s->u[k].kind != P06_K_WRECK) {
            float d = flat_dist(me, cadv_p06_unit_pos(s, k));
            if (d < bd) {
                bd = d;
                best = k;
            }
        }
    }
    return bd < 260.0f ? best : -1;
}

static void tick_walker_ai(p06_state *s, int i)
{
    p06_unit *u = &s->u[i];
    const cadv_walker_def *d = cadv_p06_walker_def(u->kind);
    cadv_walker_ctl c;
    cadv_vec3 me = u->w.pos;
    cadv_vec3 goal = me;
    cadv_vec3 tp;
    float want = 0.0f;
    float cool = 1.0f;
    int k;
    u->ai_t += P06_DT;
    if (u->target == -1 || u->ai_t > 4.0f || (u->target >= 0 && !targetable(s, u->target))) {
        u->target = pick_target(s, i);
        u->ai_t = 0.0f;
    }
    if (u->kind == P06_K_WINGMAN) {
        /* stay on the player's flank */
        float ty = s->mech.legs_yaw;
        float side = i == s->wingmen[0] ? -1.0f : 1.0f;
        goal = cadv_v3(s->mech.pos.x + cadv_cosf(ty) * side * 20.0f + cadv_sinf(ty) * 10.0f, 0,
                       s->mech.pos.z - cadv_sinf(ty) * side * 20.0f + cadv_cosf(ty) * 10.0f);
        want = flat_dist(me, goal) > 12.0f ? d->max_speed : 0.0f;
        if (flat_dist(me, s->mech.pos) > 160.0f) {
            /* catch up out of sight */
            u->w.pos = cadv_v3(goal.x, ground_at(s, goal.x, goal.z), goal.z);
        }
    } else if (u->target != -1) {
        tp = cadv_p06_unit_pos(s, u->target);
        if (u->kind == P06_K_KRAMPUS && u->w.heat > 70.0f) {
            /* too hot: off to the nearest steam vent */
            int v = 0;
            for (k = 1; k < P06_VENTS; ++k) {
                if (flat_dist(me, s->vents[k]) < flat_dist(me, s->vents[v])) {
                    v = k;
                }
            }
            goal = s->vents[v];
            want = flat_dist(me, goal) > 6.0f ? d->max_speed : 0.0f;
        } else {
            /* circle at a comfortable range */
            float dist = flat_dist(me, tp);
            float ang = yaw_to(tp, me) + u->strafe * 0.5f;
            float range = u->kind == P06_K_KRAMPUS ? 70.0f : 120.0f;
            goal = cadv_v3(tp.x - cadv_sinf(ang) * range, 0, tp.z - cadv_cosf(ang) * range);
            if (u->kind == P06_K_KRAMPUS) {
                /* the duel stays in the crater */
                float r = flat_dist(goal, s->crater);
                if (r > 45.0f) {
                    goal.x = s->crater.x + (goal.x - s->crater.x) * 45.0f / r;
                    goal.z = s->crater.z + (goal.z - s->crater.z) * 45.0f / r;
                }
            }
            want = d->max_speed * (dist > range * 1.6f ? 1.0f : 0.7f);
            if (u->ai_t > 3.5f && cadv_rng_below(&s->rng, 4) == 0) {
                u->strafe = -u->strafe;
            }
        }
    }
    c = steer(&u->w, goal, want, d->max_speed);
    if (u->kind == P06_K_KRAMPUS) {
        c.jump = false; /* no leaping over the rim */
    }
    if (u->target != -1) {
        cadv_vec3 eye = cadv_walker_eye(&u->w, d);
        aim_torso(&u->w, &c, eye, unit_centre(s, u->target));
    }
    if (u->kind == P06_K_KRAMPUS && u->hurt == 5 && u->w.fuel > 2.0f) {
        c.jump = true; /* a hop when hit */
    }
    if (u->kind == P06_K_KRAMPUS) {
        for (k = 0; k < P06_VENTS; ++k) {
            if (flat_dist(me, s->vents[k]) < 10.0f) {
                cool = 3.5f;
            }
        }
    }
    cadv_walker_tick(&u->w, d, &c, &s->terrain, cool, P06_DT);
    u->pos = u->w.pos;
    for (k = 0; k < 3; ++k) {
        u->cd[k] -= P06_DT;
    }
    if (u->target == -1 || u->w.shutdown) {
        return;
    }
    /* fire when the torso is on target */
    {
        float off = cadv_absf(cadv_wrap_angle(yaw_to(me, cadv_p06_unit_pos(s, u->target)) - cadv_walker_torso_yaw(&u->w)));
        if (off > 0.25f) {
            return;
        }
    }
    if (u->kind == P06_K_JACKAL) {
        if (u->cd[0] <= 0.0f && cadv_walker_has(&u->w, CADV_WZ_LA)) {
            ai_beam(s, i, u->target, 3, 260.0f, P06_W_LASER);
            u->cd[0] = 1.6f;
            cadv_walker_add_heat(&u->w, d, 5.0f);
        }
        if (u->cd[1] <= 0.0f && cadv_walker_has(&u->w, CADV_WZ_RA)) {
            ai_shell(s, i, u->target, P06_PROJ_SHELL, 1, 3, 200.0f, 280.0f);
            u->cd[1] = 1.2f;
        }
    } else if (u->kind == P06_K_KRAMPUS) {
        if (u->cd[0] <= 0.0f && u->w.heat < 60.0f && cadv_walker_has(&u->w, CADV_WZ_RA)) {
            ai_beam(s, i, u->target, 12, 380.0f, P06_W_PPC);
            u->cd[0] = 3.0f;
            cadv_walker_add_heat(&u->w, d, 14.0f);
        }
        if (u->cd[1] <= 0.0f && cadv_walker_has(&u->w, CADV_WZ_LT)) {
            ai_shell(s, i, u->target, P06_PROJ_MISSILE, 5, 2, 100.0f, 400.0f);
            u->cd[1] = 4.0f;
            cadv_walker_add_heat(&u->w, d, 4.0f);
        }
        if (u->cd[2] <= 0.0f && u->w.heat < 80.0f && cadv_walker_has(&u->w, CADV_WZ_LA)) {
            ai_beam(s, i, u->target, 5, 300.0f, P06_W_LASER);
            u->cd[2] = 1.4f;
            cadv_walker_add_heat(&u->w, d, 5.0f);
        }
    } else if (u->cd[0] <= 0.0f) {
        /* wingmen: steady laser fire */
        ai_beam(s, i, u->target, 7, 260.0f, P06_W_LASER);
        u->cd[0] = 1.2f;
    }
}

static void tick_other_ai(p06_state *s, int i)
{
    p06_unit *u = &s->u[i];
    int k;
    for (k = 0; k < 3; ++k) {
        u->cd[k] -= P06_DT;
    }
    if (u->hurt > 0) {
        --u->hurt;
    }
    if (u->kind == P06_K_PUMP || u->kind == P06_K_WRECK || u->kind == P06_K_CRAWLER) {
        return;
    }
    u->ai_t += P06_DT;
    if (u->target == -1 || u->ai_t > 4.0f || (u->target >= 0 && !targetable(s, u->target))) {
        u->target = pick_target(s, i);
        u->ai_t = 0.0f;
    }
    if (u->target == -1) {
        return;
    }
    {
        cadv_vec3 tp = cadv_p06_unit_pos(s, u->target);
        float dist = flat_dist(u->pos, tp);
        u->yaw = yaw_to(u->pos, tp);
        if (u->kind == P06_K_HOVER) {
            /* skim in, then sidestep around the target */
            float ang = yaw_to(tp, u->pos) + u->strafe * 0.04f;
            float r = cadv_maxf(90.0f, dist - 20.0f * P06_DT);
            cadv_vec3 to = cadv_v3(tp.x - cadv_sinf(ang) * r, 0, tp.z - cadv_cosf(ang) * r);
            cadv_vec3 step = cadv_v3_sub(to, u->pos);
            float len = cadv_sqrtf(step.x * step.x + step.z * step.z);
            float sp = cadv_minf(len, 20.0f * P06_DT);
            if (len > 0.01f) {
                float nx = u->pos.x + step.x / len * sp;
                float nz = u->pos.z + step.z / len * sp;
                float ng = cadv_maxf(ground_at(s, nx, nz), P06_SEA);
                if (ng - u->pos.y < 2.0f) {
                    u->pos.x = nx;
                    u->pos.z = nz;
                } else {
                    u->strafe = -u->strafe;
                }
            }
            u->pos.y = cadv_maxf(ground_at(s, u->pos.x, u->pos.z), P06_SEA);
            if (u->cd[0] <= 0.0f && dist < 300.0f) {
                ai_shell(s, i, u->target, P06_PROJ_MISSILE, 2, 3, 90.0f, 300.0f);
                u->cd[0] = 3.5f;
            }
        } else if (u->kind == P06_K_TURRET && u->cd[0] <= 0.0f && dist < 300.0f) {
            ai_shell(s, i, u->target, P06_PROJ_SHELL, 1, 4, 220.0f, 300.0f);
            u->cd[0] = 1.4f;
        }
    }
}

/* ---- missions ------------------------------------------------------------------------------ */

static int ambush_alive(const p06_state *s, int group)
{
    int i;
    int n = 0;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        n += targetable(s, i) && s->u[i].team == P06_TEAM_GRAIL && s->u[i].group == group && s->u[i].kind != P06_K_WRECK;
    }
    return n;
}

static void send_ambush(p06_state *s, int g)
{
    cadv_vec3 at = s->path[k_ambush_node[g] + 1];
    int jackals = g == 2 ? 3 : (g == 1 ? 1 : 2);
    int hovers = g == 1 ? 2 : 0;
    int i;
    if (s->cfg.difficulty == CADV_DIFF_STORY && jackals > 1) {
        --jackals;
    } else if (s->cfg.difficulty == CADV_DIFF_HARD) {
        ++hovers;
    }
    for (i = 0; i < jackals + hovers; ++i) {
        float ox = ((float)(i % 3) - 1.0f) * 14.0f;
        float oz = (float)(i / 3) * 14.0f - 20.0f;
        int u = spawn(s, i < jackals ? P06_K_JACKAL : P06_K_HOVER, P06_TEAM_GRAIL, at.x + ox, at.z + oz, yaw_to(at, s->u[s->crawler].pos));
        s->u[u].group = g + 1;
    }
    s->ambush_sent = g + 1;
    radio(s, g == 0 ? "SANTA CLAUDE: Contacts ahead, Sable. I'll hold here - I believe in your aim."
                    : g == 1 ? "SANTA CLAUDE: Hover tanks in the gorge! Sleighbell is holding position."
                             : "SANTA CLAUDE: Last ambush before the depot. Steady now.");
    sfx(s, CADV_SFX_BELL, 0.6f);
}

static void restart_convoy(p06_state *s)
{
    ++s->restarts;
    setup_m1(s, s->crawler_cp);
    s->ambush_sent = 0;
    while (s->ambush_sent < 3 && k_ambush_node[s->ambush_sent] < s->crawler_cp + 1) {
        ++s->ambush_sent; /* those were already cleared */
    }
    radio(s, "SANTA CLAUDE: Let's try that again, I believe in you.");
    say(s, "THE CONVOY IS LOST - BACK TO THE LAST CHECKPOINT");
}

static void mission1(p06_state *s)
{
    p06_unit *c = &s->u[s->crawler];
    int g = s->ambush_sent;
    bool hold = false;
    int i;
    if (c->dead) {
        restart_convoy(s);
        return;
    }
    /* friends on the radio */
    if (!(s->marks_found & 4u) && s->wingmen[0] >= 0 && flat_dist(s->mech.pos, s->u[s->wingmen[0]].w.pos) < 50.0f && s->ticks > 90) {
        char buf[120];
        s->marks_found |= 4u;
        snprintf(buf, sizeof buf, "%s: On your left flank, Sable.", s->mark_text[2]);
        radio(s, buf);
    } else if (!(s->marks_found & 8u) && s->wingmen[1] >= 0 && flat_dist(s->mech.pos, s->u[s->wingmen[1]].w.pos) < 50.0f && s->ticks > 420) {
        char buf[120];
        s->marks_found |= 8u;
        snprintf(buf, sizeof buf, "%s: On your right - venting hot air as usual.", s->mark_text[3]);
        radio(s, buf);
    }
    if (!(s->marks_found & 2u) && flat_dist(s->mech.pos, s->wreck_pos) < 35.0f) {
        char buf[96];
        s->marks_found |= 2u;
        snprintf(buf, sizeof buf, "A WRECK STENCILLED \"%s\"", s->mark_text[1]);
        say(s, buf);
    }
    for (i = 0; i < 3; ++i) {
        if (s->ambush_sent > i && ambush_alive(s, i + 1) == 0 && s->crawler_cp < k_ambush_node[i]) {
            s->crawler_cp = k_ambush_node[i];
            radio(s, "SANTA CLAUDE: Road's clear. Rolling on - mind the icy bits.");
        }
    }
    if (g < 3 && s->path_next > k_ambush_node[g]) {
        send_ambush(s, g);
    }
    /* Sleighbell holds while there is a fight or the escort lags behind */
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (targetable(s, i) && s->u[i].team == P06_TEAM_GRAIL && s->u[i].kind != P06_K_WRECK && flat_dist(cadv_p06_unit_pos(s, i), c->pos) < 150.0f) {
            hold = true;
        }
    }
    if (flat_dist(s->mech.pos, c->pos) > 120.0f) {
        if (!hold && s->ticks % 600 == 0) {
            radio(s, "SANTA CLAUDE: I'll wait for you, Sable. Convoys never leave anyone behind.");
        }
        hold = true;
    }
    if (s->path_next < P06_PATH_NODES && !hold) {
        cadv_vec3 to = s->path[s->path_next];
        float d = flat_dist(c->pos, to);
        float sp = 6.0f * P06_DT;
        c->yaw = yaw_to(c->pos, to);
        if (d <= sp) {
            c->pos.x = to.x;
            c->pos.z = to.z;
            ++s->path_next;
        } else {
            c->pos.x += (to.x - c->pos.x) / d * sp;
            c->pos.z += (to.z - c->pos.z) / d * sp;
        }
        c->pos.y = ground_at(s, c->pos.x, c->pos.z);
    }
    if (c->hp < c->hp_max / 2 && c->hurt == 5) {
        radio(s, "SANTA CLAUDE: Sleighbell is taking a few dents. Nothing a little paint won't fix... please hurry.");
    }
    if (c->hurt > 0) {
        --c->hurt;
    }
    /* at the depot: a gift from the cargo bay */
    if (s->path_next >= P06_PATH_NODES && !s->star_active && !s->star_taken) {
        s->star_active = true;
        s->star_pos = cadv_v3(c->pos.x + cadv_sinf(c->yaw) * 7.0f, c->pos.y + 3.0f, c->pos.z + cadv_cosf(c->yaw) * 7.0f);
        radio(s, "SANTA CLAUDE: We made it! I saved something for you in the cargo bay. Ho ho ho!");
        say(s, "COLLECT THE GIFT AT SLEIGHBELL'S CARGO BAY");
    }
    if (s->star_active && flat_dist(s->mech.pos, s->star_pos) < 14.0f) {
        s->star_active = false;
        s->star_taken = true;
        sfx(s, CADV_SFX_STAR, 1.0f);
        say(s, "THE SHINING STAR! MISSION 1 COMPLETE");
        if (s->checkpoint < 1) {
            s->checkpoint = 1;
        }
        setup_m2(s);
        radio(s, "SANTA CLAUDE: The Shining Star - every militia needs one. Now, those pumps are draining the reservoir.");
    }
}

static void mission2(p06_state *s)
{
    if (!s->reinforced && s->pumps_left < 4) {
        int i;
        s->reinforced = true;
        for (i = 0; i < (s->cfg.difficulty == CADV_DIFF_HARD ? 3 : 2); ++i) {
            spawn(s, P06_K_HOVER, P06_TEAM_GRAIL, 300.0f, 70.0f + 20.0f * (float)i, CADV_PI * 0.5f);
        }
        radio(s, "SANTA CLAUDE: Hover tanks inbound from the east. They look grumpy.");
    }
    if (s->pumps_left <= 0) {
        if (s->checkpoint < 2) {
            s->checkpoint = 2;
        }
        say(s, "MISSION 2 COMPLETE");
        setup_m3(s);
    }
}

static void mission3(p06_state *s)
{
    p06_unit *k = &s->u[s->krampus];
    if (s->mech.lost > s->lost_at_m3) {
        s->m3_component_lost = true;
    }
    if (k->w.hp[CADV_WZ_CT] < k_krampus_def.armour[CADV_WZ_CT] / 2 && k->hurt == 5 && !(s->ticks % 3)) {
        radio(s, "SANTA CLAUDE: He's limping! Keep the pressure on - politely.");
    }
    if (k->dead && s->win_timer == 0 && s->lose_timer == 0) {
        s->mission = P06_DONE;
        s->win_timer = 360;
        sfx(s, CADV_SFX_STAR, 1.0f);
        say(s, "KRAMPUS IS DOWN - CAMPAIGN COMPLETE!");
        radio(s, "SANTA CLAUDE: Krampus is down! Winterhold is safe. I'll put the kettle on.");
    }
}

static void tick_coolant(p06_state *s)
{
    if (s->cool_boost > 0) {
        --s->cool_boost;
    }
    if (s->coolant_timer > 0 && --s->coolant_timer == 0) {
        s->coolant_down = true;
        sfx(s, CADV_SFX_LAND, 0.8f);
    }
    if (s->coolant_down && flat_dist(s->mech.pos, s->coolant_pos) < 9.0f) {
        s->coolant_down = false;
        s->mech.heat = 0.0f;
        s->cool_boost = 600;
        say(s, "COOLANT FLUSH - HEAT ZERO");
        sfx(s, CADV_SFX_PICKUP, 0.8f);
    }
}

/* ---- tick ------------------------------------------------------------------------------ */

void cadv_p06_tick(void *state, const cadv_input_frame *in)
{
    p06_state *s = (p06_state *)state;
    int i;
    if (s->outcome != CADV_OUTCOME_RUNNING) {
        return;
    }
    ++s->ticks;
    if (s->message_timer > 0) {
        --s->message_timer;
    }
    if (s->radio_timer > 0) {
        --s->radio_timer;
    }
    if (s->intro > 0) {
        --s->intro;
    }
    if (s->hurt_flash > 0) {
        --s->hurt_flash;
    }
    if (s->shake > 0) {
        --s->shake;
    }
    if (s->win_timer > 0) {
        if (--s->win_timer == 0 || (s->win_timer < 300 && cadv_input_pressed(in, CADV_ACT_USE))) {
            s->outcome = CADV_OUTCOME_WON;
        }
        return;
    }
    if (s->lose_timer > 0) {
        if (--s->lose_timer == 0) {
            s->outcome = CADV_OUTCOME_LOST;
        }
        return;
    }
    if (cadv_input_pressed(in, CADV_ACT_PAUSE)) {
        s->outcome = CADV_OUTCOME_QUIT;
        return;
    }
    if (s->mission == P06_LAB) {
        lab(s, in);
        return;
    }
    cockpit(s, in);
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        p06_unit *u = &s->u[i];
        if (!u->active || u->dead) {
            continue;
        }
        if (is_walker(u->kind)) {
            if (u->hurt > 0) {
                --u->hurt;
            }
            tick_walker_ai(s, i);
        } else {
            tick_other_ai(s, i);
        }
    }
    tick_proj(s);
    tick_coolant(s);
    if (s->lose_timer > 0) {
        return;
    }
    if (s->mission == P06_M1) {
        mission1(s);
    } else if (s->mission == P06_M2) {
        mission2(s);
    } else if (s->mission == P06_M3) {
        mission3(s);
    }
}

/* ---- results & scripting -------------------------------------------------------------------- */

cadv_portal_result cadv_p06_result(const void *state)
{
    const p06_state *s = (const p06_state *)state;
    cadv_portal_result res;
    memset(&res, 0, sizeof res);
    res.outcome = s->outcome;
    res.star_collected = s->star_taken;
    res.checkpoint = s->checkpoint;
    res.marks_found = s->marks_found;
    res.ticks = s->ticks;
    if (s->outcome == CADV_OUTCOME_WON) {
        res.snowflakes = 1 + (s->mech.shutdowns == 0 ? 1 : 0) + (s->m3_component_lost ? 0 : 1);
    }
    return res;
}

bool cadv_p06_query(const void *state, const char *key, double *out)
{
    const p06_state *s = (const p06_state *)state;
    cadv_vec3 e = cadv_p06_eye(s);
    bool crawler = s->crawler >= 0 && s->u[s->crawler].active;
    bool krampus = s->krampus >= 0 && s->u[s->krampus].active;
    struct kv { const char *k; double v; };
    const struct kv table[] = {
        {"player.x", e.x},
        {"player.y", e.y},
        {"player.z", e.z},
        {"player.yaw", cadv_walker_torso_yaw(&s->mech)},
        {"player.health", s->mech.hp[CADV_WZ_CT]},
        {"heat", s->mech.heat},
        {"throttle", s->mech.throttle},
        {"speed", s->mech.speed},
        {"twist", s->mech.twist},
        {"shutdowns", s->mech.shutdowns},
        {"lost", s->mech.lost},
        {"mission", s->mission},
        {"lab", s->mission == P06_LAB},
        {"tonnage", cadv_p06_tonnage(s)},
        {"paint", s->paint},
        {"group", s->group},
        {"lock", s->lock >= 0},
        {"crawler.hp", crawler && !s->u[s->crawler].dead ? s->u[s->crawler].hp : 0},
        {"crawler.node", s->path_next},
        {"ambush", s->ambush_sent},
        {"enemies", cadv_p06_enemies_alive(s)},
        {"pumps", s->pumps_left},
        {"krampus.hp", krampus && !s->u[s->krampus].dead ? s->u[s->krampus].w.hp[CADV_WZ_CT] : 0},
        {"coolant", s->coolant_left},
        {"star.taken", s->star_taken},
        {"restarts", s->restarts},
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

void cadv_p06_teleport(void *state, float x, float y, float z, float yaw)
{
    p06_state *s = (p06_state *)state;
    (void)y;
    s->mech.pos = cadv_v3(x, ground_at(s, x, z), z);
    s->mech.legs_yaw = yaw;
    s->mech.twist = 0.0f;
    s->mech.pitch = 0.0f;
}

/* ---- completion bot -------------------------------------------------------------------- */

static void bot_press(cadv_input_frame *out, cadv_action a)
{
    out->pressed |= CADV_ACT_BIT(a);
    out->held |= CADV_ACT_BIT(a);
}

/* Walk the mech towards a point: legs on the bearing, throttle to 'want'. */
static void bot_drive(const p06_state *s, cadv_vec3 goal, float want, cadv_input_frame *out)
{
    const cadv_walker *w = &s->mech;
    float dy = cadv_wrap_angle(yaw_to(w->pos, goal) - w->legs_yaw);
    out->move_x = cadv_clampf(-dy * 2.5f, -1.0f, 1.0f);
    if (cadv_absf(dy) > 1.0f) {
        want = cadv_minf(want, 0.25f);
    }
    out->move_y = cadv_clampf((want - w->throttle) * 5.0f, -1.0f, 1.0f);
    if (w->blocked) {
        out->held |= CADV_ACT_BIT(CADV_ACT_JUMP);
    }
}

static int bot_target(const p06_state *s)
{
    int best = -1;
    float bd = 1e9f;
    int i;
    for (i = 0; i < P06_MAX_UNITS; ++i) {
        if (targetable(s, i) && s->u[i].team == P06_TEAM_GRAIL) {
            float d = flat_dist(s->mech.pos, cadv_p06_unit_pos(s, i));
            if (s->u[i].kind == P06_K_PUMP) {
                d += 60.0f; /* defenders first */
            }
            if (d < bd && d < 420.0f && cadv_terrain_line_clear(&s->terrain, cadv_p06_eye(s), unit_centre(s, i))) {
                bd = d;
                best = i;
            }
        }
    }
    return best;
}

/* The path node just ahead of the mech along the convoy route. */
static cadv_vec3 bot_route(const p06_state *s, cadv_vec3 to)
{
    int best = 0;
    float bd = 1e9f;
    int i;
    for (i = 0; i < P06_PATH_NODES; ++i) {
        float d = flat_dist(s->mech.pos, s->path[i]);
        if (d < bd) {
            bd = d;
            best = i;
        }
    }
    /* head for the next node towards 'to' (the route runs south to north) */
    if (best + 1 < P06_PATH_NODES && s->path[best].z > to.z + 1.0f && bd < 30.0f) {
        return s->path[best + 1];
    }
    return bd < 30.0f || s->path[best].z <= to.z + 1.0f ? to : s->path[best];
}

void cadv_p06_bot(const void *state, cadv_input_frame *out)
{
    const p06_state *s = (const p06_state *)state;
    const cadv_walker *w = &s->mech;
    cadv_vec3 eye = cadv_p06_eye(s);
    int t;
    memset(out, 0, sizeof *out);
    if (s->win_timer > 0) {
        if (s->win_timer < 280) {
            bot_press(out, CADV_ACT_USE);
        }
        return;
    }
    if (s->mission == P06_LAB) {
        if (s->paint != 2) {
            bot_press(out, CADV_ACT_ALT_FIRE); /* the friend's paint scheme */
        } else {
            bot_press(out, CADV_ACT_USE);
        }
        return;
    }
    if (w->shutdown) {
        return;
    }
    t = bot_target(s);
    /* where to walk */
    if (s->mission == P06_M1) {
        const p06_unit *c = &s->u[s->crawler];
        if (s->star_active) {
            bot_drive(s, s->star_pos, 0.6f, out);
        } else if (t >= 0 && flat_dist(w->pos, cadv_p06_unit_pos(s, t)) < 200.0f) {
            bot_drive(s, bot_route(s, cadv_p06_unit_pos(s, t)), flat_dist(w->pos, cadv_p06_unit_pos(s, t)) > 90.0f ? 0.8f : 0.0f, out);
        } else {
            /* lead the convoy a little */
            cadv_vec3 ahead = s->path[s->path_next < P06_PATH_NODES ? s->path_next : P06_PATH_NODES - 1];
            float d = flat_dist(w->pos, c->pos);
            bot_drive(s, bot_route(s, ahead), d < 60.0f ? 0.5f : 1.0f, out);
        }
    } else if (s->mission == P06_M2) {
        if (t >= 0) {
            /* orbit the target at range, never standing still */
            cadv_vec3 tp = cadv_p06_unit_pos(s, t);
            float a = cadv_atan2f(w->pos.z - tp.z, w->pos.x - tp.x) + 0.35f;
            cadv_vec3 g = cadv_v3(tp.x + cadv_cosf(a) * 90.0f, 0, tp.z + cadv_sinf(a) * 90.0f);
            g.x = cadv_clampf(g.x, 120.0f, 330.0f); /* keep to the valley floor */
            g.z = cadv_clampf(g.z, 40.0f, 180.0f);
            bot_drive(s, g, 0.9f, out);
        } else {
            bot_drive(s, s->pump_station, 0.9f, out);
        }
    } else if (s->mission == P06_M3) {
        if (s->coolant_down) {
            bot_drive(s, s->coolant_pos, 0.6f, out);
        } else if (w->heat > 70.0f) {
            /* cool off at the nearest vent */
            int v = 0;
            int k;
            for (k = 1; k < P06_VENTS; ++k) {
                if (flat_dist(w->pos, s->vents[k]) < flat_dist(w->pos, s->vents[v])) {
                    v = k;
                }
            }
            bot_drive(s, s->vents[v], flat_dist(w->pos, s->vents[v]) > 5.0f ? 0.7f : 0.0f, out);
        } else if (flat_dist(w->pos, s->crater) > 50.0f) {
            bot_drive(s, s->crater, 1.0f, out);
        } else {
            /* circle the crater at full speed: a moving target is hard to hit */
            float a = cadv_atan2f(w->pos.z - s->crater.z, w->pos.x - s->crater.x) + 0.45f;
            bot_drive(s, cadv_v3(s->crater.x + cadv_cosf(a) * 38.0f, 0, s->crater.z + cadv_sinf(a) * 38.0f), 1.0f, out);
        }
        if (w->heat > 80.0f && s->coolant_left > 0 && s->coolant_timer == 0 && !s->coolant_down) {
            bot_press(out, CADV_ACT_USE);
        }
    }
    /* torso on the target, fire with an eye on the heat */
    if (t >= 0) {
        cadv_vec3 at = unit_centre(s, t);
        float want = cadv_wrap_angle(yaw_to(w->pos, at) - w->legs_yaw);
        float d = flat_dist(eye, at);
        float dy = cadv_wrap_angle(want - w->twist);
        float dp = cadv_atan2f(at.y - eye.y, d) - w->pitch;
        out->look_x = cadv_clampf(dy, -0.08f, 0.08f);
        out->look_y = cadv_clampf(dp, -0.05f, 0.05f);
        if (cadv_absf(dy) < 0.03f && cadv_absf(dp) < 0.04f && d < 400.0f) {
            if (w->heat < 45.0f) {
                out->held |= CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
            } else {
                if (s->group != (s->lock >= 0 ? P06_G_MISSILE : P06_G_BALLISTIC)) {
                    bot_press(out, s->lock >= 0 ? CADV_ACT_WEAPON_3 : CADV_ACT_WEAPON_2);
                }
                out->held |= CADV_ACT_BIT(CADV_ACT_FIRE);
            }
        }
    } else {
        out->held |= CADV_ACT_BIT(CADV_ACT_RUN); /* torso back over the legs */
        out->look_y = cadv_clampf(-w->pitch, -0.05f, 0.05f);
    }
}

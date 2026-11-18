/* SPDX-License-Identifier: MIT */
/* Day 2 - Aurora Carpet: mana, balloons, castles, Vael and the star. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p02_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p02_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff, uint64_t seed)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 2;
    cfg.seed = seed;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(2), &cfg));
    st = (p02_state *)s.state;
}

static void start(int checkpoint) { start_with(checkpoint, CADV_DIFF_DEFAULT, 7); }

void setUp(void) { memset(&in, 0, sizeof in); }
void tearDown(void) { cadv_session_end(&s); }

static void tick(int n)
{
    int i;
    for (i = 0; i < n; ++i) {
        cadv_session_tick(&s, &in);
        in.pressed = 0;
    }
}

static void calm(void)
{
    int i;
    for (i = 0; i < P02_MAX_CREATURES; ++i) {
        st->wild_pool += st->creatures[i].active ? st->creatures[i].mana : 0;
        st->creatures[i].active = false;
        st->creatures[i].mana = 0;
    }
    st->vael.alive = false;
    st->vael.respawn = 0;
    st->spawn_cd = 1e9f;
}

/* Moves mana into a castle from the wild pool and unclaimed orbs (keeps the
 * world total intact). */
static void give(int owner, int amount)
{
    int i;
    int take = amount < st->wild_pool ? amount : st->wild_pool;
    st->wild_pool -= take;
    st->castle[owner].mana += take;
    amount -= take;
    for (i = 0; i < st->norbs && amount > 0; ++i) {
        p02_orb *o = &st->orbs[i];
        if (o->state == P02_ORB_FREE && o->owner == P02_NONE) {
            o->state = P02_ORB_GONE;
            st->castle[owner].mana += o->value;
            amount -= o->value;
        }
    }
    TEST_ASSERT_TRUE(amount <= 0);
}

static double query(const char *key)
{
    double v = -1.0;
    TEST_ASSERT_TRUE(cadv_p02_query(st, key, &v));
    return v;
}

static void test_portal_catalog__portal_02_aurora_carpet(void)
{
    const cadv_portal_desc *d = cadv_portal_get(2);
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_EQUAL_STRING("p02_aurora_carpet", d->id);
    TEST_ASSERT_TRUE(d->state_size >= sizeof(p02_state));
    start(0);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    TEST_ASSERT_FALSE(st->castle[P02_PLAYER].exists);
    TEST_ASSERT_TRUE(st->castle[P02_VAEL].exists);
    TEST_ASSERT_EQUAL_INT(2, st->castle[P02_VAEL].level);
    TEST_ASSERT_EQUAL_INT(40, (int)query("orbs.free"));
    TEST_ASSERT_EQUAL_INT(18, (int)query("creatures"));
    TEST_ASSERT_EQUAL_STRING("EVEN DR. DOOM COULD NOT HOLD THIS MUCH MANA", st->mark_text[0]);
}

static void test_portal_02_aurora_carpet__mana_is_conserved(void)
{
    int i;
    start(0);
    calm();
    st->vael.alive = true;
    for (i = 0; i < 6; ++i) {
        in.look_x = 0.05f;
        in.move_y = 1.0f;
        in.held = CADV_ACT_BIT(i % 2 ? CADV_ACT_FIRE : CADV_ACT_ALT_FIRE);
        tick(300);
        TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    }
    /* castle damage turns stored mana into free orbs, nothing is lost */
    st->wild_pool += st->castle[P02_VAEL].mana - 95;
    st->castle[P02_VAEL].mana = 95;
    TEST_ASSERT_EQUAL_INT(95, cadv_p02_damage_castle(st, P02_VAEL, 500));
    TEST_ASSERT_TRUE(st->castle[P02_VAEL].destroyed);
    TEST_ASSERT_FALSE(st->castle[P02_VAEL].exists);
    TEST_ASSERT_EQUAL_INT(0, cadv_p02_damage_castle(st, P02_VAEL, 10));
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static int orb_near(cadv_vec3 p, float r)
{
    int i;
    for (i = 0; i < st->norbs; ++i) {
        if (st->orbs[i].state == P02_ORB_FREE && cadv_v3_dist(st->orbs[i].pos, p) < r) {
            return i;
        }
    }
    return -1;
}

static void aim_at(cadv_vec3 target)
{
    cadv_vec3 eye = cadv_v3_add(st->player.pos, cadv_v3(0, 0.9f, 0));
    cadv_vec3 to = cadv_v3_sub(target, eye);
    st->player.yaw = cadv_atan2f(-to.x, -to.z);
    st->player.pitch = cadv_atan2f(to.y, cadv_sqrtf(to.x * to.x + to.z * to.z));
}

static void test_portal_02_aurora_carpet__claiming_transfers_ownership(void)
{
    int o;
    start(0);
    calm();
    o = 0;
    cadv_p02_teleport(st, st->orbs[o].pos.x + 12.0f, st->orbs[o].pos.y + 6.0f, st->orbs[o].pos.z, 0);
    aim_at(st->orbs[o].pos);
    in.held = CADV_ACT_BIT(CADV_ACT_ALT_FIRE);
    tick(20);
    in.held = 0;
    tick(30); /* let the last claim land */
    TEST_ASSERT_EQUAL_INT(P02_PLAYER, st->orbs[o].owner);
    /* claimed but not fetched yet: it does not count towards the share */
    TEST_ASSERT_EQUAL_INT(st->castle[P02_PLAYER].mana, cadv_p02_owned_mana(st, P02_PLAYER));
    /* Vael can steal it back with his own claim */
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_VAEL, P02_SPELL_CLAIM, cadv_v3_add(st->orbs[o].pos, cadv_v3(0, 5, 0)), cadv_v3(0, -1, 0)));
    tick(10);
    TEST_ASSERT_EQUAL_INT(P02_VAEL, st->orbs[o].owner);
    /* an already owned orb is not claimed again by its owner */
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_VAEL, P02_SPELL_CLAIM, cadv_v3_add(st->orbs[o].pos, cadv_v3(0, 5, 0)), cadv_v3(0, -1, 0)));
    tick(10);
    TEST_ASSERT_EQUAL_INT(P02_VAEL, st->orbs[o].owner);
}

static void test_portal_02_aurora_carpet__balloons_fly_over_raised_land(void)
{
    int o;
    int i;
    cadv_vec3 home;
    int before;
    start(1); /* castle on the home plateau */
    calm();
    home = st->castle[P02_PLAYER].pos;
    for (i = 0; i < st->norbs; ++i) {
        if (st->orbs[i].state == P02_ORB_FREE) {
            st->orbs[i].owner = P02_VAEL; /* keep our balloons for the one test orb */
        }
    }
    o = cadv_p02_add_orb(st, cadv_v3(home.x + 50.0f, 0, home.z), 10, P02_PLAYER);
    st->orbs[o].pos = cadv_v3(home.x + 50.0f, cadv_terrain_surface(&st->terrain, home.x + 50.0f, home.z) + 0.8f, home.z);
    st->wild_pool -= 10;
    /* a wall of raised land between castle and orb */
    for (i = -10; i <= 10; ++i) {
        cadv_terrain_raise_to(&st->terrain, home.x + 25.0f, home.z + (float)i * 3.0f, 4.0f, st->orbs[o].pos.y + 14.0f);
    }
    before = st->castle[P02_PLAYER].mana;
    for (i = 0; i < 60 * 60 && st->orbs[o].state != P02_ORB_GONE; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(P02_ORB_GONE, st->orbs[o].state);
    TEST_ASSERT_EQUAL_INT(before + 10, st->castle[P02_PLAYER].mana);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static void test_balloon_shot_down_drops_its_orb(void)
{
    int o;
    int b;
    start(1);
    calm();
    o = orb_near(st->castle[P02_PLAYER].pos, 80.0f);
    TEST_ASSERT_TRUE(o >= 0);
    st->orbs[o].owner = P02_PLAYER;
    tick(200);
    b = st->orbs[o].balloon;
    TEST_ASSERT_TRUE(b >= 0);
    st->balloons[b].carrying = true;
    st->orbs[o].state = P02_ORB_CARRIED;
    TEST_ASSERT_TRUE(cadv_p02_owned_mana(st, P02_PLAYER) > st->castle[P02_PLAYER].mana); /* in the air: counts */
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_VAEL, P02_SPELL_FIREBALL, cadv_v3_add(st->balloons[b].pos, cadv_v3(0, 1.0f, 0)), cadv_v3(0, -1, 0)));
    tick(3);
    TEST_ASSERT_FALSE(st->balloons[b].active);
    TEST_ASSERT_EQUAL_INT(P02_ORB_FREE, st->orbs[o].state);
    TEST_ASSERT_EQUAL_INT(P02_PLAYER, st->orbs[o].owner);
    /* a destroyed home castle grounds every balloon */
    tick(200);
    TEST_ASSERT_TRUE(st->orbs[o].balloon >= 0);
    cadv_p02_damage_castle(st, P02_PLAYER, 10000);
    tick(1);
    TEST_ASSERT_EQUAL_INT(0, (int)query("balloons"));
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static void test_portal_02_aurora_carpet__castle_grows_with_mana(void)
{
    TEST_ASSERT_EQUAL_INT(1, cadv_p02_castle_level_for(0));
    TEST_ASSERT_EQUAL_INT(1, cadv_p02_castle_level_for(149));
    TEST_ASSERT_EQUAL_INT(2, cadv_p02_castle_level_for(150));
    TEST_ASSERT_EQUAL_INT(3, cadv_p02_castle_level_for(350));
    TEST_ASSERT_EQUAL_INT(4, cadv_p02_castle_level_for(5000));
    start(0);
    calm();
    /* castle needs flat dry land away from Vael */
    TEST_ASSERT_FALSE(cadv_p02_place_castle(st, P02_PLAYER, cadv_v3(5, 0, 5)));                    /* sea */
    TEST_ASSERT_FALSE(cadv_p02_place_castle(st, P02_PLAYER, cadv_v3_add(st->castle[P02_VAEL].pos, cadv_v3(20, 0, 0))));
    cadv_p02_teleport(st, st->home_spot.x + 14.0f, st->home_spot.y + 12.0f, st->home_spot.z, 0);
    aim_at(st->home_spot);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_WEAPON_5);
    tick(1);
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(2);
    in.held = 0;
    TEST_ASSERT_TRUE(st->castle[P02_PLAYER].exists);
    TEST_ASSERT_EQUAL_INT(1, st->castle[P02_PLAYER].level);
    TEST_ASSERT_EQUAL_INT(1, st->checkpoint);
    TEST_ASSERT_EQUAL_INT(P02_PHASE_EXPANSION, st->phase);
    /* a second castle is refused */
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(40);
    in.held = 0;
    TEST_ASSERT_EQUAL_STRING("YOU ALREADY HAVE A CASTLE", st->message);
    give(P02_PLAYER, 370);
    tick(1);
    TEST_ASSERT_EQUAL_INT(3, st->castle[P02_PLAYER].level);
    TEST_ASSERT_TRUE(st->reached_level2);
}

static void test_raise_and_lower_land(void)
{
    cadv_vec3 hit;
    float before;
    int mana;
    start(1);
    calm();
    give(P02_PLAYER, 20);
    cadv_p02_teleport(st, 128.0f, 40.0f, 150.0f, 0);
    st->player.pitch = -0.6f;
    TEST_ASSERT_TRUE(cadv_terrain_raycast(&st->terrain, cadv_v3_add(st->player.pos, cadv_v3(0, 0.9f, 0)), cadv_flyer_forward(&st->player), 110.0f, &hit));
    before = cadv_terrain_height(&st->terrain, hit.x, hit.z);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_WEAPON_3);
    tick(1);
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_TRUE(cadv_terrain_height(&st->terrain, hit.x, hit.z) > before + 3.0f);
    TEST_ASSERT_EQUAL_INT(45, st->castle[P02_PLAYER].mana);
    tick(60);
    TEST_ASSERT_TRUE(cadv_terrain_raycast(&st->terrain, cadv_v3_add(st->player.pos, cadv_v3(0, 0.9f, 0)), cadv_flyer_forward(&st->player), 110.0f, &hit));
    before = cadv_terrain_height(&st->terrain, hit.x, hit.z);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_WEAPON_4);
    tick(1);
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_TRUE(cadv_terrain_height(&st->terrain, hit.x, hit.z) < before - 3.0f);
    /* castle wards protect the ground around castles; no mana, no spell */
    TEST_ASSERT_FALSE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_RAISE, cadv_v3_add(st->castle[P02_PLAYER].pos, cadv_v3(0, 20, 0)), cadv_v3(0, -1, 0)));
    mana = st->castle[P02_PLAYER].mana;
    st->castle[P02_PLAYER].mana = 2;
    st->wild_pool += mana - 2;
    tick(60);
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_EQUAL_STRING("NOT ENOUGH MANA", st->message);
    tick(60);
    give(P02_PLAYER, 38);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_WEAPON_3);
    tick(1);
    st->player.pitch = 0.0f;
    st->player.yaw = 0.0f;
    cadv_p02_teleport(st, st->castle[P02_PLAYER].pos.x, st->castle[P02_PLAYER].pos.y + 12.0f, st->castle[P02_PLAYER].pos.z + 4.0f, 0);
    st->player.pitch = -1.2f;
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_EQUAL_STRING("CASTLE WARDS PROTECT THAT GROUND", st->message);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static void test_spell_selection(void)
{
    start(0);
    in.pressed = CADV_ACT_BIT(CADV_ACT_WEAPON_6);
    tick(1);
    TEST_ASSERT_EQUAL_INT(P02_SPELL_FIREBALL, st->spell); /* volcano not learned yet */
    in.pressed = CADV_ACT_BIT(CADV_ACT_PREV_WEAPON);
    tick(1);
    TEST_ASSERT_EQUAL_INT(P02_SPELL_CASTLE, st->spell);
    in.pressed = CADV_ACT_BIT(CADV_ACT_NEXT_WEAPON);
    tick(1);
    TEST_ASSERT_EQUAL_INT(P02_SPELL_FIREBALL, st->spell);
    st->has_volcano = true;
    in.pressed = CADV_ACT_BIT(CADV_ACT_WEAPON_6);
    tick(1);
    TEST_ASSERT_EQUAL_INT(P02_SPELL_VOLCANO, st->spell);
    TEST_ASSERT_EQUAL_STRING("VOLCANO", cadv_p02_spell_name(st->spell));
    TEST_ASSERT_EQUAL_INT(120, cadv_p02_spell_cost(st->spell));
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_EQUAL_STRING("NOT ENOUGH MANA", st->message);
    tick(60);
    in.pressed = in.held = CADV_ACT_BIT(CADV_ACT_WEAPON_5);
    tick(1);
    st->player.pitch = 1.0f; /* aiming at the sky: no land */
    in.held = CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_EQUAL_STRING("NEED FLAT DRY LAND, AWAY FROM VAEL", st->message);
}

static void test_portal_02_aurora_carpet__ward_protects_vael_s_castle_until_nightfall(void)
{
    int mana;
    cadv_vec3 top;
    start(0);
    calm();
    st->vael.alive = true;
    top = cadv_v3_add(st->castle[P02_VAEL].pos, cadv_v3(0, 14, 0));
    mana = st->castle[P02_VAEL].mana;
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_FIREBALL, top, cadv_v3(0, -1, 0)));
    tick(30);
    TEST_ASSERT_EQUAL_INT(mana, st->castle[P02_VAEL].mana);
    st->vael.alive = false; /* ward still holds before nightfall */
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_FIREBALL, top, cadv_v3(0, -1, 0)));
    tick(30);
    TEST_ASSERT_EQUAL_INT(mana, st->castle[P02_VAEL].mana);
    st->phase = P02_PHASE_NIGHT;
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_FIREBALL, top, cadv_v3(0, -1, 0)));
    tick(30);
    TEST_ASSERT_EQUAL_INT(mana - 8, st->castle[P02_VAEL].mana);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static void test_volcano_erupts_under_a_castle(void)
{
    int i;
    start(2);
    calm();
    st->has_volcano = true;
    st->volcano_ticks = 0;
    st->wild_pool += st->castle[P02_VAEL].mana - 150;
    st->castle[P02_VAEL].mana = 150; /* make the eruption fatal */
    cadv_p02_spawn_creature(st, P02_WOLF, st->castle[P02_VAEL].pos, 15);
    st->wild_pool -= 15;
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_VOLCANO, cadv_v3_add(st->castle[P02_VAEL].pos, cadv_v3(4, 30, 4)), cadv_v3(0, -1, 0)));
    TEST_ASSERT_FALSE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_VOLCANO, cadv_v3_add(st->castle[P02_VAEL].pos, cadv_v3(4, 30, 4)), cadv_v3(0, -1, 0)));
    for (i = 0; i < 200; ++i) {
        tick(1);
    }
    TEST_ASSERT_TRUE(st->castle[P02_VAEL].destroyed);
    TEST_ASSERT_EQUAL_INT(0, (int)query("creatures"));
    TEST_ASSERT_TRUE(st->star_active);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static void test_portal_02_aurora_carpet__star_rises_at_75_percent(void)
{
    start(1);
    calm();
    give(P02_PLAYER, 740 - st->castle[P02_PLAYER].mana);
    tick(1);
    TEST_ASSERT_FALSE(st->star_active);
    give(P02_PLAYER, 10);
    tick(1);
    TEST_ASSERT_TRUE(st->star_active);
    TEST_ASSERT_TRUE(query("share") >= 0.75);
    cadv_p02_teleport(st, st->star_pos.x, st->star_pos.y, st->star_pos.z, 0);
    tick(2);
    TEST_ASSERT_TRUE(st->star_taken);
    TEST_ASSERT_EQUAL_INT(1, (int)query("star.taken"));
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_RUNNING, st->outcome); /* banner shows first */
    tick(150);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, st->outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
}

static void test_portal_catalog__portal_02_win_condition(void)
{
    cadv_portal_result r;
    start(1);
    calm();
    st->phase = P02_PHASE_NIGHT;
    cadv_p02_damage_castle(st, P02_VAEL, 10000);
    tick(1);
    TEST_ASSERT_TRUE(st->star_active);
    tick(300);
    TEST_ASSERT_TRUE(st->star_pos.y > st->castle[P02_VAEL].pos.y + 6.0f); /* rising */
    cadv_p02_teleport(st, st->star_pos.x, st->star_pos.y, st->star_pos.z, 0);
    tick(400);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.snowflakes >= 1);
}

static void test_portal_02_aurora_carpet__vael_hoards_the_light(void)
{
    start(0);
    calm();
    give(P02_VAEL, 560);
    tick(2);
    TEST_ASSERT_TRUE(st->lose_timer > 0);
    tick(200);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
    TEST_ASSERT_EQUAL_INT(0, cadv_session_result(&s).snowflakes);
}

static void test_portal_02_aurora_carpet__snowflakes_for_a_kept_castle(void)
{
    cadv_portal_result r;
    start(1);
    calm();
    give(P02_PLAYER, 170);
    tick(2);
    TEST_ASSERT_TRUE(st->castle_kept);
    st->star_active = true;
    st->star_pos = st->player.pos;
    tick(2);
    tick(400);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_EQUAL_INT(3, r.snowflakes); /* win + castle kept + under 15 min */
    cadv_session_end(&s);
    /* castle dropped below level 2: no second snowflake; story: no speed flake */
    start_with(1, CADV_DIFF_STORY, 3);
    calm();
    give(P02_PLAYER, 170);
    tick(2);
    cadv_p02_damage_castle(st, P02_PLAYER, 100);
    tick(2);
    TEST_ASSERT_FALSE(st->castle_kept);
    st->star_active = true;
    st->star_pos = st->player.pos;
    tick(402);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(1, r.snowflakes);
}

static void test_portal_02_aurora_carpet__resume_checkpoints(void)
{
    start(1);
    TEST_ASSERT_TRUE(st->castle[P02_PLAYER].exists);
    TEST_ASSERT_EQUAL_INT(P02_PHASE_EXPANSION, st->phase);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    cadv_session_end(&s);
    start(2);
    TEST_ASSERT_EQUAL_INT(P02_PHASE_NIGHT, st->phase);
    TEST_ASSERT_TRUE(st->castle[P02_PLAYER].mana >= 300);
    TEST_ASSERT_TRUE(st->scroll_active);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    /* the scroll teaches Volcano */
    cadv_p02_teleport(st, st->scroll_pos.x, st->scroll_pos.y, st->scroll_pos.z, 0);
    tick(1);
    TEST_ASSERT_TRUE(st->has_volcano);
    TEST_ASSERT_EQUAL_INT(P02_SPELL_VOLCANO, st->spell);
}

static void test_phases_and_siege(void)
{
    start(0);
    calm();
    tick(3 * 3600 + 2);
    TEST_ASSERT_EQUAL_INT(P02_PHASE_EXPANSION, st->phase); /* dusk ends even without a castle */
    st->ticks = 8 * 3600;
    tick(2);
    TEST_ASSERT_EQUAL_INT(P02_PHASE_NIGHT, st->phase);
    TEST_ASSERT_TRUE(st->scroll_active);
    st->ticks = 13 * 3600;
    tick(2);
    TEST_ASSERT_EQUAL_INT(P02_PHASE_SIEGE, st->phase);
    TEST_ASSERT_TRUE(st->marks_found & 1u); /* Vael's taunt names Dr. Doom */
}

static void test_early_night_when_the_wild_is_harvested(void)
{
    int i;
    start(1);
    calm();
    for (i = 0; i < st->norbs; ++i) {
        if (st->orbs[i].owner == P02_NONE) {
            st->orbs[i].owner = P02_VAEL;
        }
    }
    st->ticks = 4 * 3600;
    tick(2);
    TEST_ASSERT_EQUAL_INT(P02_PHASE_NIGHT, st->phase);
}

static void test_portal_02_aurora_carpet__friend_marks_in_the_fjords(void)
{
    start(0);
    calm();
    TEST_ASSERT_EQUAL_INT(0, (int)query("marks"));
    cadv_p02_teleport(st, st->rune_centre.x, 60.0f, st->rune_centre.z + 20.0f, 0);
    tick(1);
    TEST_ASSERT_EQUAL_INT(2, (int)query("marks"));
    cadv_p02_teleport(st, st->rock_pos.x, st->rock_pos.y + 10.0f, st->rock_pos.z - 20.0f, 0);
    tick(1);
    TEST_ASSERT_EQUAL_INT(6, (int)query("marks"));
    /* the runes spell the friend's name into the middle isle */
    TEST_ASSERT_TRUE(cadv_terrain_roughness(&st->terrain, st->rune_centre.x, st->rune_centre.z, 12.0f) > 1.0f);
}

static void test_creatures_bite_and_the_carpet_carries_you_home(void)
{
    int c;
    start(1);
    calm();
    c = cadv_p02_spawn_creature(st, P02_WOLF, st->castle[P02_PLAYER].pos, 15);
    st->wild_pool -= 15;
    cadv_p02_teleport(st, st->creatures[c].pos.x, st->creatures[c].pos.y + 1.0f, st->creatures[c].pos.z + 1.0f, 0);
    st->creatures[c].pos = cadv_v3_sub(st->player.pos, cadv_v3(0, 1.0f, 1.0f));
    tick(5);
    TEST_ASSERT_TRUE(st->hp < P02_PLAYER_HP);
    st->hp = 1;
    st->creatures[c].cooldown = 0.0f;
    st->creatures[c].pos = cadv_v3_sub(st->player.pos, cadv_v3(0, 1.0f, 1.0f));
    tick(3);
    TEST_ASSERT_EQUAL_INT(1, (int)query("deaths"));
    TEST_ASSERT_EQUAL_INT(P02_PLAYER_HP, st->hp);
    TEST_ASSERT_EQUAL_STRING("THE CARPET CARRIES YOU HOME...", st->message);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    /* health regenerates once out of combat */
    st->hp = 50;
    st->hurt_cd = 0.0f;
    st->creatures[c].active = false;
    st->wild_pool += st->creatures[c].mana;
    tick(120);
    TEST_ASSERT_TRUE(st->hp > 50);
}

static void test_killing_creatures_releases_their_mana(void)
{
    int c;
    int free_before;
    start(0);
    calm();
    c = cadv_p02_spawn_creature(st, P02_GRIFFIN, cadv_v3(200.f, 0, 192.f), 30);
    st->wild_pool -= 30;
    free_before = (int)query("orbs.free");
    st->creatures[c].hp = 1;
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_FIREBALL, cadv_v3_add(st->creatures[c].pos, cadv_v3(0, 0.8f, 4)), cadv_v3(0, 0, -1)));
    tick(10);
    TEST_ASSERT_FALSE(st->creatures[c].active);
    TEST_ASSERT_EQUAL_INT(free_before + 1, (int)query("orbs.free"));
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    /* the wild reclaims spent mana as new creatures */
    st->spawn_cd = 0.0f;
    st->wild_pool += 0;
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static void test_vael_duels_and_tumbles(void)
{
    int i;
    start(1);
    calm();
    st->vael.alive = true;
    st->vael.fly.pos = cadv_v3_add(st->player.pos, cadv_v3(0, 0, -20));
    for (i = 0; i < 200 && st->hp == P02_PLAYER_HP; ++i) {
        tick(1);
    }
    TEST_ASSERT_TRUE(st->hp < P02_PLAYER_HP); /* he duels the apprentice */
    st->vael.hp = 30;
    TEST_ASSERT_TRUE(cadv_p02_cast(st, P02_PLAYER, P02_SPELL_FIREBALL, cadv_v3_add(st->vael.fly.pos, cadv_v3(0, 0, 4)), cadv_v3(0, 0, -1)));
    tick(6);
    TEST_ASSERT_FALSE(st->vael.alive);
    TEST_ASSERT_EQUAL_INT(0, (int)query("vael.alive"));
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    st->vael.respawn = 1;
    tick(1);
    TEST_ASSERT_TRUE(st->vael.alive);
    /* low on health he retreats home to heal */
    st->vael.hp = 10;
    st->vael.fly.pos = cadv_v3_add(st->castle[P02_VAEL].pos, cadv_v3(0, 14, 0));
    tick(120);
    TEST_ASSERT_EQUAL_INT(3, st->vael.mode);
    TEST_ASSERT_TRUE(st->vael.hp > 10);
}

static void test_siege_and_turrets(void)
{
    int mana;
    start(1);
    calm();
    give(P02_PLAYER, 170); /* level 2: turrets */
    st->phase = P02_PHASE_SIEGE;
    st->vael.alive = true;
    st->vael.hp = P02_VAEL_HP;
    st->vael.fly.pos = cadv_v3_add(st->castle[P02_PLAYER].pos, cadv_v3(30, 20, 0));
    cadv_p02_teleport(st, 30.0f, 30.0f, 30.0f, 0); /* far from the fight */
    mana = st->castle[P02_PLAYER].mana;
    tick(600);
    TEST_ASSERT_TRUE(st->vael.mode >= 2); /* raiding, or retreating from the turrets */
    TEST_ASSERT_TRUE(st->castle[P02_PLAYER].mana < mana || st->vael.hp < P02_VAEL_HP);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
}

static void test_quit_and_queries(void)
{
    double v;
    start(0);
    TEST_ASSERT_FALSE(cadv_p02_query(st, "nope", &v));
    TEST_ASSERT_EQUAL_INT(0, (int)query("phase"));
    TEST_ASSERT_EQUAL_INT(1000, (int)query("total_mana"));
    query("player.x");
    query("player.y");
    query("player.z");
    query("player.yaw");
    query("player.health");
    query("vael.share");
    query("castle.mana");
    query("vael.castle.level");
    query("vael.castle.destroyed");
    query("orbs.mine");
    query("orbs.carried");
    query("star.active");
    query("has_volcano");
    query("spell");
    query("outcome");
    query("ticks");
    in.pressed = CADV_ACT_BIT(CADV_ACT_PAUSE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1); /* finished portals ignore further ticks */
}

static void run_bot(cadv_difficulty diff, uint64_t seed)
{
    int i;
    start_with(0, diff, seed);
    for (i = 0; i < 40000 && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_p02_bot(st, &in);
        cadv_session_tick(&s, &in);
    }
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    TEST_ASSERT_EQUAL_INT(CADV_P02_WORLD_MANA, cadv_p02_total_mana(st));
    cadv_session_end(&s);
}

static void test_bot_finishes_on_every_difficulty(void)
{
    run_bot(CADV_DIFF_STORY, 1);
    run_bot(CADV_DIFF_DEFAULT, 2);
    run_bot(CADV_DIFF_HARD, 3);
    start(0); /* tearDown ends this one */
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__portal_02_aurora_carpet);
    RUN_TEST(test_portal_02_aurora_carpet__mana_is_conserved);
    RUN_TEST(test_portal_02_aurora_carpet__claiming_transfers_ownership);
    RUN_TEST(test_portal_02_aurora_carpet__balloons_fly_over_raised_land);
    RUN_TEST(test_balloon_shot_down_drops_its_orb);
    RUN_TEST(test_portal_02_aurora_carpet__castle_grows_with_mana);
    RUN_TEST(test_raise_and_lower_land);
    RUN_TEST(test_spell_selection);
    RUN_TEST(test_portal_02_aurora_carpet__ward_protects_vael_s_castle_until_nightfall);
    RUN_TEST(test_volcano_erupts_under_a_castle);
    RUN_TEST(test_portal_02_aurora_carpet__star_rises_at_75_percent);
    RUN_TEST(test_portal_catalog__portal_02_win_condition);
    RUN_TEST(test_portal_02_aurora_carpet__vael_hoards_the_light);
    RUN_TEST(test_portal_02_aurora_carpet__snowflakes_for_a_kept_castle);
    RUN_TEST(test_portal_02_aurora_carpet__resume_checkpoints);
    RUN_TEST(test_phases_and_siege);
    RUN_TEST(test_early_night_when_the_wild_is_harvested);
    RUN_TEST(test_portal_02_aurora_carpet__friend_marks_in_the_fjords);
    RUN_TEST(test_creatures_bite_and_the_carpet_carries_you_home);
    RUN_TEST(test_killing_creatures_releases_their_mana);
    RUN_TEST(test_vael_duels_and_tumbles);
    RUN_TEST(test_siege_and_turrets);
    RUN_TEST(test_quit_and_queries);
    RUN_TEST(test_bot_finishes_on_every_difficulty);
    return UNITY_END();
}

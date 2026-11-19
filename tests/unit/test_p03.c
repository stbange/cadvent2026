/* SPDX-License-Identifier: MIT */
/* Day 3 - Frostfront: the commander on foot, the tablet, the Spire and the star. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p03_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p03_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 3;
    cfg.seed = 7;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(3), &cfg));
    st = (p03_state *)s.state;
}

static void start(int checkpoint) { start_with(checkpoint, CADV_DIFF_DEFAULT); }

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

static void press(cadv_action a)
{
    in.pressed |= CADV_ACT_BIT(a);
    in.held |= CADV_ACT_BIT(a);
    tick(1);
    in.held &= ~CADV_ACT_BIT(a);
}

static double query(const char *key)
{
    double v = -1.0;
    TEST_ASSERT_TRUE(cadv_p03_query(st, key, &v));
    return v;
}

static void aim(float x, float y, float z)
{
    cadv_vec3 e = cadv_p03_eye(st);
    cadv_vec3 to = cadv_v3_sub(cadv_v3(x, y, z), e);
    st->yaw = cadv_atan2f(-to.x, -to.z);
    st->pitch = cadv_atan2f(to.y, cadv_sqrtf(to.x * to.x + to.z * to.z));
}

static void aim_cell(int cx, int cz) { aim(((float)cx + 0.5f) * P03_CELL, 0.0f, ((float)cz + 0.5f) * P03_CELL); }

static void stand(int cx, int cz)
{
    cadv_p03_teleport(st, ((float)cx + 0.5f) * P03_CELL, 0, ((float)cz + 0.5f) * P03_CELL, 0);
}

static void calm(void)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->rts.u[i].alive && st->rts.u[i].owner == P03_ORDER) {
            st->rts.u[i].alive = false;
        }
    }
    st->ai.wave_timer = -1000000;
    st->ai.ntrain = 0;
    st->ai.harvester_def = -1;
}

static void buy(int item)
{
    while (st->tab_item != item) {
        press(CADV_ACT_NEXT_WEAPON);
    }
    press(CADV_ACT_USE);
}

static void test_portal_catalog__portal_03_frostfront(void)
{
    const cadv_portal_desc *d = cadv_portal_get(3);
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_EQUAL_STRING("p03_frostfront", d->id);
    TEST_ASSERT_TRUE(d->state_size >= sizeof(p03_state));
    start(0);
    TEST_ASSERT_EQUAL_INT(1, (int)query("buildings"));
    TEST_ASSERT_EQUAL_INT(3500, (int)query("credits"));
    TEST_ASSERT_EQUAL_INT(2400, (int)query("spire.hp"));
    TEST_ASSERT_TRUE(query("enemy.buildings") >= 12);
    TEST_ASSERT_EQUAL_INT(300, (int)query("player.health"));
    TEST_ASSERT_EQUAL_STRING("DR. DOOM RESEARCH LAB", st->mark_text[0]);
    TEST_ASSERT_EQUAL_STRING("Power Plant", cadv_p03_tab_name(0));
    TEST_ASSERT_EQUAL_STRING("Recon Buggy", cadv_p03_tab_name(P03_TAB_ITEMS - 1));
    TEST_ASSERT_EQUAL_INT(300, cadv_p03_tab_cost(0));
    TEST_ASSERT_EQUAL_INT(100, cadv_p03_tab_cost(P03_TAB_STRUCTURES));
    TEST_ASSERT_NOT_NULL(cadv_p03_units());
}

static void test_tablet_build_and_place(void)
{
    int harv;
    start(0);
    calm();
    press(CADV_ACT_PREV_WEAPON);
    TEST_ASSERT_EQUAL_INT(P03_TAB_ITEMS - 1, st->tab_item);
    press(CADV_ACT_RADIAL);
    TEST_ASSERT_EQUAL_INT(0, st->tab_item);
    buy(P03_TAB_STRUCTURES); /* rifleman without barracks */
    TEST_ASSERT_EQUAL_STRING("NEEDS A BARRACKS", st->message);
    buy(P03_TAB_STRUCTURES + 3); /* harvester without factory */
    TEST_ASSERT_EQUAL_STRING("NEEDS A WAR FACTORY", st->message);
    buy(1); /* refinery */
    TEST_ASSERT_EQUAL_STRING("BUILDING", st->message);
    TEST_ASSERT_EQUAL_INT(2300, (int)query("credits"));
    tick(700);
    TEST_ASSERT_TRUE(st->placing);
    TEST_ASSERT_EQUAL_INT(1, (int)query("placing"));
    /* placement rules through the beam */
    stand(31, 59);
    st->pitch = 0.5f;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_STRING("AIM THE BEAM AT THE GROUND", st->message);
    aim_cell(31, 45);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_STRING("TOO FAR FROM THE BASE", st->message);
    aim_cell(31, 55);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_STRING("CANNOT BUILD THERE", st->message);
    press(CADV_ACT_ALT_FIRE);
    TEST_ASSERT_FALSE(st->placing);
    buy(1);
    TEST_ASSERT_TRUE(st->placing);
    harv = (int)query("harvesters");
    aim_cell(35, 55);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_STRING("STRUCTURE PLACED", st->message);
    TEST_ASSERT_EQUAL_INT(2, (int)query("buildings"));
    TEST_ASSERT_EQUAL_INT(harv + 1, (int)query("harvesters")); /* comes with a harvester */
    /* money runs out; a full queue is refused */
    st->rts.side[P03_ACCORD].credits = 0;
    buy(0);
    TEST_ASSERT_EQUAL_STRING("INSUFFICIENT FUNDS", st->message);
    st->rts.side[P03_ACCORD].credits = 100000;
    st->rts.side[P03_ACCORD].ready_building = -1;
    st->rts.side[P03_ACCORD].queued[0] = CADV_RTS_QUEUE;
    st->rts.side[P03_ACCORD].queue[0][0].def = P03_B_FACTORY;
    st->rts.side[P03_ACCORD].queue[0][0].progress = 0;
    st->rts.side[P03_ACCORD].queue[0][0].building = true;
    buy(0);
    TEST_ASSERT_EQUAL_STRING("QUEUE FULL", st->message);
    st->rts.side[P03_ACCORD].queued[0] = 0;
    cadv_rts_add_building(&st->rts, P03_ACCORD, P03_B_BARRACKS, 26, 51);
    st->rts.side[P03_ACCORD].queued[1] = CADV_RTS_QUEUE;
    buy(P03_TAB_STRUCTURES);
    TEST_ASSERT_EQUAL_STRING("QUEUE FULL", st->message);
    st->rts.side[P03_ACCORD].queued[1] = 0;
    st->rts.side[P03_ACCORD].credits = 0;
    buy(P03_TAB_STRUCTURES);
    TEST_ASSERT_EQUAL_STRING("INSUFFICIENT FUNDS", st->message);
    st->rts.side[P03_ACCORD].credits = 1000;
    buy(P03_TAB_STRUCTURES);
    TEST_ASSERT_EQUAL_STRING("TRAINING", st->message);
    press(CADV_ACT_MAP);
    TEST_ASSERT_TRUE(st->tablet_open);
}

static int own_unit(int def)
{
    int i;
    for (i = 0; i < CADV_RTS_MAX_UNITS; ++i) {
        if (st->rts.u[i].alive && st->rts.u[i].owner == P03_ACCORD && st->rts.u[i].def == def) {
            return i;
        }
    }
    return -1;
}

static int spawn(int side, int def, int cx, int cz)
{
    return cadv_rts_add_unit(&st->rts, side, def, ((float)cx + 0.5f) * P03_CELL, ((float)cz + 0.5f) * P03_CELL);
}

static void test_selection_and_orders(void)
{
    int a;
    int b;
    int e;
    int eng;
    start(1);
    calm();
    a = spawn(P03_ACCORD, P03_U_TANK, 31, 48);
    b = spawn(P03_ACCORD, P03_U_RIFLE, 33, 48);
    eng = spawn(P03_ACCORD, P03_U_ENGINEER, 29, 48);
    stand(31, 53);
    /* pick one unit, add another with RUN */
    aim(st->rts.u[a].x, 1.0f, st->rts.u[a].z);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(1, (int)query("selected"));
    TEST_ASSERT_EQUAL_STRING("Light Tank", st->message);
    aim(st->rts.u[b].x, 0.9f, st->rts.u[b].z);
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    press(CADV_ACT_FIRE);
    in.held = 0;
    TEST_ASSERT_EQUAL_INT(2, (int)query("selected"));
    /* assign group 1, clear, recall */
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    press(CADV_ACT_WEAPON_1);
    in.held = 0;
    TEST_ASSERT_EQUAL_STRING("GROUP SET", st->message);
    press(CADV_ACT_CROUCH);
    TEST_ASSERT_EQUAL_INT(0, (int)query("selected"));
    press(CADV_ACT_WEAPON_1);
    TEST_ASSERT_EQUAL_INT(2, (int)query("selected"));
    /* with a squad selected FIRE orders, even on top of an own unit */
    aim_cell(31, 40);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_STRING("ACKNOWLEDGED - ADVANCING", st->message);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_ATTACK_MOVE, st->rts.u[a].order);
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    aim_cell(30, 40);
    in.pressed = CADV_ACT_BIT(CADV_ACT_FIRE);
    in.held |= CADV_ACT_BIT(CADV_ACT_FIRE);
    tick(1);
    in.held = 0;
    TEST_ASSERT_TRUE(st->rts.u[a].order == CADV_ORDER_MOVE || query("selected") == 2);
    /* attack an enemy in sight */
    e = spawn(P03_ORDER, P03_U_RIFLE, 31, 44);
    tick(1);
    aim(st->rts.u[e].x, 0.9f, st->rts.u[e].z);
    press(CADV_ACT_WEAPON_1);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_STRING("TARGET ACQUIRED", st->message);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_ATTACK, st->rts.u[a].order);
    /* sweep select around a point; engineers walk to buildings they are sent to */
    aim_cell(30, 48);
    press(CADV_ACT_ALT_FIRE);
    TEST_ASSERT_EQUAL_STRING("UNITS SELECTED", st->message);
    press(CADV_ACT_WEAPON_6);
    TEST_ASSERT_EQUAL_INT(1, (int)query("selected"));
    {
        const cadv_rts_building *lab = &st->rts.b[st->lab];
        aim(((float)lab->x + 1.5f) * P03_CELL, 2.0f, ((float)lab->z + 1.0f) * P03_CELL);
    }
    stand(42, 30);
    {
        const cadv_rts_building *lab = &st->rts.b[st->lab];
        aim(((float)lab->x + 1.5f) * P03_CELL, 2.0f, ((float)lab->z + 1.0f) * P03_CELL);
    }
    TEST_ASSERT_EQUAL_INT(CADV_RTS_TARGET_BUILDING + st->lab, cadv_p03_aim_target(st, false));
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_MOVE, st->rts.u[eng].order);
    press(CADV_ACT_WEAPON_5);
    TEST_ASSERT_EQUAL_INT(2, (int)query("army"));
    /* a selected harvester just goes back to work */
    memset(st->selected, 0, sizeof st->selected);
    st->selected[own_unit(P03_U_HARVESTER)] = 1;
    aim_cell(40, 40);
    stand(31, 53);
    aim_cell(31, 45);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(CADV_ORDER_HARVEST, st->rts.u[own_unit(P03_U_HARVESTER)].order);
}

static void test_sidearm_and_drone(void)
{
    int e;
    int hp;
    start(0);
    calm();
    e = spawn(P03_ORDER, P03_U_TANK, 31, 52);
    stand(31, 58);
    tick(1);
    aim(st->rts.u[e].x, 1.0f, st->rts.u[e].z);
    hp = st->rts.u[e].hp;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(hp - 12, st->rts.u[e].hp);
    st->rts.u[e].alive = false;
    press(CADV_ACT_JUMP);
    TEST_ASSERT_EQUAL_STRING("RECON DRONE UP", st->message);
    tick(60);
    TEST_ASSERT_TRUE(query("player.y") > 25.0f);
    in.move_y = 1.0f;
    {
        double z0 = query("player.z");
        tick(10);
        TEST_ASSERT_EQUAL_DOUBLE(z0, query("player.z")); /* the body waits */
    }
    in.move_y = 0.0f;
    tick(130);
    TEST_ASSERT_EQUAL_INT(0, (int)query("drone"));
    TEST_ASSERT_FLOAT_WITHIN(0.01f, P03_EYE, (float)query("player.y"));
    press(CADV_ACT_JUMP);
    TEST_ASSERT_EQUAL_INT(0, (int)query("drone")); /* cooling down */
    tick(20 * 60);
    press(CADV_ACT_JUMP);
    TEST_ASSERT_TRUE(query("drone") > 0);
}

static void test_portal_03_frostfront__recon_drone_view(void)
{
    start(0);
    calm();
    press(CADV_ACT_JUMP);
    tick(40);
    TEST_ASSERT_FLOAT_WITHIN(0.5f, 30.0f + P03_EYE, (float)query("player.y"));
    tick(150);
    TEST_ASSERT_EQUAL_INT(0, st->drone);
    TEST_ASSERT_TRUE(st->drone_cd > 0);
}

static void test_walking_collides_with_buildings(void)
{
    start(0);
    calm();
    stand(31, 58);
    in.move_y = 1.0f; /* north into the yard */
    tick(200);
    TEST_ASSERT_TRUE(query("player.z") > 57.0 * P03_CELL - 1.0);
    in.move_y = 0.0f;
    in.move_x = 1.0f;
    in.held = CADV_ACT_BIT(CADV_ACT_RUN);
    tick(60);
    TEST_ASSERT_TRUE(query("player.x") > 31.5 * P03_CELL + 5.0);
}

static void test_crystals_burn_infantry(void)
{
    int r;
    start(0);
    calm();
    r = spawn(P03_ACCORD, P03_U_RIFLE, 45, 52);
    tick(130);
    TEST_ASSERT_TRUE(st->rts.u[r].hp < 60);
}

static void test_friend_marks_lab_bridge_buggy(void)
{
    int eng;
    start(1);
    calm();
    TEST_ASSERT_EQUAL_INT(0, (int)query("marks"));
    spawn(P03_ACCORD, P03_U_RIFLE, 31, 34);
    tick(1);
    TEST_ASSERT_EQUAL_INT(2, (int)query("marks"));
    spawn(P03_ACCORD, P03_U_BUGGY, 31, 50);
    tick(1);
    TEST_ASSERT_EQUAL_INT(6, (int)query("marks"));
    TEST_ASSERT_NOT_NULL(strstr(st->quip, "X-RAY JUNKIE"));
    /* capturing the lab: the engineer moves in, the lab gives a tower and a scan */
    eng = spawn(P03_ACCORD, P03_U_ENGINEER, 43, 24);
    tick(1);
    TEST_ASSERT_TRUE(st->lab_captured);
    TEST_ASSERT_FALSE(st->rts.u[eng].alive);
    TEST_ASSERT_EQUAL_INT(P03_ACCORD, st->rts.b[st->lab].owner);
    TEST_ASSERT_EQUAL_INT(1, (int)query("lab"));
    TEST_ASSERT_EQUAL_INT(7, (int)query("marks"));
    TEST_ASSERT_EQUAL_INT(1, cadv_rts_count_buildings(&st->rts, P03_ACCORD, P03_B_TOWER));
    TEST_ASSERT_TRUE(st->rts.explored[P03_ACCORD][7 * P03_W + 31]);
}

static void test_portal_03_frostfront__spire_falls_and_the_star_comes_home(void)
{
    int eng;
    int i;
    cadv_portal_result r;
    start(1);
    stand(31, 18);
    cadv_rts_damage(&st->rts, CADV_RTS_TARGET_BUILDING + st->spire, 100000);
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("star.state"));
    TEST_ASSERT_EQUAL_INT(0, cadv_rts_count_buildings(&st->rts, P03_ORDER, P03_B_PYLON)); /* pylons go dark */
    /* an engineer reaches the vault, then dies on the way back: the star drops */
    eng = cadv_rts_add_unit(&st->rts, P03_ACCORD, P03_U_ENGINEER, st->star_x + 1.0f, st->star_z + 3.0f);
    tick(1);
    TEST_ASSERT_EQUAL_INT(2, (int)query("star.state"));
    cadv_rts_damage(&st->rts, eng, 1000);
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("star.state"));
    TEST_ASSERT_EQUAL_STRING("THE STAR FELL - SEND ANOTHER ENGINEER", st->message);
    /* a second one carries it to the commander, who waits nearby */
    eng = cadv_rts_add_unit(&st->rts, P03_ACCORD, P03_U_ENGINEER, st->star_x, st->star_z + 2.0f);
    for (i = 0; i < 3000 && st->star_state != 3; ++i) {
        tick(1);
    }
    TEST_ASSERT_EQUAL_INT(3, st->star_state);
    tick(400);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.star_collected);
}

static void test_portal_catalog__portal_03_win_condition(void)
{
    cadv_portal_result r;
    start(1);
    stand(31, 14);
    cadv_rts_damage(&st->rts, CADV_RTS_TARGET_BUILDING + st->spire, 100000);
    tick(1);
    cadv_rts_add_unit(&st->rts, P03_ACCORD, P03_U_ENGINEER, st->rts.u[st->commander].x, st->rts.u[st->commander].z + 1.0f);
    st->star_x = st->rts.u[st->commander].x;
    st->star_z = st->rts.u[st->commander].z + 1.0f;
    tick(3);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_RUNNING, cadv_session_result(&s).outcome); /* banner first */
    tick(150);
    in.pressed = CADV_ACT_BIT(CADV_ACT_USE);
    tick(1);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, r.outcome);
    TEST_ASSERT_TRUE(r.snowflakes >= 1);
}

static void test_portal_03_frostfront__harvester_never_lost(void)
{
    cadv_portal_result r;
    int h;
    start(1);
    calm();
    st->ticks = 16 * 3600;
    st->star_state = 3;
    st->win_timer = 1;
    tick(1);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(2, r.snowflakes); /* win + harvester kept, too slow for the third */
    cadv_session_end(&s);
    start(1);
    calm();
    h = own_unit(P03_U_HARVESTER);
    cadv_rts_damage(&st->rts, h, 10000);
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("harvester_lost"));
    TEST_ASSERT_EQUAL_STRING("HARVESTER LOST", st->message);
    st->star_state = 3;
    st->win_timer = 1;
    tick(1);
    r = cadv_session_result(&s);
    TEST_ASSERT_EQUAL_INT(2, r.snowflakes); /* win + under 15 minutes */
}

static void test_portal_03_frostfront__resume_with_the_base_standing(void)
{
    start(1);
    TEST_ASSERT_EQUAL_INT(6, (int)query("buildings"));
    TEST_ASSERT_EQUAL_INT(1, (int)query("harvesters"));
    TEST_ASSERT_EQUAL_INT(3000, (int)query("credits"));
    TEST_ASSERT_EQUAL_INT(1, cadv_session_result(&s).checkpoint);
    cadv_session_end(&s);
    start(0);
    cadv_rts_add_building(&st->rts, P03_ACCORD, P03_B_BARRACKS, 26, 51);
    cadv_rts_add_building(&st->rts, P03_ACCORD, P03_B_FACTORY, 34, 50);
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, st->checkpoint);
}

static void test_commander_down_and_defeat(void)
{
    int i;
    start(1);
    calm();
    cadv_rts_damage(&st->rts, st->commander, 100000);
    tick(1);
    TEST_ASSERT_EQUAL_INT(-1, st->commander);
    TEST_ASSERT_EQUAL_INT(0, (int)query("player.health"));
    tick(1);
    tick(300);
    TEST_ASSERT_TRUE(st->commander >= 0);
    TEST_ASSERT_EQUAL_STRING("COMMANDER REDEPLOYED", st->message);
    /* without the yard the commander's fall ends the operation */
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        if (st->rts.b[i].alive && st->rts.b[i].def == P03_B_YARD) {
            cadv_rts_destroy_building(&st->rts, i);
        }
    }
    cadv_rts_damage(&st->rts, st->commander, 100000);
    tick(1);
    TEST_ASSERT_TRUE(st->lose_timer > 0);
    tick(200);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
    cadv_session_end(&s);
    /* every structure gone: lost as well */
    start(0);
    calm();
    for (i = 0; i < CADV_RTS_MAX_BUILDINGS; ++i) {
        if (st->rts.b[i].alive && st->rts.b[i].owner == P03_ACCORD) {
            cadv_rts_destroy_building(&st->rts, i);
        }
    }
    tick(1);
    TEST_ASSERT_EQUAL_STRING("THE BASE HAS FALLEN", st->message);
    tick(200);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
}

static void test_waves_events_and_supply(void)
{
    int credits;
    start(1);
    st->ai.wave_timer = st->ai.wave_every;
    tick(1);
    TEST_ASSERT_EQUAL_INT(1, (int)query("waves"));
    TEST_ASSERT_EQUAL_STRING("ENEMY FORCES APPROACHING FROM THE NORTH", st->message);
    st->rts.events[P03_ACCORD] = 0;
    credits = st->rts.side[P03_ACCORD].credits;
    while (st->ticks % 60 != 59) {
        tick(1);
    }
    st->rts.side[P03_ACCORD].credits = credits = 0;
    tick(1);
    TEST_ASSERT_TRUE(st->rts.side[P03_ACCORD].credits >= 3); /* supply line */
    /* low power and lost structures are announced */
    cadv_rts_destroy_building(&st->rts, cadv_rts_building_at(&st->rts, 26, 54));
    cadv_rts_destroy_building(&st->rts, cadv_rts_building_at(&st->rts, 26, 57));
    st->rts.events[P03_ACCORD] = CADV_RTS_EV_LOW_POWER | CADV_RTS_EV_UNLOAD;
    tick(1);
}

static void test_aim_and_quit(void)
{
    float x;
    float z;
    double v;
    start(0);
    stand(31, 58);
    st->pitch = -0.3f;
    st->yaw = 0.0f;
    TEST_ASSERT_TRUE(cadv_p03_aim_ground(st, &x, &z));
    st->pitch = -0.01f;
    TEST_ASSERT_FALSE(cadv_p03_aim_ground(st, &x, &z)); /* beyond 160 m */
    st->pitch = -0.2f;
    st->yaw = 3.14159f;
    stand(31, 62);
    TEST_ASSERT_FALSE(cadv_p03_aim_ground(st, &x, &z)); /* off the map */
    st->pitch = 1.0f;
    TEST_ASSERT_EQUAL_INT(-1, cadv_p03_aim_target(st, false));
    TEST_ASSERT_FALSE(cadv_p03_query(st, "nope", &v));
    query("player.x");
    query("player.yaw");
    query("power");
    query("power.used");
    query("units");
    query("enemy.units");
    query("tab");
    query("harvested");
    query("outcome");
    query("ticks");
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1);
}

static void run_bot(cadv_difficulty diff)
{
    int i;
    start_with(0, diff);
    for (i = 0; i < 60000 && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_p03_bot(st, &in);
        cadv_session_tick(&s, &in);
    }
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    cadv_session_end(&s);
}

static void test_bot_finishes_on_every_difficulty(void)
{
    run_bot(CADV_DIFF_STORY);
    run_bot(CADV_DIFF_DEFAULT);
    run_bot(CADV_DIFF_HARD);
    start(0);
    cadv_p03_bot(st, &in); /* idle frame while alive */
    st->commander = -1;
    cadv_p03_bot(st, &in);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__portal_03_frostfront);
    RUN_TEST(test_tablet_build_and_place);
    RUN_TEST(test_selection_and_orders);
    RUN_TEST(test_sidearm_and_drone);
    RUN_TEST(test_portal_03_frostfront__recon_drone_view);
    RUN_TEST(test_walking_collides_with_buildings);
    RUN_TEST(test_crystals_burn_infantry);
    RUN_TEST(test_friend_marks_lab_bridge_buggy);
    RUN_TEST(test_portal_03_frostfront__spire_falls_and_the_star_comes_home);
    RUN_TEST(test_portal_catalog__portal_03_win_condition);
    RUN_TEST(test_portal_03_frostfront__harvester_never_lost);
    RUN_TEST(test_portal_03_frostfront__resume_with_the_base_standing);
    RUN_TEST(test_commander_down_and_defeat);
    RUN_TEST(test_waves_events_and_supply);
    RUN_TEST(test_aim_and_quit);
    RUN_TEST(test_bot_finishes_on_every_difficulty);
    return UNITY_END();
}

/* SPDX-License-Identifier: MIT */
/* Day 7 - Tinsel Strike: buying, shooting, the bomb, smoke and flash,
 * rounds and halves, the celebration, the bots and their balance. */
#include <string.h>

#include "portal/portal.h"
#include "portals/p07_internal.h"
#include "unity.h"

static cadv_session s;
static cadv_portal_cfg cfg;
static p07_state *st;
static cadv_input_frame in;

static void start_with(int checkpoint, cadv_difficulty diff, uint64_t seed)
{
    memset(&cfg, 0, sizeof cfg);
    cfg.day = 7;
    cfg.seed = seed;
    cfg.difficulty = diff;
    cfg.checkpoint = checkpoint;
    TEST_ASSERT_TRUE(cadv_session_start(&s, cadv_portal_get(7), &cfg));
    st = (p07_state *)s.state;
}

static void start(void) { start_with(0, CADV_DIFF_DEFAULT, 7); }

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

static void hold(cadv_action a, int n)
{
    in.held |= CADV_ACT_BIT(a);
    tick(n);
    in.held &= ~CADV_ACT_BIT(a);
}

static double query(const char *key)
{
    double v = -1.0;
    TEST_ASSERT_TRUE(cadv_p07_query(st, key, &v));
    return v;
}

static float cc(int c) { return ((float)c + 0.5f) * P07_CELL; }

/* Skip the buy time and freeze every bot where it stands. */
static void go_live(void)
{
    st->timer = 1;
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(P07_PH_LIVE, query("phase"));
}

/* Take every bot out of the round (they stay "alive" for the round's rules). */
static void park_bots(void)
{
    int i;
    for (i = 1; i < P07_AGENTS; ++i) {
        st->a[i].body.pos = cadv_v3(cc(2 + i), 0.0f, cc(1));
    }
    st->bench = true;
}

static void face(int agent, cadv_vec3 p)
{
    cadv_vec3 e = cadv_p07_eye(st, agent);
    float d = cadv_sqrtf((p.x - e.x) * (p.x - e.x) + (p.z - e.z) * (p.z - e.z));
    st->a[agent].body.yaw = cadv_atan2f(-(p.x - e.x), -(p.z - e.z));
    st->a[agent].body.pitch = cadv_atan2f(p.y - e.y, d);
}

static void test_portal_catalog__portal_07_tinsel_strike(void)
{
    const cadv_portal_desc *d = cadv_portal_get(7);
    int site;
    TEST_ASSERT_EQUAL_STRING("Tinsel Strike", d->title);
    TEST_ASSERT_TRUE(d->implemented);
    TEST_ASSERT_TRUE(sizeof(p07_state) <= CADV_P07_STATE_SIZE);
    start();
    TEST_ASSERT_EQUAL_DOUBLE(1, query("round"));
    TEST_ASSERT_EQUAL_DOUBLE(P07_PH_FREEZE, query("phase"));
    TEST_ASSERT_EQUAL_DOUBLE(P07_HUMBUG, query("side"));
    TEST_ASSERT_EQUAL_DOUBLE(800, query("money"));
    TEST_ASSERT_EQUAL_DOUBLE(5, query("alive.us"));
    TEST_ASSERT_EQUAL_DOUBLE(5, query("alive.them"));
    TEST_ASSERT_TRUE(st->carrier >= 0 && st->carrier < P07_TEAM); /* an attacker has the bomb */
    /* the rosters carry the friends */
    TEST_ASSERT_EQUAL_STRING("YOU", st->names[0]);
    TEST_ASSERT_EQUAL_STRING("Bloodmaker", st->names[1]);
    TEST_ASSERT_EQUAL_STRING("Messerjocke", st->names[6]);
    TEST_ASSERT_EQUAL_STRING("BAUBLE", st->names[3]);
    /* the map: sites, streets, the square */
    TEST_ASSERT_TRUE(cadv_p07_in_site(st, cadv_v3(cc(33), 0, cc(8)), &site));
    TEST_ASSERT_EQUAL_INT(0, site);
    TEST_ASSERT_TRUE(cadv_p07_in_site(st, cadv_v3(cc(6), 0, cc(8)), &site));
    TEST_ASSERT_EQUAL_INT(1, site);
    TEST_ASSERT_FALSE(cadv_p07_in_site(st, cadv_v3(cc(20), 0, cc(28)), NULL));
    TEST_ASSERT_EQUAL_STRING("THE SQUARE", cadv_p07_area(st, cadv_v3(cc(17), 0, cc(14))));
    TEST_ASSERT_EQUAL_STRING("A LONG", cadv_p07_area(st, cadv_v3(cc(35), 0, cc(20))));
    TEST_ASSERT_EQUAL_STRING("THE MARKET", cadv_p07_area(st, cadv_v3(0, 0, 0)));
    TEST_ASSERT_TRUE(cadv_nav_walkable(&st->nav, 20, 28));
    TEST_ASSERT_FALSE(cadv_nav_walkable(&st->nav, 0, 0));
    TEST_ASSERT_EQUAL_STRING("STOLLEN RIFLE", cadv_p07_buy_name(P07_BUY_RIFLE));
    TEST_ASSERT_EQUAL_STRING("?", cadv_p07_buy_name(-1));
    TEST_ASSERT_EQUAL_INT(0, cadv_p07_buy_price(99));
}

static void test_portal_07_tinsel_strike__buy_menu_and_freeze(void)
{
    float z;
    start();
    st->a[0].money = 5000;
    z = st->a[0].body.pos.z;
    in.move_y = 1.0f;
    tick(30);
    in.move_y = 0.0f;
    TEST_ASSERT_EQUAL_FLOAT(z, st->a[0].body.pos.z); /* frozen during buy time */
    press(CADV_ACT_WEAPON_6);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("buy.open"));
    press(CADV_ACT_NEXT_WEAPON);
    press(CADV_ACT_NEXT_WEAPON);
    press(CADV_ACT_NEXT_WEAPON); /* the rifle */
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_DOUBLE(P07_RIFLE, query("primary"));
    TEST_ASSERT_EQUAL_DOUBLE(2300, query("money"));
    press(CADV_ACT_FIRE); /* already have it */
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CANNOT BUY"));
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_PREV_WEAPON); /* wraps to the smoke */
    TEST_ASSERT_EQUAL_INT(P07_BUY_SMOKE, st->buy_cursor);
    press(CADV_ACT_USE);
    press(CADV_ACT_USE); /* one smoke only */
    TEST_ASSERT_EQUAL_INT(1, st->a[0].smokes);
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_FIRE);
    press(CADV_ACT_FIRE);
    press(CADV_ACT_FIRE); /* two flashes at most */
    TEST_ASSERT_EQUAL_INT(2, st->a[0].flashes);
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_FIRE); /* the kit is for the Guard */
    TEST_ASSERT_FALSE(st->a[0].kit);
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_DOUBLE(100, query("armour"));
    press(CADV_ACT_FIRE); /* full already */
    st->buy_cursor = P07_BUY_DEAGLE;
    st->a[0].money = 2000;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(P07_DEAGLE, st->a[0].pistol);
    press(CADV_ACT_FIRE);
    st->a[0].money = 0;
    st->buy_cursor = P07_BUY_SNIPER;
    press(CADV_ACT_FIRE); /* broke */
    TEST_ASSERT_EQUAL_DOUBLE(P07_RIFLE, query("primary"));
    press(CADV_ACT_PAUSE); /* Esc closes the menu, it does not quit */
    TEST_ASSERT_EQUAL_DOUBLE(0, query("buy.open"));
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_RUNNING, cadv_session_result(&s).outcome);
    press(CADV_ACT_WEAPON_6);
    press(CADV_ACT_WEAPON_6);
    /* buy time ends */
    tick(P07_FREEZE);
    TEST_ASSERT_EQUAL_DOUBLE(P07_PH_LIVE, query("phase"));
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "GO"));
}

static void test_fps_kit__recoil_and_spray_in_play(void)
{
    float p0;
    float p1;
    start();
    go_live();
    park_bots();
    st->a[0].primary = P07_RIFLE;
    st->a[0].mag[2] = 30;
    st->a[0].reserve[2] = 0;
    st->a[0].slot = P07_SLOT_PRIMARY;
    st->a[0].body.pitch = 0.0f;
    p0 = st->a[0].body.pitch;
    hold(CADV_ACT_FIRE, 120);
    p1 = st->a[0].body.pitch;
    TEST_ASSERT_TRUE(p1 > p0 + 0.02f); /* the view climbs with the spray */
    TEST_ASSERT_TRUE(st->a[0].spray > 5);
    tick(40);
    hold(CADV_ACT_FIRE, 1);
    TEST_ASSERT_EQUAL_INT(1, st->a[0].spray); /* a pause resets the pattern */
    /* empty: reload from the reserve, then dry */
    st->a[0].mag[2] = 1;
    st->a[0].reserve[2] = 30;
    tick(30);
    hold(CADV_ACT_FIRE, 2);
    TEST_ASSERT_EQUAL_INT(30, st->a[0].mag[2]);
    TEST_ASSERT_TRUE(st->a[0].shot_cd > 1.5f);
    /* switching weapons */
    press(CADV_ACT_WEAPON_3);
    TEST_ASSERT_EQUAL_DOUBLE(P07_SLOT_KNIFE, query("slot"));
    press(CADV_ACT_WEAPON_2);
    TEST_ASSERT_EQUAL_DOUBLE(P07_SLOT_PISTOL, query("slot"));
    press(CADV_ACT_NEXT_WEAPON);
    TEST_ASSERT_EQUAL_DOUBLE(P07_SLOT_PRIMARY, query("slot"));
    press(CADV_ACT_PREV_WEAPON);
    press(CADV_ACT_WEAPON_1);
    TEST_ASSERT_EQUAL_DOUBLE(P07_SLOT_PRIMARY, query("slot"));
    press(CADV_ACT_WEAPON_4); /* no flash: no change */
    TEST_ASSERT_EQUAL_DOUBLE(P07_SLOT_PRIMARY, query("slot"));
}

static void test_tagging_pays_and_headshots_hurt_more(void)
{
    int v = 5;
    int money;
    start();
    go_live();
    park_bots();
    st->a[v].body.pos = cadv_v3(cc(20), 0, cc(24));
    st->a[0].body.pos = cadv_v3(cc(20), 0, cc(27));
    st->a[0].slot = P07_SLOT_PISTOL;
    st->a[v].body.armor = 0;
    {
        cadv_vec3 head = cadv_p07_eye(st, v);
        face(0, head);
    }
    money = st->a[0].money;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_TRUE(st->a[v].body.health < 30 || !st->a[v].alive); /* 26 x 3 to the head */
    tick(12);
    press(CADV_ACT_FIRE);
    tick(12);
    press(CADV_ACT_FIRE);
    TEST_ASSERT_FALSE(st->a[v].alive);
    TEST_ASSERT_EQUAL_INT(money + 300, st->a[0].money);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("kills"));
    TEST_ASSERT_NOT_NULL(strstr(st->feed[0], "YOU"));
    TEST_ASSERT_TRUE(st->confetti_t > 0);
    /* a knife tag pays far more */
    v = 6;
    st->a[v].body.pos = cadv_v3(cc(20), 0, cc(27) - 1.0f);
    st->a[v].body.health = 10;
    face(0, cadv_v3(cc(20), 1.0f, cc(27) - 1.0f));
    press(CADV_ACT_WEAPON_3);
    tick(20);
    money = st->a[0].money;
    hold(CADV_ACT_FIRE, 2);
    TEST_ASSERT_FALSE(st->a[v].alive);
    TEST_ASSERT_EQUAL_INT(money + 1500, st->a[0].money);
}

static void test_portal_07_tinsel_strike__plant_and_defuse(void)
{
    int c;
    int k;
    start();
    go_live();
    park_bots();
    st->carrier = 0;
    /* outside a site nothing happens */
    hold(CADV_ACT_USE, P07_PLANT + 5);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("planted"));
    cadv_p07_teleport(st, cc(33), 0.0f, cc(8), 0.0f);
    tick(5);
    hold(CADV_ACT_USE, P07_PLANT / 2);
    TEST_ASSERT_TRUE(st->a[0].plant_t > 0.0f);
    tick(1); /* letting go cancels */
    TEST_ASSERT_EQUAL_FLOAT(0.0f, st->a[0].plant_t);
    hold(CADV_ACT_USE, P07_PLANT + 2);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("planted"));
    TEST_ASSERT_EQUAL_INT(0, st->bomb_site);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "PLANTED AT A"));
    /* a Guard defuses: ten seconds, five with a kit; the rest stand aside */
    c = 7;
    for (k = 1; k < P07_AGENTS; ++k) {
        st->a[k].alive = k == c;
    }
    st->a[c].body.pos = st->bomb_pos;
    st->a[c].kit = true;
    st->bench = false;
    st->a[0].body.pos = cadv_v3(cc(20), 0, cc(29));
    tick(P07_DEFUSE_KIT - 30);
    TEST_ASSERT_TRUE(st->a[c].defuse_t > 3.0f);
    TEST_ASSERT_EQUAL_INT(P07_PH_LIVE, st->phase);
    tick(60);
    TEST_ASSERT_EQUAL_INT(P07_PH_AFTER, st->phase);
    TEST_ASSERT_EQUAL_INT(CADV_WIN_DEFUSE, st->win_reason);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("score.them"));
    /* the losers who planted still get the bonus */
    TEST_ASSERT_TRUE(st->a[0].money >= 1400 + 800);
}

static void test_bomb_explodes_and_drops(void)
{
    start();
    go_live();
    park_bots();
    /* the carrier falls: the bomb drops, an attacker picks it up */
    st->carrier = 1;
    st->a[1].blind = 0.0f;
    st->a[1].body.pos = cadv_v3(cc(20), 0, cc(28));
    st->a[1].body.health = 1;
    {
        int v = 1;
        st->a[5].body.pos = cadv_v3(cc(20), 0, cc(24));
        st->a[5].blind = 0.0f;
        cadv_player_damage(&st->a[v].body, 50);
        st->a[v].alive = false;
        st->carrier = -1;
        st->bomb_dropped = true;
        st->bomb_pos = st->a[v].body.pos;
        st->a[5].blind = 1000.0f;
    }
    cadv_p07_teleport(st, st->bomb_pos.x, 0.0f, st->bomb_pos.z, 0.0f);
    tick(2);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("carrier"));
    TEST_ASSERT_NOT_NULL(strstr(st->message, "GLITTER BOMB"));
    /* planted at B and left to tick down */
    cadv_p07_teleport(st, cc(6), 0.0f, cc(9), 0.0f);
    tick(3);
    hold(CADV_ACT_USE, P07_PLANT + 2);
    TEST_ASSERT_EQUAL_INT(1, st->bomb_site);
    cadv_p07_teleport(st, cc(20), 0.0f, cc(29), 0.0f);
    st->bomb_timer = 2;
    tick(3);
    TEST_ASSERT_EQUAL_INT(CADV_WIN_BOMB, st->win_reason);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("score.us"));
}

static void test_fps_kit__smoke_and_flash_in_play(void)
{
    int k;
    start();
    go_live();
    park_bots();
    st->a[5].blind = 0.0f;
    st->a[5].body.pos = cadv_v3(cc(17), 0, cc(13));
    st->a[0].body.pos = cadv_v3(cc(17), 0, cc(19));
    TEST_ASSERT_TRUE(cadv_p07_can_see(st, 0, 5));
    st->smokes[0].pos = cadv_v3(cc(17), 1.6f, cc(16));
    st->smokes[0].ticks = 600;
    TEST_ASSERT_FALSE(cadv_p07_can_see(st, 0, 5)); /* the smoke hides them */
    TEST_ASSERT_EQUAL_DOUBLE(1, query("smokes"));
    st->smokes[0].ticks = 0;
    /* throw a smoke and a flash */
    st->a[0].smokes = 1;
    st->a[0].flashes = 1;
    press(CADV_ACT_WEAPON_5);
    TEST_ASSERT_EQUAL_DOUBLE(P07_SLOT_SMOKE, query("slot"));
    st->a[0].body.pitch = 0.2f;
    press(CADV_ACT_FIRE);
    TEST_ASSERT_EQUAL_INT(0, st->a[0].smokes);
    tick(140);
    for (k = 0; k < P07_SMOKES && st->smokes[k].ticks <= 0; ++k) {
    }
    TEST_ASSERT_TRUE(k < P07_SMOKES);
    memset(st->smokes, 0, sizeof st->smokes); /* clear the air for the flash */
    tick(30);
    press(CADV_ACT_WEAPON_4);
    st->a[0].body.yaw = 0.0f;
    st->a[0].body.pitch = 0.1f;
    tick(30);
    press(CADV_ACT_FIRE);
    tick(100);
    TEST_ASSERT_TRUE(query("blind") > 0.1 || st->a[5].blind > 0.1f);
}

static void test_footsteps_are_heard_unless_walking(void)
{
    start();
    go_live();
    park_bots();
    st->a[5].blind = 0.0f;
    st->a[5].heard_t = 0.0f;
    st->a[5].body.pos = cadv_v3(cc(20), 0, cc(20));
    st->a[0].body.pos = cadv_v3(cc(18), 0, cc(26));
    in.held = CADV_ACT_BIT(CADV_ACT_RUN); /* walking quietly */
    in.move_y = 1.0f;
    tick(90);
    TEST_ASSERT_TRUE(st->a[5].heard_t <= 0.0f);
    in.held = 0;
    tick(90);
    in.move_y = 0.0f;
    TEST_ASSERT_TRUE(st->a[5].heard_t > 0.0f);
}

static void test_portal_07_tinsel_strike__rounds_halves_and_the_star(void)
{
    int i;
    start();
    go_live();
    /* time runs out with no plant: the Guard wins */
    st->timer = 1;
    park_bots();
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_WIN_TIME, st->win_reason);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("score.them"));
    tick(P07_AFTER + 1);
    TEST_ASSERT_EQUAL_DOUBLE(2, query("round"));
    TEST_ASSERT_EQUAL_DOUBLE(800 + 1400, query("money"));
    /* elimination: tag the whole other team */
    go_live();
    for (i = P07_TEAM; i < P07_AGENTS; ++i) {
        st->a[i].alive = false;
    }
    tick(1);
    TEST_ASSERT_EQUAL_INT(CADV_WIN_ELIMINATION, st->win_reason);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("score.us"));
    /* on to the half: sides switch, money resets, a checkpoint is kept */
    st->score[0] = 2;
    st->score[1] = 1;
    st->round = P07_HALF - 1;
    tick(P07_AFTER + 1);
    TEST_ASSERT_EQUAL_DOUBLE(P07_HALF, query("round"));
    go_live();
    for (i = 0; i < P07_TEAM; ++i) {
        st->a[i].alive = i == 0;
    }
    st->clutch_chance = true; /* 1 against 5 */
    for (i = P07_TEAM; i < P07_AGENTS; ++i) {
        st->a[i].alive = false;
    }
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("clutch"));
    TEST_ASSERT_EQUAL_INT(1 + 3 * 16 + 1, cadv_session_result(&s).checkpoint);
    tick(P07_AFTER + 1);
    TEST_ASSERT_EQUAL_DOUBLE(P07_HALF + 1, query("round"));
    TEST_ASSERT_EQUAL_DOUBLE(P07_GUARD, query("side"));
    TEST_ASSERT_EQUAL_DOUBLE(800, query("money"));
    TEST_ASSERT_NOT_NULL(strstr(st->radio, "HALF TIME"));
    /* the winning round */
    st->score[0] = 4;
    go_live();
    for (i = P07_TEAM; i < P07_AGENTS; ++i) {
        st->a[i].alive = false;
    }
    tick(1);
    tick(P07_AFTER + 1);
    TEST_ASSERT_EQUAL_DOUBLE(P07_PH_CELEBRATE, query("phase"));
    TEST_ASSERT_TRUE(st->steps);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "CLIMB THE TREE"));
    cadv_p07_teleport(st, st->star_pos.x, st->star_pos.y - 0.9f, st->star_pos.z, 0.0f);
    tick(1);
    TEST_ASSERT_EQUAL_DOUBLE(1, query("star.taken"));
    tick(200);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    TEST_ASSERT_EQUAL_INT(3, cadv_session_result(&s).snowflakes); /* won both halves and a clutch */
}

static void test_losing_the_match(void)
{
    int i;
    start_with(1 + 2 * 16 + 4, CADV_DIFF_DEFAULT, 7); /* resumed at half time, 2:4 */
    TEST_ASSERT_EQUAL_DOUBLE(P07_HALF + 1, query("round"));
    TEST_ASSERT_EQUAL_DOUBLE(4, query("score.them"));
    go_live();
    for (i = 0; i < P07_TEAM; ++i) {
        st->a[i].alive = false;
    }
    tick(1);
    TEST_ASSERT_TRUE(st->spectate == 0 || !st->a[st->spectate].alive);
    tick(P07_AFTER + 2);
    TEST_ASSERT_NOT_NULL(strstr(st->message, "MATCH LOST"));
    tick(260);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_LOST, cadv_session_result(&s).outcome);
}

static void test_spectating_and_marks(void)
{
    start();
    go_live();
    tick(1); /* a friend at the spawn */
    /* the spray tag in the square */
    cadv_p07_teleport(st, 33.0f, 0.0f, 24.0f, 0.0f);
    tick(1);
    TEST_ASSERT_TRUE(st->marks_found & 1u);
    TEST_ASSERT_TRUE(st->marks_found & 2u); /* Bloodmaker stood beside us at the spawn */
    /* tagged: the camera follows a teammate, E cycles */
    st->a[0].body.health = 1;
    {
        int k = 5;
        st->a[k].body.pos = cadv_v3(33.0f, 0.0f, 20.5f);
        cadv_player_damage(&st->a[0].body, 50);
    }
    st->a[0].alive = false;
    st->spectate = 1;
    press(CADV_ACT_USE);
    TEST_ASSERT_TRUE(st->spectate > 1 && st->spectate < P07_TEAM);
    TEST_ASSERT_EQUAL_DOUBLE(0, query("player.health"));
}

static void test_queries_teleport_and_quit(void)
{
    const char *keys[] = {"player.x", "player.y", "player.z", "player.yaw", "player.health", "armour", "money", "round", "phase",
                          "score.us", "score.them", "side", "alive.us", "alive.them", "planted", "carrier", "kills", "primary",
                          "slot", "buy.open", "smokes", "blind", "clutch", "star.taken", "outcome", "marks", "ticks"};
    size_t i;
    double v;
    start();
    for (i = 0; i < sizeof keys / sizeof keys[0]; ++i) {
        TEST_ASSERT_TRUE(cadv_p07_query(st, keys[i], &v));
    }
    TEST_ASSERT_FALSE(cadv_p07_query(st, "bogus", &v));
    cadv_p07_teleport(st, 30, 0, 40, 1.0f);
    TEST_ASSERT_EQUAL_DOUBLE(30, query("player.x"));
    press(CADV_ACT_PAUSE);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_QUIT, cadv_session_result(&s).outcome);
    tick(1);
}

static void play_bot(int max)
{
    int i;
    for (i = 0; i < max && cadv_session_result(&s).outcome == CADV_OUTCOME_RUNNING; ++i) {
        cadv_input_frame bot;
        cadv_p07_bot(st, &bot);
        cadv_session_tick(&s, &bot);
    }
}

static void test_bot_wins_on_every_difficulty(void)
{
    start_with(0, CADV_DIFF_DEFAULT, 7);
    play_bot(120000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    TEST_ASSERT_TRUE(cadv_session_result(&s).star_collected);
    cadv_session_end(&s);
    start_with(0, CADV_DIFF_HARD, 3);
    play_bot(120000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
    cadv_session_end(&s);
    start_with(1 + 2 * 16 + 2, CADV_DIFF_STORY, 5);
    play_bot(120000);
    TEST_ASSERT_EQUAL_INT(CADV_OUTCOME_WON, cadv_session_result(&s).outcome);
}

static void test_portal_07_tinsel_strike__bot_vs_bot_balance(void)
{
    /* even bots on both sides: the defenders win 35-65 % of rounds */
    int seed;
    int side[2] = {0, 0};
    for (seed = 1; seed <= 16; ++seed) {
        start_with(0, CADV_DIFF_DEFAULT, (uint64_t)seed);
        st->even = true;
        play_bot(60 * 60 * 40);
        side[0] += st->side_wins[0];
        side[1] += st->side_wins[1];
        cadv_session_end(&s);
    }
    start(); /* for tearDown */
    TEST_ASSERT_TRUE(side[0] + side[1] > 80);
    TEST_ASSERT_TRUE(side[1] * 100 >= 35 * (side[0] + side[1]));
    TEST_ASSERT_TRUE(side[1] * 100 <= 65 * (side[0] + side[1]));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_catalog__portal_07_tinsel_strike);
    RUN_TEST(test_portal_07_tinsel_strike__buy_menu_and_freeze);
    RUN_TEST(test_fps_kit__recoil_and_spray_in_play);
    RUN_TEST(test_tagging_pays_and_headshots_hurt_more);
    RUN_TEST(test_portal_07_tinsel_strike__plant_and_defuse);
    RUN_TEST(test_bomb_explodes_and_drops);
    RUN_TEST(test_fps_kit__smoke_and_flash_in_play);
    RUN_TEST(test_footsteps_are_heard_unless_walking);
    RUN_TEST(test_portal_07_tinsel_strike__rounds_halves_and_the_star);
    RUN_TEST(test_losing_the_match);
    RUN_TEST(test_spectating_and_marks);
    RUN_TEST(test_queries_teleport_and_quit);
    RUN_TEST(test_bot_wins_on_every_difficulty);
    RUN_TEST(test_portal_07_tinsel_strike__bot_vs_bot_balance);
    return UNITY_END();
}

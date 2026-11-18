/* SPDX-License-Identifier: MIT */
#include <stdio.h>
#include <string.h>

#include "kits/fps/map.h"
#include "unity.h"

static cadv_map m;
static cadv_map_links links;
static cadv_fps f;

static const char k_map[] =
    "# a tiny test level\n"
    "name Tiny Test Hall\n"
    "spawn 0 0 5 0\r\n"
    "box -10 -1 -10 10 0 10 floor\n"
    "deco -1 3 -1 1 3.1 1 lamp\n"
    "glass 4 0 0 3 2 0.2 15 window\n"
    "target 5 0 -5 6 1 -4 100\n"
    "door -1 0 -8 1 2.5 -7.8 3 metal\n"
    "enemy hog 0 0 -5 3.14\n"
    "enemy unknownbeast 1 0 -5 0\n"
    "pickup ammo 20 2 1 0.5 1\n"
    "pickup key 1 0 2 0.5 1\n"
    "prop vending 5 0 5 1.57\n"
    "secret 8 0 8 9 2 9\n"
    "trigger lift 0 0 -9 1 2 -8\n"
    "light 0 3 0 1 0.8 0.6 10 1\n"
    "sign 0 2.5 -9.9 0 3 GNOME DEPOT\n"
    "mark 2 -9.9 2 0 1.57\n"
    "waypoint 0 0 0\n"
    "waypoint 0 0 -6 use\n";

static int enemy_type(const char *name) { return strcmp(name, "hog") == 0 ? 0 : -1; }

void setUp(void) {}
void tearDown(void) {}

static void test_parse_everything(void)
{
    TEST_ASSERT_TRUE_MESSAGE(cadv_map_parse(&m, k_map), m.error);
    TEST_ASSERT_EQUAL_STRING("Tiny Test Hall", m.name);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, m.spawn.z);
    TEST_ASSERT_EQUAL_INT(5, m.nbox);
    TEST_ASSERT_EQUAL_INT(1, m.box[1].kind);
    TEST_ASSERT_EQUAL_FLOAT(3.0f, m.box[2].lo.x); /* min/max normalised */
    TEST_ASSERT_EQUAL_INT(15, m.box[2].value);
    TEST_ASSERT_EQUAL_STRING("target", m.box[3].mat);
    TEST_ASSERT_EQUAL_INT(4, m.box[4].kind);
    TEST_ASSERT_EQUAL_INT(3, m.box[4].value);
    TEST_ASSERT_EQUAL_INT(2, m.nenemy);
    TEST_ASSERT_EQUAL_INT(2, m.npickup);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m.pickup[0].b);
    TEST_ASSERT_EQUAL_INT(1, m.nprop);
    TEST_ASSERT_EQUAL_INT(1, m.nsecret);
    TEST_ASSERT_EQUAL_INT(1, m.ntrigger);
    TEST_ASSERT_NOT_NULL(cadv_map_find_trigger(&m, "lift"));
    TEST_ASSERT_NULL(cadv_map_find_trigger(&m, "nope"));
    TEST_ASSERT_EQUAL_FLOAT(0.6f, m.light[0].hi.z);
    TEST_ASSERT_EQUAL_STRING("GNOME DEPOT", m.sign[0].text);
    TEST_ASSERT_EQUAL_FLOAT(2.0f, m.mark[0].a);
    TEST_ASSERT_EQUAL_INT(2, m.nwaypoint);
    TEST_ASSERT_EQUAL_STRING("use", m.waypoint[1].name);
}

static void test_apply_builds_world(void)
{
    double dummy = 0;
    (void)dummy;
    TEST_ASSERT_TRUE(cadv_map_parse(&m, k_map));
    cadv_fps_init(&f, 1);
    cadv_map_apply(&m, &f, enemy_type, &links);
    TEST_ASSERT_EQUAL_INT(1, f.enemy_count);
    TEST_ASSERT_EQUAL_INT(-1, links.enemy_index[1]);
    TEST_ASSERT_EQUAL_INT(2, f.pickup_count);
    TEST_ASSERT_EQUAL_INT(2, f.pickups[0].sub);
    TEST_ASSERT_EQUAL_INT(2, f.breakable_count);
    TEST_ASSERT_EQUAL_INT(1, f.door_count);
    TEST_ASSERT_EQUAL_INT(-1, links.box_solid[1]);
    TEST_ASSERT_EQUAL_INT(400, f.world.s[links.prop_solid[0]].tag);
    TEST_ASSERT_EQUAL_FLOAT(5.0f, f.player.pos.z);
    cadv_fps_init(&f, 1);
    cadv_map_apply(&m, &f, NULL, &links); /* no enemy resolver: none added */
    TEST_ASSERT_EQUAL_INT(0, f.enemy_count);
}

static void test_errors(void)
{
    char big[300];
    static const char *const bad[] = {
        "spawn 1 2", "box 1 2 3 4 5 6", "deco 1 2 3", "glass 1 2 3 4 5 6 7", "target 1 2 3 4 5 6",
        "door 1 2 3 4 5 6 7", "enemy hog 1 2", "pickup sword 1 0 1 2 3", "pickup ammo 1 0 1",
        "prop vending 1 2", "mark 1 2 3", "secret 1 2 3", "trigger lift 1 2 3", "light 1 2 3",
        "sign 1 2 3 0", "waypoint 1 2", "teleporter 1 2 3",
    };
    size_t i;
    for (i = 0; i < sizeof bad / sizeof bad[0]; ++i) {
        TEST_ASSERT_FALSE_MESSAGE(cadv_map_parse(&m, bad[i]), bad[i]);
        TEST_ASSERT_EQUAL_INT(0, strncmp(m.error, "line 1:", 7));
    }
    memset(big, 'x', sizeof big - 1);
    big[sizeof big - 1] = '\0';
    TEST_ASSERT_FALSE(cadv_map_parse(&m, big));
    TEST_ASSERT_FALSE(cadv_map_load(&m, "missing.map"));
    TEST_ASSERT_EQUAL_INT(-1, cadv_map_pickup_kind("sword"));
    TEST_ASSERT_EQUAL_INT(7, cadv_map_pickup_kind("fuel"));
}

static void test_capacity_limits(void)
{
    static char text[CADV_MAP_MAX_BOXES * 24 + 64];
    int i;
    text[0] = '\0';
    for (i = 0; i <= CADV_MAP_MAX_BOXES; ++i) {
        strcat(text, "box 0 0 0 1 1 1 a\n");
    }
    TEST_ASSERT_FALSE(cadv_map_parse(&m, text));
    TEST_ASSERT_NOT_NULL(strstr(m.error, "too many boxes"));
    text[0] = '\0';
    for (i = 0; i <= 16; ++i) {
        strcat(text, "secret 0 0 0 1 1 1\n");
    }
    TEST_ASSERT_FALSE(cadv_map_parse(&m, text));
    TEST_ASSERT_NOT_NULL(strstr(m.error, "too many entries"));
}

static void test_load_file(void)
{
    FILE *fp = fopen("test_tmp.map", "wb");
    fputs(k_map, fp);
    fclose(fp);
    TEST_ASSERT_TRUE(cadv_map_load(&m, "test_tmp.map"));
    TEST_ASSERT_EQUAL_INT(5, m.nbox);
    fp = fopen("test_tmp.map", "wb");
    fclose(fp);
    TEST_ASSERT_TRUE(cadv_map_load(&m, "test_tmp.map")); /* empty map is valid */
    remove("test_tmp.map");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_parse_everything);
    RUN_TEST(test_apply_builds_world);
    RUN_TEST(test_errors);
    RUN_TEST(test_capacity_limits);
    RUN_TEST(test_load_file);
    return UNITY_END();
}

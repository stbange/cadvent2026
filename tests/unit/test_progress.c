/* SPDX-License-Identifier: MIT */
#include <string.h>

#include "hub/friends.h"
#include "hub/progress.h"
#include "unity.h"

static cadv_progress pr;

void setUp(void) { cadv_progress_reset(&pr); }
void tearDown(void) {}

static cadv_portal_result res(cadv_outcome o, int flakes, bool star)
{
    cadv_portal_result r;
    memset(&r, 0, sizeof r);
    r.outcome = o;
    r.snowflakes = flakes;
    r.star_collected = star;
    r.ticks = 1000;
    return r;
}

static void test_portal_framework__best_result_kept(void)
{
    cadv_portal_result r = res(CADV_OUTCOME_WON, 2, true);
    TEST_ASSERT_TRUE(cadv_progress_record(&pr, 4, &r));
    r = res(CADV_OUTCOME_WON, 1, true);
    r.ticks = 2000;
    TEST_ASSERT_FALSE(cadv_progress_record(&pr, 4, &r));
    TEST_ASSERT_EQUAL_INT(2, pr.p[3].snowflakes);
    TEST_ASSERT_EQUAL_INT(1000, pr.p[3].best_ticks);
    r = res(CADV_OUTCOME_WON, 9, true); /* clamped to 3 */
    r.ticks = 500;
    TEST_ASSERT_TRUE(cadv_progress_record(&pr, 4, &r));
    TEST_ASSERT_EQUAL_INT(3, pr.p[3].snowflakes);
    TEST_ASSERT_EQUAL_INT(500, pr.p[3].best_ticks);
    TEST_ASSERT_FALSE(cadv_progress_record(&pr, 0, &r));
    TEST_ASSERT_FALSE(cadv_progress_record(&pr, 25, &r));
}

static void test_shining_stars__star_kept_after_defeat(void)
{
    cadv_portal_result r = res(CADV_OUTCOME_WON, 1, true);
    cadv_progress_record(&pr, 2, &r);
    r = res(CADV_OUTCOME_LOST, 0, false);
    cadv_progress_record(&pr, 2, &r);
    TEST_ASSERT_TRUE(pr.p[1].star_collected);
}

static void test_shining_stars__win_without_pickup(void)
{
    cadv_portal_result r = res(CADV_OUTCOME_WON, 1, false);
    cadv_progress_record(&pr, 4, &r);
    TEST_ASSERT_TRUE(pr.p[3].completed);
    TEST_ASSERT_FALSE(pr.p[3].star_collected);
    TEST_ASSERT_EQUAL_INT(0, cadv_progress_stars_carried(&pr));
}

static void test_checkpoints_and_marks(void)
{
    cadv_portal_result r = res(CADV_OUTCOME_QUIT, 0, false);
    r.checkpoint = 2;
    r.marks_found = 5;
    TEST_ASSERT_TRUE(cadv_progress_record(&pr, 1, &r));
    TEST_ASSERT_EQUAL_INT(2, pr.p[0].checkpoint);
    TEST_ASSERT_EQUAL_UINT32(5, pr.p[0].marks_found);
    r.marks_found = 4;
    r.checkpoint = 1;
    TEST_ASSERT_FALSE(cadv_progress_record(&pr, 1, &r));
    r = res(CADV_OUTCOME_WON, 1, true);
    cadv_progress_record(&pr, 1, &r);
    TEST_ASSERT_EQUAL_INT(0, pr.p[0].checkpoint); /* finished runs start fresh */
}

static void test_hub_hall__candle_lighting(void)
{
    int d;
    for (d = 1; d <= 3; ++d) {
        pr.p[d - 1].snowflakes = 3;
    }
    pr.p[3].snowflakes = 2;
    TEST_ASSERT_EQUAL_INT(11, cadv_progress_total_snowflakes(&pr));
    TEST_ASSERT_EQUAL_INT(1, cadv_progress_candles(&pr));
    pr.p[4].snowflakes = 1;
    TEST_ASSERT_EQUAL_INT(2, cadv_progress_candles(&pr));
    for (d = 1; d <= 24; ++d) {
        pr.p[d - 1].snowflakes = 3;
    }
    TEST_ASSERT_EQUAL_INT(4, cadv_progress_candles(&pr));
}

static void test_sockets_and_arch(void)
{
    int missing[23];
    pr.p[2].star_collected = true;
    pr.p[4].star_collected = true;
    pr.p[23].star_collected = true; /* day 24 star never goes into the arch */
    TEST_ASSERT_EQUAL_INT(2, cadv_progress_stars_carried(&pr));
    TEST_ASSERT_EQUAL_INT(2, cadv_progress_socket_all(&pr));
    TEST_ASSERT_EQUAL_INT(0, cadv_progress_socket_all(&pr));
    TEST_ASSERT_EQUAL_INT(2, cadv_progress_stars_socketed(&pr));
    TEST_ASSERT_EQUAL_INT(21, cadv_progress_missing_sockets(&pr, missing, 23));
    TEST_ASSERT_EQUAL_INT(1, missing[0]);
    TEST_ASSERT_EQUAL_INT(21, cadv_progress_missing_sockets(&pr, missing, 2)); /* count beyond max */
    TEST_ASSERT_FALSE(cadv_progress_arch_complete(&pr));
    TEST_ASSERT_TRUE(cadv_progress_can_crown(&pr));
    pr.tree_crowned = true;
    TEST_ASSERT_FALSE(cadv_progress_can_crown(&pr));
    cadv_progress_dev_fill(&pr);
    TEST_ASSERT_TRUE(cadv_progress_arch_complete(&pr));
}

static void test_kv_round_trip(void)
{
    static cadv_kvstore kv;
    cadv_progress back;
    pr.p[0].entered = true;
    pr.p[0].completed = true;
    pr.p[0].star_collected = true;
    pr.p[0].star_socketed = true;
    pr.p[0].snowflakes = 3;
    pr.p[5].checkpoint = 2;
    pr.p[5].best_ticks = 4242;
    pr.p[5].marks_found = 9;
    pr.tree_crowned = true;
    pr.epilogue_seen = true;
    cadv_kv_clear(&kv);
    cadv_progress_to_kv(&pr, &kv);
    cadv_progress_from_kv(&back, &kv);
    TEST_ASSERT_EQUAL_MEMORY(&pr, &back, sizeof pr);
}

static void test_kv_sanitises_values(void)
{
    static cadv_kvstore kv;
    cadv_progress back;
    cadv_kv_clear(&kv);
    cadv_kv_set(&kv, "p01.snowflakes", "99");
    cadv_kv_set(&kv, "p01.flags", "8"); /* socketed without collected is ignored */
    cadv_kv_set(&kv, "p02.checkpoint", "-5");
    cadv_progress_from_kv(&back, &kv);
    TEST_ASSERT_EQUAL_INT(3, back.p[0].snowflakes);
    TEST_ASSERT_FALSE(back.p[0].star_socketed);
    TEST_ASSERT_EQUAL_INT(0, back.p[1].checkpoint);
}

static void test_friends_defaults_load_and_marks(void)
{
    cadv_friends f;
    FILE *fp;
    char text[64];
    cadv_mark_def m = {1, "%s WAS HERE"};
    cadv_mark_def plain = {0, "NO NAME"};
    cadv_mark_def bad = {42, "BEWARE OF %s!"};
    int found;
    int total;
    cadv_friends_default(&f);
    TEST_ASSERT_EQUAL_INT(6, f.count);
    TEST_ASSERT_EQUAL_STRING("Messerjocke", f.ptr[4]);
    cadv_friends_mark_text(&f, &m, text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("BLOODMAKER WAS HERE", text);
    cadv_friends_mark_text(&f, &plain, text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("NO NAME", text);
    cadv_friends_mark_text(&f, &bad, text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("BEWARE OF SOMEONE!", text);
    TEST_ASSERT_FALSE(cadv_friends_load(&f, NULL));
    TEST_ASSERT_FALSE(cadv_friends_load(&f, "missing.cfg"));
    fp = fopen("test_friends.cfg", "wb");
    fputs("cadvent-friends 1\nfriend.0=Alice\nfriend.1=Bob\n", fp);
    fclose(fp);
    TEST_ASSERT_TRUE(cadv_friends_load(&f, "test_friends.cfg"));
    TEST_ASSERT_EQUAL_INT(2, f.count);
    TEST_ASSERT_EQUAL_STRING("Bob", f.ptr[1]);
    fp = fopen("test_friends.cfg", "wb");
    fputs("cadvent-friends 1\nother=1\n", fp);
    fclose(fp);
    TEST_ASSERT_FALSE(cadv_friends_load(&f, "test_friends.cfg"));
    TEST_ASSERT_EQUAL_INT(6, f.count);
    remove("test_friends.cfg");
    pr.p[0].marks_found = 1u; /* day 1, mark 0 = friend 0 */
    cadv_friends_tally(&pr, 0, &found, &total);
    TEST_ASSERT_EQUAL_INT(1, found);
    TEST_ASSERT_TRUE(total >= 12);
}

static void test_friends_marks__coverage_check(void)
{
    /* every portal has >= 2 marks and every friend appears in >= 12 portals */
    int friend_portals[6] = {0};
    int day;
    int f;
    for (day = 1; day <= CADV_PORTAL_COUNT; ++day) {
        const cadv_portal_desc *d = cadv_portal_get(day);
        bool seen[6] = {false};
        int i;
        TEST_ASSERT_TRUE(d->mark_count >= 2);
        for (i = 0; i < d->mark_count; ++i) {
            seen[d->marks[i].friend_index] = true;
        }
        for (f = 0; f < 6; ++f) {
            friend_portals[f] += seen[f];
        }
    }
    for (f = 0; f < 6; ++f) {
        TEST_ASSERT_TRUE(friend_portals[f] >= 12);
    }
}

static void test_friends_marks__renaming_a_friend(void)
{
    cadv_friends f;
    char text[64];
    cadv_mark_def m = {2, "MAINTENANCE BOT %s"};
    cadv_friends_default(&f);
    snprintf(f.name[2], CADV_FRIEND_NAME, "%s", "R2X");
    cadv_friends_mark_text(&f, &m, text, sizeof text);
    TEST_ASSERT_EQUAL_STRING("MAINTENANCE BOT R2X", text);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_portal_framework__best_result_kept);
    RUN_TEST(test_shining_stars__star_kept_after_defeat);
    RUN_TEST(test_shining_stars__win_without_pickup);
    RUN_TEST(test_checkpoints_and_marks);
    RUN_TEST(test_hub_hall__candle_lighting);
    RUN_TEST(test_sockets_and_arch);
    RUN_TEST(test_kv_round_trip);
    RUN_TEST(test_kv_sanitises_values);
    RUN_TEST(test_friends_defaults_load_and_marks);
    RUN_TEST(test_friends_marks__coverage_check);
    RUN_TEST(test_friends_marks__renaming_a_friend);
    return UNITY_END();
}

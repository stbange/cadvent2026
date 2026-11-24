# Day 8 is a shipped game: on 8 December it opens without any override; the
# completion bot rides the monorail, starts the test, escorts the scientists
# to the scanner and reverses the beam at the array.
# portal_08_cascade__the_story_from_the_hall, portal_catalog__portal_08_win_condition
expect hub.state.8 == 3
walk_to 7 2 900
walk_to 7 -9 900
face -1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 8
expect portal.act == 0
bot 40000
expect_event star_collected:8
expect_event portal_won:8
expect mode == 0
expect progress.star.8 == 1

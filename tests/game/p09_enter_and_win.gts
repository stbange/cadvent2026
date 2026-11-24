# Day 9 is a shipped game: on 9 December it opens without any override; the
# completion bot runs the training course (ski, jet, disc), wins the match on
# Snowglobe Basin and carries the trophy star home on the victory lap.
# portal_09_skifrost__the_match_from_the_hall, portal_catalog__portal_09_win_condition
expect hub.state.9 == 3
walk_to 7 2 900
walk_to 7 -14 900
face -1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 9
expect portal.phase == 0
expect portal.train == 0
bot 110000
expect_event star_collected:9
expect_event portal_won:9
expect mode == 0
expect progress.star.9 == 1

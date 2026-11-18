# Day 1 is a shipped game: on 1 December it opens without any override.
# portal_catalog__portal_01_win_condition, shining_stars__one_star_per_portal
expect hub.state.1 == 3
walk_to -7 2 900
walk_to -7 -4 900
face 1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 1
expect portal.keys == 0
bot 40000
expect_event portal_won:1
expect_event star_collected:1
expect mode == 0
expect progress.star.1 == 1
expect progress.completed.1 == 1
expect hub.stars_carried == 1

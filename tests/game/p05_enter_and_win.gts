# Day 5 is a shipped game: on 5 December it opens without any override and the
# completion bot holds Kindlewick through three raids, ferries its army over
# the thawed lake, burns the Longhall and carries the star home.
# portal_05_hearthguard__hold_the_raids_then_burn_the_longhall, portal_catalog__portal_05_win_condition
expect hub.state.5 == 3
walk_to -7 2 900
walk_to -7 -24 900
face 1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 5
expect portal.raid == 0
expect portal.workers == 3
bot 90000
expect_event portal_won:5
expect_event star_collected:5
expect mode == 0
expect progress.star.5 == 1
expect progress.snowflakes.5 >= 2

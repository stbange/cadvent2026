# Day 4 is a shipped game: on 4 December it opens without any override and the
# completion bot follows the explorer's route: pits, ledges, the block puzzle,
# the timed door, the flooded crypt, the ice wall and the Colossus chase.
# portal_04_ice_tomb__three_gears_open_the_solar_door, portal_catalog__portal_04_win_condition
expect hub.state.4 == 3
walk_to -7 2 900
walk_to -7 -19 900
face 1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 4
expect portal.gears == 0
bot 40000
expect_event portal_won:4
expect_event star_collected:4
expect mode == 0
expect progress.star.4 == 1
expect progress.snowflakes.4 == 3

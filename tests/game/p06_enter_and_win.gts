# Day 6 is a shipped game: on 6 December it opens without any override; the
# completion bot leaves the mech lab, escorts Sleighbell (taking the star from
# its cargo bay), destroys the dam pumps and wins the duel with Krampus.
# portal_06_iron_yule__the_campaign_from_the_hall, portal_catalog__portal_06_win_condition
expect hub.state.6 == 3
walk_to -7 2 900
walk_to -7 -29 900
face 1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 6
expect portal.lab == 1
bot 40000
expect_event star_collected:6
expect_event portal_won:6
expect mode == 0
expect progress.star.6 == 1
expect progress.snowflakes.6 >= 2

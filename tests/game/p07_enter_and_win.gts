# Day 7 is a shipped game: on 7 December it opens without any override; the
# completion bot buys, fights through the rounds with its team, wins the
# match and climbs the giant tree to the Shining Star.
# portal_07_tinsel_strike__the_match_from_the_hall, portal_catalog__portal_07_win_condition
expect hub.state.7 == 3
walk_to 7 2 900
walk_to 7 -4 900
face -1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 7
expect portal.round == 1
expect portal.money == 800
bot 70000
expect_event star_collected:7
expect_event portal_won:7
expect mode == 0
expect progress.star.7 == 1

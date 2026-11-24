# Day 10 is a shipped game: on 10 December it opens without any override; the
# completion bot cuts the three relays (market, factory, tower), takes the star
# from the antenna and holds the plaza until the VTOL lands.
# portal_10_neon_syndic__the_night_from_the_hall, portal_catalog__portal_10_win_condition
expect hub.state.10 == 3
walk_to 7 2 900
walk_to 7 -19 900
face -1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 10
expect portal.act == 0
expect portal.agents == 3
bot 60000
expect_event star_collected:10
expect_event portal_won:10
expect mode == 0
expect progress.star.10 == 1

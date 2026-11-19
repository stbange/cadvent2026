# Day 3 is a shipped game: on 3 December it opens without any override and the
# completion bot builds a base, takes the bridge, topples the Spire and has an
# engineer carry the Shining Star out of the vault.
# portal_03_frostfront__spire_falls_and_the_star_comes_home, portal_catalog__portal_03_win_condition
expect hub.state.3 == 3
walk_to -7 2 900
walk_to -7 -14 900
face 1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 3
expect portal.buildings == 1
expect portal.credits >= 3500
bot 60000
expect_event portal_won:3
expect_event star_collected:3
expect mode == 0
expect progress.star.3 == 1
expect progress.completed.3 == 1

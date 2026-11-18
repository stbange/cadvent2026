# Day 2 is a shipped game: on 2 December it opens without any override and the
# completion bot wins it from the hall.
# portal_02_aurora_carpet__win_by_mana_share_or_ruins, shining_stars__one_star_per_portal
expect hub.state.2 == 3
walk_to -7 2 900
walk_to -7 -9 900
face 1.5708
move 0 1 200
wait_until mode == 1 120
expect day == 2
expect portal.total_mana == 1000
expect portal.castle.level == 0
bot 40000
expect_event portal_won:2
expect_event star_collected:2
expect mode == 0
expect progress.star.2 == 1
expect progress.completed.2 == 1

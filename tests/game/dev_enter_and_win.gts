# Development override: walk into portal 4 (test range), let the bot win, return to the hall.
# hub_hall__return_to_hall, portal_framework__best_result_kept (first result)
walk_to -7 2 900
walk_to -7 -19 900
face 1.5708
move 0 1 120
wait_until mode == 1 120
expect_event portal_entered:4
expect day == 4
expect portal.enemies.alive == 3
bot 30000
expect_event portal_won:4
expect_event star_collected:4
expect mode == 0
expect progress.completed.4 == 1
expect progress.snowflakes.4 >= 2
expect hub.snowflakes >= 2

# Development override: walk into portal 2 (test range), let the bot win, return to the hall.
# hub_hall__return_to_hall, portal_framework__best_result_kept (first result)
walk_to -7 2 900
walk_to -7 -9 900
face 1.5708
move 0 1 120
wait_until mode == 1 120
expect_event portal_entered:2
expect day == 2
expect portal.enemies.alive == 3
bot 30000
expect_event portal_won:2
expect_event star_collected:2
expect mode == 0
expect progress.completed.2 == 1
expect progress.snowflakes.2 >= 2
expect hub.snowflakes >= 2

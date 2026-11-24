# Development override: walk into portal 10 (test range), let the bot win, return to the hall.
# hub_hall__return_to_hall, portal_framework__best_result_kept (first result)
walk_to 7 2 900
walk_to 7 -19 900
face -1.5708
move 0 1 120
wait_until mode == 1 120
expect_event portal_entered:10
expect day == 10
expect portal.enemies.alive == 3
bot 30000
expect_event portal_won:10
expect_event star_collected:10
expect mode == 0
expect progress.completed.10 == 1
expect progress.snowflakes.10 >= 2
expect hub.snowflakes >= 2

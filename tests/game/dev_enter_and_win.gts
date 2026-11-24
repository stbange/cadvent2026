# Development override: walk into portal 11 (test range), let the bot win, return to the hall.
# hub_hall__return_to_hall, portal_framework__best_result_kept (first result)
walk_to 7 2 900
walk_to 7 -24 900
face -1.5708
move 0 1 120
wait_until mode == 1 120
expect_event portal_entered:11
expect day == 11
expect portal.enemies.alive == 3
bot 30000
expect_event portal_won:11
expect_event star_collected:11
expect mode == 0
expect progress.completed.11 == 1
expect progress.snowflakes.11 >= 2
expect hub.snowflakes >= 2

# Resuming Day 9 at the victory lap: the match is won, the trophy star waits at
# centre field and the bot carries it home to the flag stand.
# portal_09_skifrost__victory_lap
expect portal.phase == 3
expect portal.score.us == 5
wait 20
screenshot p09_lap_start.ppm
bot_for 700
screenshot p09_lap_star.ppm
bot 6000
expect_event star_collected:9
expect_event portal_won:9

# Day 2 from the nightfall checkpoint: the bot fetches the volcano scroll,
# erupts it under Vael's castle and catches the Shining Star.
# portal_02_aurora_carpet__resume_at_nightfall
expect portal.phase == 2
expect portal.castle.level >= 2
wait 30
screenshot p02_night.ppm
bot 30000
expect_event portal_won:2
expect_event star_collected:2

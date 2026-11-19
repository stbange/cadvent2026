# Day 6 from the duel checkpoint, rendered: the frozen crater with its steam
# vents, Krampus, beams lighting the snow, and the bot winning the campaign.
# portal_06_iron_yule__resume_at_the_duel
expect portal.mission == 3
expect portal.krampus.hp > 100
wait 30
screenshot p06_crater.ppm
bot_for 300
screenshot p06_duel.ppm
bot_for 300
screenshot p06_duel2.ppm
bot 30000
expect_event portal_won:6

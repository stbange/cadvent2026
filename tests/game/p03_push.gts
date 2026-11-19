# Day 3 from the base checkpoint, rendered: the bot trains an army, crosses the
# bridge, fights through the pylons and brings the star home.
# portal_03_frostfront__resume_with_the_base_standing
expect portal.buildings == 6
expect portal.harvesters == 1
wait 30
screenshot p03_checkpoint.ppm
bot 60000
expect_event portal_won:3
expect_event star_collected:3

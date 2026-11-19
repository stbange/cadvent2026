# Day 5 from the spring checkpoint, rendered: the lake is open, the bot builds
# a shipyard and a ferry, ships the army north, burns the Longhall and sails
# the star home to the hearth.
# portal_05_hearthguard__resume_in_spring
expect portal.thawed == 1
expect portal.raid == 3
wait 30
screenshot p05_spring.ppm
bot_for 4000
screenshot p05_s1.ppm
bot_for 2000
screenshot p05_s2.ppm
bot_for 2000
screenshot p05_s3.ppm
bot_for 2000
screenshot p05_s4.ppm
bot 60000
expect_event portal_won:5
expect_event star_collected:5

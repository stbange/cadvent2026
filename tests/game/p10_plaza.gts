# Resuming Day 10 at the plaza: the star is taken, the VTOL is a minute out and
# Halcyon sends waves; the bot holds the pad and boards.
# portal_10_neon_syndic__extraction
expect portal.act == 4
expect portal.star.taken == 1
wait 20
screenshot p10_plaza.ppm
bot_for 2400
screenshot p10_vtol.ppm
bot 4000
expect_event star_collected:10
expect_event portal_won:10

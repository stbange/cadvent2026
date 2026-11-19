# Day 4 from the crypt checkpoint, rendered: the vent melts the ice wall, gear 3
# wakes the Colossus, the gallery collapses during the run back.
# portal_04_ice_tomb__resume_at_a_checkpoint
expect portal.gears == 2
wait 20
screenshot p04_checkpoint.ppm
bot 20000
expect_event portal_won:4
expect_event star_collected:4

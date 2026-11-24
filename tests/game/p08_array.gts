# Day 8 from the array checkpoint, rendered: the blizzard, the slams from
# under the ice, the three breakers and the falling shard.
# portal_08_cascade__resume_at_the_array
expect portal.act == 5
expect portal.breakers == 0
wait 30
screenshot p08_array_start.ppm
bot_for 900
screenshot p08_array_fight.ppm
bot 20000
expect_event portal_won:8
expect_event star_collected:8

# Rendered walk over Day 6: the mech lab, the canyon at dusk with Sleighbell
# and the wingmen, a fight, the Bloodmaker wreck, and the coolant call.
wait 5
expect portal.lab == 1
screenshot p06_lab.ppm
press alt
press alt
expect portal.paint == 2
press use
wait 30
expect portal.mission == 1
screenshot p06_canyon.ppm
bot_for 1100
screenshot p06_ambush.ppm
teleport 300 0 385 0
wait 5
expect portal.marks >= 2
screenshot p06_wreck.ppm
press map
wait 2
screenshot p06_map.ppm
press map
press use
wait 200
screenshot p06_coolant.ppm
press pause
wait 2

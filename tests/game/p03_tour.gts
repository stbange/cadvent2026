# Rendered walk over Day 3: landfall, the bot building the base, the tablet,
# the placement beam, the bridge, the lab, the Spire and the recon drone.
wait 5
screenshot p03_start.ppm
bot_for 2400
expect portal.buildings >= 4
screenshot p03_base.ppm
teleport 94 0 108 0
turn 0 -0.15
wait 10
expect portal.marks >= 2
screenshot p03_bridge.ppm
teleport 140 0 90 0
wait 5
expect portal.marks >= 3
screenshot p03_lab.ppm
teleport 94 0 66 0
wait 30
screenshot p03_spire.ppm
press jump
wait 60
screenshot p03_drone.ppm
press map
wait 2
screenshot p03_tablet.ppm
press map
press pause
wait 2

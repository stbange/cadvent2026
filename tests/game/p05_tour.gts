# Rendered walk over Day 5: the village at dusk, the bot building it up,
# the tavern sign, the frozen lake with the owl, the troll camp, the map.
wait 5
screenshot p05_start.ppm
bot_for 4200
expect portal.buildings >= 4
screenshot p05_village.ppm
teleport 75 0 180 0
wait 5
expect portal.marks >= 8
screenshot p05_tavern.ppm
teleport 96 0 112 0
turn 0 -0.1
wait 5
screenshot p05_lake.ppm
teleport 96 0 72 0
wait 5
screenshot p05_camp.ppm
press map
wait 2
screenshot p05_map.ppm
press map
press pause
wait 2

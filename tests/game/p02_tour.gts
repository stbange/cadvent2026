# Rendered flight over Day 2 at dusk: terrain, sea, aurora, castles, creatures,
# spells in flight, the dagger rock and the giant runes.
wait 5
screenshot p02_start.ppm
hold fire 20
hold alt 12
bot_for 1500
expect portal.castle.level >= 1
screenshot p02_castle.ppm
press w3
turn 0 -0.4
hold fire 5
press w4
hold fire 5
teleport 168 30 84 -0.97
wait 10
screenshot p02_vael.ppm
teleport 128 30 206 3.1416
wait 10
expect portal.marks >= 4
screenshot p02_rock.ppm
teleport 128 72 168 0
turn 0 -0.9
wait 10
expect portal.marks >= 6
screenshot p02_runes.ppm
press w6
press next
press prev
press pause
wait 2

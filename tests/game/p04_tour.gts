# Rendered walk through Day 4: the entrance cavern and journal, the Hall of
# Pillars with its block and platform, the blade, the flooded crypt (above and
# under water), the ice wall in the gallery and the solar door.
wait 5
screenshot p04_entrance.ppm
teleport 12.0 4 91.5 1.5708
wait 10
expect portal.marks >= 4
screenshot p04_journal.ppm
teleport 16.5 6 66 0
wait 10
screenshot p04_hall.ppm
teleport 16.0 6 43.5 0
wait 10
expect portal.marks >= 6
screenshot p04_blade.ppm
teleport 20.5 6 33.6 0
turn 0 -0.4
wait 10
screenshot p04_crypt.ppm
teleport 20.5 3 26 0
wait 10
screenshot p04_underwater.ppm
teleport 34.5 6 34 0
wait 10
screenshot p04_ice.ppm
teleport 34.5 6 47 3.1416
wait 10
screenshot p04_solar.ppm
press fire
wait 3
press pause
wait 2

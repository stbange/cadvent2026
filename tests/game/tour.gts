# Rendered tour through the hall and a portal (exercises the GPU paths).
wait 5
screenshot tour_spawn.ppm
teleport -10 0 -9 1.5708
wait 100
screenshot tour_portals.ppm
teleport 0 0 -30 0
wait 10
screenshot tour_apse.ppm
teleport 6 0 6 3.1416
wait 5
screenshot tour_legends.ppm
teleport 14 0 -4 -1.5708
move 0 1 40
wait_until mode == 1 60
wait 10
screenshot tour_test_range.ppm
hold fire 30
press pause
wait 5
expect mode == 0

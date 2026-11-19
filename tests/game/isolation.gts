# portal_framework__no_leaked_state: portals 7, 8, 7 again start fresh
expect day == 7
press pause
wait 2
teleport 14 0 -9 -1.5708
move 0 1 60
expect day == 8
press pause
wait 2
teleport 14 0 -4 -1.5708
move 0 1 60
expect day == 7
expect portal.enemies.alive == 3
expect portal.keys == 0

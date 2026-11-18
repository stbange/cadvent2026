# portal_framework__no_leaked_state: portals 1, 2, 1 again start fresh
expect day == 1
press pause
wait 2
teleport -14 0 -9 1.5708
move 0 1 60
expect day == 2
press pause
wait 2
teleport -14 0 -4 1.5708
move 0 1 60
expect day == 1
expect portal.enemies.alive == 3
expect portal.keys == 0

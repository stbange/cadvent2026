# portal_framework__no_leaked_state: portals 3, 4, 3 again start fresh
expect day == 3
press pause
wait 2
teleport -14 0 -19 1.5708
move 0 1 60
expect day == 4
press pause
wait 2
teleport -14 0 -14 1.5708
move 0 1 60
expect day == 3
expect portal.enemies.alive == 3
expect portal.keys == 0

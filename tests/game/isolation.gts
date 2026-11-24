# portal_framework__no_leaked_state: portals 11, 12, 11 again start fresh
expect day == 11
press pause
wait 2
teleport 14 0 -29 -1.5708
move 0 1 60
expect day == 12
press pause
wait 2
teleport 14 0 -24 -1.5708
move 0 1 60
expect day == 11
expect portal.enemies.alive == 3
expect portal.keys == 0

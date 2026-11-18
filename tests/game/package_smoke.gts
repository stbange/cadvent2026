# Runs from an extracted release archive: Day 1's level and the friends file load.
expect mode == 1
expect day == 1
expect portal.keys == 0
press pause
wait 2
expect mode == 0

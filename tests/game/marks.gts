# friends_marks__registering_graffiti, hub_hall__mark_found
expect portal.marks == 0
teleport -6 0 -3 1.5708
wait 3
expect portal.marks == 1
press pause
wait 2
expect progress.marks.2 == 1
expect hub.legend.0 == 1

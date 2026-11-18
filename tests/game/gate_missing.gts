# shining_stars__missing_stars_on_christmas_eve (fixture: sockets 7 and 18 empty)
expect hub.state.24 == 2
expect hub.beams == 2
walk_to 4.5 0 900
walk_to 4.5 -24 2400
walk_to 0 -30 2400
walk_to 0 -38.2 2400
move 0 1 60
expect_event denied
expect mode == 0

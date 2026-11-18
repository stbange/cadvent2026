# shining_stars__setting_stars (fixture: stars of days 3 and 5 carried)
expect hub.stars_carried == 2
expect hub.stars_socketed == 0
walk_to 4.5 0 900
walk_to 4.5 -24 2400
walk_to 0 -30 2400
walk_to 0 -36.5 2400
press use
wait 2
expect_event star_socketed
expect hub.stars_socketed == 2
expect hub.stars_carried == 0
expect progress.socket.3 == 1
expect progress.socket.5 == 1
expect progress.socket.4 == 0

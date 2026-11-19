# hub_hall__blocked_sealed_portal (run with --date 2026-12-05T23:59)
# advent_calendar__portal_sealed_before_its_day (in the running game)
expect hub.state.6 == 0
expect hub.state.5 == 3
walk_to -7 2 900
walk_to -7 -29 2400
walk_to -14 -29 900
face 1.5708
move 0 1 90
expect_event denied
expect mode == 0

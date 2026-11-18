# hub_hall__blocked_sealed_portal (run with --date 2026-12-04T23:59)
# advent_calendar__portal_sealed_before_its_day (in the running game)
expect hub.state.5 == 0
expect hub.state.4 == 1
walk_to -7 2 900
walk_to -7 -24 2400
walk_to -14 -24 900
face 1.5708
move 0 1 90
expect_event denied
expect mode == 0

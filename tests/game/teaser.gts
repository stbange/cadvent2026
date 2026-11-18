# Teaser before the season (run with --date 2026-11-18): every portal is
# sealed, walking into portal 1 is refused, and the countdown is on show.
# advent_calendar__teaser_before_december, hub_hall__blocked_sealed_portal
expect hub.state.1 == 0
expect hub.state.2 == 0
expect hub.state.12 == 0
expect hub.state.24 == 0
walk_to -7 2 900
walk_to -7 -4 900
face 1.5708
move 0 1 200
expect_event denied
expect mode == 0

# A portal whose day has come but whose game is not shipped yet stays closed
# (run with --date 2026-12-05).
expect hub.state.4 == 1
expect hub.state.1 == 3
walk_to -7 2 900
walk_to -7 -19 900
face 1.5708
move 0 1 200
expect_event denied
expect mode == 0

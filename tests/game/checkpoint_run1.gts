# portal_framework__resume_after_restart, part 1: reach the checkpoint, quit
expect mode == 1
teleport 6 0 4.5 0
wait 5
expect portal.keys == 1
press pause
wait 2
expect mode == 0
expect progress.checkpoint.3 == 1

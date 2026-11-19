# on_screen_help__help_pauses_the_world: F1 opens the help, the world pauses
# while it is open (moving does nothing), and F1 or Esc closes it again.
# on_screen_help__reminder_on_entry: entering a world shows the "F1 HELP" hint.
expect help.hint > 300
teleport 0 0 3 0
wait 2
expect help.open == 0
press help
expect_event help_opened
expect help.open == 1
move 0 1 60
expect portal.player.x == 0
expect portal.player.z == 3
press next
press pause
expect help.open == 0
expect mode == 1
press help
wait 5
press help
expect_event help_closed
move 0 1 30
expect portal.player.z < 3

# hub_hall__epilogue_trigger, shining_stars__crowning_the_tree
# (fixture: the 24th star was won)
expect hub.crowned == 0
walk_to 2.6 -2 900
walk_to 2.6 -13.2 900
press use
wait 2
expect_event tree_crowned
expect hub.crowned == 1
expect hub.epilogue == 1
wait 1500
expect_event epilogue_end
expect hub.epilogue == 0

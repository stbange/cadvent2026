# Day 7 resumed at half time (2:2), rendered: the teams have switched sides,
# money is back to the start, and the bot plays on as the Guard.
# portal_07_tinsel_strike__resume_at_half_time
expect portal.round == 5
expect portal.score.us == 2
expect portal.score.them == 2
expect portal.side == 1
expect portal.money == 800
wait 30
screenshot p07_half.ppm
bot_for 1500
screenshot p07_half_fight.ppm
bot_for 1500
expect portal.score.us >= 2

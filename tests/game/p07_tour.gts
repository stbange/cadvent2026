# Rendered walk over Day 7: buy time in the spawn, the buy menu, the market
# square with the giant tree, a fight, the spray tag and the scoreboard.
wait 5
expect portal.phase == 0
screenshot p07_spawn.ppm
press w6
expect portal.buy.open == 1
screenshot p07_buy.ppm
press w6
bot_for 1200
screenshot p07_live.ppm
teleport 33 0 30 0
wait 5
expect portal.marks >= 1
screenshot p07_square.ppm
hold map 3
screenshot p07_scores.ppm
bot_for 600
screenshot p07_fight.ppm
press pause
wait 2

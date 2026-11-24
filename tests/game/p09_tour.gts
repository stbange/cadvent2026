# Rendered run through Day 9: the training course (ski down the hill, jet to
# the beacon, disc the drone), the countdown, the ski run across the basin
# and the attack on the Owls' hill.
# portal_09_skifrost__training_course
wait 30
screenshot p09_training.ppm
expect portal.train == 0
bot_for 400
screenshot p09_ski.ppm
expect portal.train >= 1
bot_for 900
screenshot p09_jet.ppm
bot_for 600
expect portal.phase >= 1
screenshot p09_countdown.ppm
bot_for 900
screenshot p09_basin.ppm
bot_for 900
screenshot p09_attack.ppm
press pause
wait 2

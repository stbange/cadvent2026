# Rendered run through Day 10: the rainy alley, the market crowd and the relay
# kiosk, the stolen hover-car, the factory and the Halcyon Tower gauntlet.
# portal_10_neon_syndic__hacking_relays
wait 30
screenshot p10_alley.ppm
bot_for 500
screenshot p10_market.ppm
bot_for 700
expect portal.relays >= 1
screenshot p10_car.ppm
bot_for 1500
screenshot p10_factory.ppm
bot_for 2400
screenshot p10_tower.ppm
press pause
wait 2

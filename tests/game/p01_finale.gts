# Day 1 from the roof checkpoint: the bot breaks the Siphon, takes the star and
# escapes - renders the collapse, the exit beam and the victory banner.
expect portal.boss_awake == 1
bot 20000
expect_event portal_won:1
expect_event star_collected:1

# Proposal

## Why

Day 9 opens on 9 December 2026: *Skifrost*, an original homage to the
jetpack-and-skiing team shooter (concept: `docs/concept/portals/week-2.md`,
Day 9). Two winter tribes, the Rime Wolves and the Ember Owls, settle the
festival games of the Northern Reach with Capture the Flag on Snowglobe
Basin. It needs skiing armour for the Vehicle kit and CTF pieces for the FPS
kit.

## What Changes

- Vehicle kit (skier): armoured skiers on heightmap terrain. Holding jump on
  the ground skis without friction, so gravity along the slope builds speed
  and landings keep the momentum along the surface. A jetpack drains an
  energy reserve that regenerates otherwise; an empty pack coughs out for a
  moment. Three armour classes trade speed and energy for health and weapon
  slots. Above running speed the input only steers.
- FPS kit (CTF): flags that are taken, carried, dropped with a 30 second
  return timer and returned by a touch; captures that need your own flag at
  home; generators that power turrets and stations, go dark when destroyed
  and are repaired after a while.
- Day 9 portal: a short training course (ski to 100 km/h, jet to a beacon,
  disc a moving drone), then 4 v 4 on Snowglobe Basin: two hill-top bases,
  the halfpipe trench, a ridge across the middle. Disc launcher (slow, splash,
  knockback, half the shooter's speed), chaingun with spin-up, grenade
  launcher; the inventory station swaps armour; bots ski two precomputed
  downhill lines in roles (capper, heavy offence, defence, midfield) and lead
  their discs. First to five captures, or the lead after 20 minutes (sudden
  death on a tie); the trophy star is carried home on a victory lap.
  Snowflakes, checkpoints after your captures, friends' marks, rendering,
  help page, completion bot.

Out of scope (garnish): the hover sled, the sensor jammer, the mid-air
"pinball" announcer beyond the kill-feed special.

## Capabilities

### New Capabilities
- `vehicle-skier`: skiing, jetpack energy, armour classes.
- `fps-ctf`: flags, captures, generators, turrets.
- `portal-09-skifrost`: rules, goal, scoring and content of Day 9.

### Modified Capabilities
- none

## Impact

`src/kits/vehicle/skier.*`, `src/kits/fps/ctf.*`, `src/portals/p09_*`,
registry, views, help, tests. Dev and practice-range scenarios move to
portals 10 and 11.

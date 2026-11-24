# Proposal

## Why

Day 10 opens on 10 December 2026: *Neon Syndic*, an original homage to the
cyberpunk squad-tactics classic (concept: `docs/concept/portals/week-2.md`,
Day 10). In the rain-soaked megacity of Nivalis, Halcyon Corp runs the
city's mood through a cheer-broadcast network; the player leads a four-agent
cell of the Sleepless Choir to cut its relays. It brings the first pieces of
the RPG / tactics kit and destructible facades for the FPS kit.

## What Changes

- RPG kit (party): squad orders (follow, hold, attack, cover) and wedge
  formation slots; overclock sliders (speed, aim, awareness) that drain a
  stamina pool and crash for a while when it runs dry; a radial ring menu
  steered by looking.
- RPG kit (persuade): a beacon that converts people near you over a moment
  of exposure, with an allowance that grows with the crowd and minds that
  need a crowd first; the hacking mini-game, a timing ring with a moving gap.
- FPS kit (facade): building fronts made of voxel chunks with hit points;
  blasts chip and remove chunks and loose chunks fall.
- Day 10 portal: the lead agent and three AI agents (one with a friend's
  codename) in a night city: the market relay among crowds, a hover-car that
  rams the road barrier, the factory relay, the Halcyon Tower turret gauntlet
  with breakable pillars, the Shining Star in the antenna casing, a one
  minute hold on the plaza until the VTOL lands. Minigun, laser, gauss; the
  Charm Beacon crowd that soaks up fire and sways guards; alert levels with
  vans of reinforcements; neon lamps, holo ads that dim relay by relay, rain,
  headlights; snowflakes, checkpoints, friends' marks, completion bot, help.

Out of scope (garnish): cyber-upgrades between relays, reflective puddles.

## Capabilities

### New Capabilities
- `rpg-party`: orders, formation, overclock, radial ring.
- `rpg-persuade`: persuasion and hacking.
- `fps-facade`: destructible voxel facades.
- `portal-10-neon-syndic`: rules, goal, scoring and content of Day 10.

### Modified Capabilities
- none

## Impact

`src/kits/rpg/*`, `src/kits/fps/facade.*`, `src/portals/p10_*`, registry,
views, help, tests. Dev and practice-range scenarios move to portals 11 and
12.

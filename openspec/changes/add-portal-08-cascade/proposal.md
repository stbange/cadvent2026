# Proposal

## Why

Day 8 opens on 8 December 2026: *Cascade*, an original story-driven
shooter homage told entirely in first person (concept:
`docs/concept/portals/week-2.md`, Day 8). Technician Ines Kowalczyk arrives
at the Arctic station Polaris Deep on the day the Lumen Shard resonance test
goes wrong. It needs story pieces for the FPS kit.

## What Changes

- FPS kit (story): trigger volumes (once or every entry) for scripted beats
  that never take control away, NPC followers on the walk grid (follow, wait,
  go to), a squad AI decision rule (patrol, engage, flank, suppress, grenade,
  retreat) and wall chargers with a limited reserve. The walk grid grows to
  96 cells per side; ray hits on short targets now count the cylinder caps.
- Day 8 portal: the monorail ride, the lobby (suit locker, C6P sweeping), the
  test chamber and the cascade, the ruins (ice axe, pistol, skitterlings, the
  wounded guard who joins, radiology scanner, two power cells, the generator,
  a pushable crate to the shotgun ledge), Blackice (two squads, the safe lab,
  scientists escorted to the retina scanner, chargers, crossbow, grenades),
  the antenna array in a blizzard (slams from under the ice, skitterling
  waves, three breakers), the shard falling at your feet; snowflakes,
  checkpoints, friend marks, a completion bot, rendering and a help page.

Out of scope (garnish): the Archivist observer, soldier radio banter beyond
the key lines.

## Capabilities

### New Capabilities
- `fps-story`: triggers, followers, squad AI, chargers.
- `portal-08-cascade`: rules, goal, scoring and content of Day 8.

### Modified Capabilities
- none

## Impact

`src/kits/fps/story.*`, `src/kits/fps/tactics.h`, `src/sim/collide.c`,
`src/portals/p08_*`, registry, views, help, tests. Dev and practice-range
scenarios move to portals 9 and 10.

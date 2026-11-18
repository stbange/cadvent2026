# Proposal

## Why

Day 2 opens on 2 December 2026: *Aurora Carpet*, an original flying-carpet
god-game homage (concept: `docs/concept/portals/week-1.md`, Day 2). It is the
first portal on the Vehicle kit, which Days 6, 9, 11, 20 and 23 reuse.

## What Changes

- Add the Vehicle kit: a deformable heightmap terrain (islands, deform,
  raise-to, flatten, raycast, line of sight, per-chunk change counters for
  the renderer) and a hovering flyer with clearance, ceiling, world bounds and
  steep-face collision.
- Add the Day 2 portal: mana orbs with ownership, claim/fireball/raise/lower/
  castle/volcano spells, balloon carriers, castles that grow in four levels
  with turrets, the rival wizard Vael, creatures that carry mana, day/night
  phases, the Shining Star at the aurora or in Vael's ruins, friend marks.
- Scenario scripts gain `bot_for N` (play N ticks without requiring an end).
- Dev scenarios that used portal 2 as the test range move to portals 3 and 4.

Out of scope (garnish): possession spell, anaglyph mode, frost-worms that
collapse terrain, versus multiplayer, XR.

## Capabilities

### New Capabilities
- `portal-02-aurora-carpet`: rules, goal, scoring and content of Day 2.
- `vehicle-kit`: terrain and flight shared by vehicle portals.

### Modified Capabilities
- none

## Impact

`src/kits/vehicle/`, `src/portals/p02_*`, registry, views, script DSL, tests.
No new asset files: the fjords, castles and creatures are generated in code.

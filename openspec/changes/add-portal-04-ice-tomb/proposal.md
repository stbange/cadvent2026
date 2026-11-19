# Proposal

## Why

Day 4 opens on 4 December 2026: *Ice Tomb of the Winter King*, an original
grid-based tomb-exploring homage played in first person (concept:
`docs/concept/portals/week-1.md`, Day 4). It needs the small Platformer kit.

## What Changes

- Platformer kit: a block-grid world (1 tile = 1 m) and a grid-aware
  traversal controller: tile-sized standing and running jumps snapped to the
  grid, ledge grab with a tolerance window, shimmy, pull-up, one-tile step
  climb, block push and pull, swimming with an air meter, edge depenetration.
- Day 4 portal: entrance cavern (pits, wall), Hall of Pillars (block puzzle,
  lever + timed door, Gear 1), blade corridor, flooded crypt (underwater
  lever, tunnel with an air pocket, Gear 2), frozen colossus gallery (vent
  melts the ice wall, Gear 3 wakes the Colossus, the floor collapses during
  the run back), solar door and Shining Star; save crystals, jade secrets,
  wolves, bats, a frost bear, dual pistols with auto-aim, friend marks,
  checkpoints, a route-following completion bot and a help page.

Out of scope (garnish): swan dive, grappling icicles, photo mode, co-op
levers, ghost race, XR comfort vignette.

## Capabilities

### New Capabilities
- `platformer-kit`: tiles and traversal for grid platformer portals.
- `portal-04-ice-tomb`: rules, goal, scoring and content of Day 4.

### Modified Capabilities
- none

## Impact

`src/kits/platformer/`, `src/portals/p04_*`, registry, views, help, tests.
The tomb is built in code; no asset files.

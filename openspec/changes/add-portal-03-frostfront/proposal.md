# Proposal

## Why

Day 3 opens on 3 December 2026: *Frostfront*, an original real-time strategy
homage in which the player commands on foot (concept:
`docs/concept/portals/week-1.md`, Day 3). The RTS kit it needs is the largest
kit of the calendar and is reused by Days 5, 18, 21, 23 and 24, so it starts
first and lands in slices.

## What Changes

- Add the RTS kit (slice 1, done): battlefield grid with ore, building
  placement rules, 8-way A* paths, sides with credits and power, build queues,
  units with move/attack/attack-move/stop orders and control groups,
  harvesters (seek, mine, return, unload), defences that need power, and a
  state hash for lockstep checks.
- Add the Day 3 portal (slice 2): commander on foot with the wrist tablet,
  Polar Accord vs Order of the Pale Sun, Frostite fields, 8 unit types and
  6 structures, AI build orders and attack waves, the Ember Spire, the
  Shining Star carried out of the vault by an engineer, friend marks.

## Capabilities

### New Capabilities
- `rts-kit`: grid, economy, orders and harvesting shared by RTS portals.
- `portal-03-frostfront`: rules, goal, scoring and content of Day 3.

### Modified Capabilities
- none

## Impact

`src/kits/rts/`, later `src/portals/p03_*`, registry, views and tests.

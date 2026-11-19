# Proposal

## Why

Day 5 opens on 5 December 2026: *Hearthguard*, an original three-resource
fantasy RTS homage played in first person (concept:
`docs/concept/portals/week-1.md`, Day 5). The gnome village of Kindlewick
must hold out against the troll raiders of the Rimeback Clans, then cross
the thawed lake and burn their Longhall. It extends the RTS kit.

## What Changes

- RTS kit: three resources (gold, lumber, tallow) with blocking resource
  cells (trees) that open the way when felled, water cells and naval paths,
  supply (food) caps from farms and halls, harvesters that work from beside
  a resource and return to the nearest drop-off, transports that load,
  carry and unload units, and melee range measured to a building's edge.
- Day 5 portal: the valley (forests, gold mine, whale-tallow platform,
  frozen lake, troll camp, neutral tavern), the Captain on foot with a
  command stave (six structures, six units, the keep upgrade), picking,
  sweep selection, groups, orders, a shield bash, three scripted raids, the
  warded Longhall, the spring thaw (the ice gives way under anyone on it),
  shipyard on the shore, ferry crossing, the star in the fire pit and the
  run home, a village foreman for idle and stranded workers, friend marks,
  checkpoints (after raid 1, in spring), snowflakes, a completion bot on
  every difficulty, rendering and a help page.

Out of scope (garnish): hero abilities, naval combat, oil-style fire damage
on structures, multiplayer skirmish.

## Capabilities

### New Capabilities
- `portal-05-hearthguard`: rules, goal, scoring and content of Day 5.

### Modified Capabilities
- `rts-kit`: resources, supply, water and transports.

## Impact

`src/kits/rts/`, `src/portals/p05_*`, registry, views, help, tests. Dev and
practice-range scenarios move from portal 5 to portals 6 and 7. The valley
is built in code; no asset files.

# Proposal

## Why

Day 6 opens on 6 December 2026: *Iron Yule*, an original walker-cockpit
homage (concept: `docs/concept/portals/week-1.md`, Day 6). Pilot Sable of
the Hollowfrost Militia defends the Winterhold valley from Grail-Rex
Industries, with the convoy AI Santa Claude and his crawler Sleighbell on the
radio. It needs a walker in the Vehicle kit.

## What Changes

- Vehicle kit: a walker controller: legs and torso turn separately (torso
  twist with a limit), a throttle with reverse, slope limits, jump jets with
  fuel, a heat model with shutdown and restart, cooling multipliers, eight
  hit zones with damage transfer (a lost side torso takes its arm, a lost
  limb passes damage inwards), leg damage slows, and destruction when the
  head, centre torso or both legs are gone.
- Day 6 portal: a flat-shaded valley (ambush canyon, reservoir and ice dam,
  frozen crater with steam vents), a mech lab (four hardpoints within ten
  tons, three paint schemes), weapon groups with chain fire and alpha strike,
  missile lock, the three-mission campaign (escort Sleighbell, the star from
  its cargo bay; destroy the dam pumps; duel Krampus), wingmen, coolant drops,
  convoy restarts, ejection, checkpoints, snowflakes, friend marks, a
  completion bot, rendering, the cockpit HUD and a help page.

Out of scope (garnish): night mission with thermal vision, salvage, present
crates with upgrade parts, an interactive XR cockpit.

## Capabilities

### New Capabilities
- `portal-06-iron-yule`: rules, goal, scoring and content of Day 6.

### Modified Capabilities
- `vehicle-kit`: the walker.

## Impact

`src/kits/vehicle/walker.*`, `src/portals/p06_*`, registry, views, help,
tests. Dev and practice-range scenarios move to portals 7 and 8. The valley
is generated in code; no asset files.

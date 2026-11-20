# Proposal

## Why

Day 7 opens on 7 December 2026: *Tinsel Strike*, an original round-based
team-tactics homage (concept: `docs/concept/portals/week-2.md`, Day 7). In
the snowy market town of Glühstadt the Humbug Syndicate tries to set off a
glitter bomb, the Nutcracker Guard defends; the tagged pop into confetti.
It needs tactical pieces for the FPS kit.

## What Changes

- FPS kit (tactics): deterministic recoil patterns, movement inaccuracy, the
  economy rules (start, kill and knife rewards, win rewards by reason, loss
  streak bonus, plant bonus, money cap), smoke blocking sight, and a walk grid
  with cover cells and 8-way A* paths for bots.
- Day 7 portal: the market map (two sites, the square with the giant tree,
  alleys and stalls), 5v5 with four bot teammates, rosters with the friends,
  best of 9 with a side switch after round 4, 15 s buy time with a buy menu,
  1:45 rounds, seven weapons with recoil, armour, defuse kit, flashbang,
  smoke, footsteps (walking is silent), a glitter bomb (3 s plant, 40 s fuse,
  10 s defuse, 5 s with a kit, drop and pick up), headshots, knife bonus,
  spectating, bots with roles, rotations and callouts, the spray tag, the
  post-match climb up the tree to the Shining Star, checkpoints at half time,
  snowflakes, a completion bot, a bot-vs-bot balance check, rendering and a
  help page.

Out of scope (garnish): hostage-rescue variant, mini-radar, online play.

## Capabilities

### New Capabilities
- `fps-tactics`: recoil, spread, economy, smoke and the walk grid.
- `portal-07-tinsel-strike`: rules, goal, scoring and content of Day 7.

### Modified Capabilities
- none

## Impact

`src/kits/fps/tactics.*`, `src/portals/p07_*`, registry, views, help, tests.
Dev and practice-range scenarios move to portals 8 and 9. The market is
built in code; no asset files.

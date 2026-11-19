# Proposal

## Why

Every world plays differently (shooter, flying god game, real-time strategy
on foot, ...). Players need a clear, good-looking reminder of the goal and
the controls in each of them, on keyboard, gamepad and touch alike.

## What Changes

- A shared help overlay for the hall and every portal: title band, goal,
  controls as key chips (labels for the device in use), tips; game paused
  while open; slides in; a second page on small screens.
- Toggle: F1 or H, the gamepad's Back button (the map moves to D-pad up), or
  a "?" touch button. Esc also closes it without leaving the portal.
- A small "F1 HELP" reminder after entering a world.
- Touch buttons are now drawn over portals too, not only in the hall.

## Capabilities

### New Capabilities
- `on-screen-help`: help content, toggle, pause and layout.

### Modified Capabilities
- none

## Impact

`src/portal/help.*` (content and drawing), `src/app/app.*` (toggle, pause,
overlay), input bindings, script action `help`, tests.

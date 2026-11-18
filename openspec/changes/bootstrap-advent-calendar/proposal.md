# Proposal

## Why

The project starts from an empty repository and must deliver a playable,
tested advent calendar by 1 December 2026, followed by one new portal game
per day until 24 December. A shared foundation (calendar logic, hub hall,
portal framework, retro renderer, cross-platform input, quality gates) is
needed before any portal can be built, and it must be specified first so
24 portals can be developed against a stable contract.

## What Changes

- Introduce the date-based unlock rule for 24 portals (1–24 Dec 2026).
- Introduce the hub hall: navigation, portal states, progression (snowflakes,
  ornaments, wreath), epilogue.
- Introduce the portal framework: lifecycle, results, snowflakes, saves,
  capability declaration, deterministic simulation and bot hooks.
- Introduce the retro rendering contract: low internal resolution, dynamic
  lights and shadows, sprites, voxels, meshes, palette post-processing,
  split-screen and stereo views.
- Introduce input & controls: action mapping, keyboard/mouse, gamepad,
  automatic touch mode on mobile web, XR controllers, accessibility options.
- Introduce platform builds: Windows, Linux, Web (WebGL2), WebXR.
- Introduce multiplayer as an optional capability: local split screen and
  online sessions, declared per portal.
- Introduce quality gates: unit-test coverage ≥ 95 %, automated game tests,
  completion bots, determinism, CI.
- Introduce the Shining Star meta-quest: one star per portal 1–23, the
  Star Arch that must be complete before portal 24 opens, and the 24th star
  that crowns the tree.
- Introduce friends' marks: the crew's gamer tags hidden in every portal and
  the Wall of Legends; Santa Claude as guest character in portal 6.
- Use snowflakes (0–3) as the per-portal rating, distinct from Shining Stars.
- Introduce the portal catalogue: the 24 portals with their goals and win
  conditions (details per portal follow in later `add-portal-NN-*` changes).
- Introduce licensing & originality rules (MIT code, CC BY 4.0 assets, no
  reuse of third-party game IP).

Out of scope for this change: the full rules of each individual portal
(each gets its own change), TURN relays, macOS packaging, achievements on
external platforms, accounts.

## Capabilities

### New Capabilities
- `advent-calendar`: date-based unlocking of the 24 portals and season states.
- `hub-hall`: the navigable festive hall, portal states, progression and epilogue.
- `portal-framework`: how a portal game is entered, run, completed, scored and saved.
- `retro-rendering`: the visual contract (low-res, dynamic lighting, shadows, sprites, voxels, palettes, multi-view).
- `input-controls`: device-independent controls including automatic touch mode and XR.
- `platform-builds`: supported targets and their packaging/runtime expectations.
- `multiplayer`: optional local split-screen and online play.
- `quality-gates`: coverage, automated game tests, determinism and CI requirements.
- `shining-stars`: collectible stars in portals 1–23, the Star Arch gate for portal 24 and the 24th star.
- `friends-marks`: configurable friends' names as discoverable marks in all portals.
- `portal-catalog`: the list of 24 portals, their day, goal and minimum content.
- `licensing`: licence and originality obligations for code and assets.

### Modified Capabilities
- none

## Impact

- New code base: `src/`, `tests/`, `tools/`, `web/`, `assets/`, CMake build.
- Dependencies: SDL3, OpenGL/WebGL2, Emscripten, glad, stb_image, cgltf, Unity test framework, gcovr, Playwright (web tests).
- CI: GitHub Actions workflows for Linux, Windows, Web, coverage and spec validation.
- Affects all 24 portals and the five gameplay kits (FPS, RTS, Vehicle, RPG/Tactics, Platformer).

# Design

## Context

Greenfield C17 project. Full concept: `docs/concept/` (esp. `02-technology.md`).
Constraints: one code base for Windows, Linux, Web and WebXR; WebGL2 is the
lowest common GPU denominator; 95 % coverage; a 13-day runway to Day 1 with
daily releases afterwards; a small team.

## Goals / Non-Goals

**Goals:**
- A deterministic, SDL/GL-free simulation layer that every portal plugs into.
- One renderer that delivers the "low-res, high-mood" look on all targets.
- Five reusable gameplay kits so 24 portals are feasible.
- Test infrastructure (unit, scenario, bots, golden frames, replays) from day one.

**Non-Goals:**
- Physically based rendering, high resolutions, modern post effects.
- A general-purpose engine or editor; tools are minimal command-line bakers.
- Mandatory multiplayer for the December release.

## Decisions

1. **C17 + SDL3 + GL 3.3 Core / GLES 3.0.** One GLSL source with an injected
   `#version` preamble. Alternative (bgfx/sokol) rejected to keep the stack
   small and match the request; sokol_gfx remains a fallback if GL issues block web.
2. **Fixed 60 Hz simulation, interpolated rendering.** Enables replays, bots,
   lockstep networking and testing without a GPU.
3. **Portal = module with a descriptor table** (init/tick/render/audio/result/
   hash/bot) and one arena; switching portals frees the arena. The hub is portal 0.
4. **Gameplay kits** under `src/kits/` (FPS, RTS, Vehicle, RPG/Tactics,
   Platformer); portals compose kits + unique mechanics.
5. **Rendering**: forward renderer into a low-res FBO, clustered unshadowed
   lights (≤ 32), ≤ 4 shadowed lights via dual-paraboloid maps + 1 directional
   map, palette 3D-LUT + optional dither, nearest upscale. Sprites carry normal
   maps; voxels greedy-meshed at load.
6. **GL behind a device interface** with a recording null backend for unit
   tests and Mesa llvmpipe for headless golden-frame tests.
7. **Determinism**: own math functions, `-ffp-contract=off`, fixed-point for
   lockstep kits, seeded PCG RNG, canonical state hash.
8. **Testing**: Unity + gcovr, scenario DSL `.gts`, per-portal completion bot,
   Playwright for web; scenario-to-test mapping checked in CI.
9. **Networking** via a transport abstraction (loopback/WebRTC/UDP);
   lockstep for RTS/turn-based, host-authoritative snapshots for action kits.
10. **Web**: Emscripten, IDBFS saves, lazy per-portal asset packages, touch
    overlay in the HTML shell driven by pointer-type detection; WebXR via a
    small JS library.
11. **Branching**: trunk-based on `main` as requested; unfinished portals
    hidden behind build flags; local `ci` workflow preset before pushing.

## Risks / Trade-offs

- [24 games in 24 days] → kits, Core/Garnish split, "still thawing" late-open fallback.
- [95 % coverage vs. iteration speed] → pure sim design, null GL backend, scenario-first TDD.
- [Float divergence wasm vs native] → fixed-point lockstep sims, cross-platform replay tests.
- [WebXR performance on standalone headsets] → per-eye low-res target, XR treated as progressive enhancement.
- [Mobile WebGL2 performance] → budgets enforced by perf smoke tests; resolution scale default per device class.
- [IP] → originality review per portal; no third-party names or assets in-game.

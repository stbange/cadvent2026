# Tasks

## 1. Repository & quality infrastructure (M0, target 22 Nov)

- [x] 1.1 CMake project with presets (linux-gcc, linux-clang, windows-msvc, windows-mingw, web, coverage, ci workflow)
- [x] 1.2 SDL3 (pinned FetchContent, static), glad (GL 3.3 core) and Unity in third_party/ with licence notices
- [x] 1.2b Vendor stb_image and cgltf
- [x] 1.3 Unity test runner + gcovr config with 95 % line / 85 % branch gate and explicit exclusion list
- [x] 1.4 Scenario-to-test traceability script (parses openspec specs, matches test names); test it with fixtures
- [x] 1.5 GitHub Actions: spec validation, Linux, Windows, Web, coverage, sanitizers, Pages deploy
- [x] 1.6 Asset credits checker (every file in assets/ listed in assets/CREDITS.md); test with fixtures

## 2. Core & platform (M0–M1a)

- [x] 2.0 M0 vertical slice: SDL3 window + GL triangle on Linux, Windows and Web, headless smoke test with screenshot probe, browser smoke test

- [x] 2.1a core: arenas, PCG RNG, FNV-1a state hash, logging, command-line parsing, PPM images — with unit tests
- [x] 2.1b core: deterministic math (own trig), fixed-step clock, calendar, key=value save/config store — with unit tests
- [x] 2.2a platform: SDL3 window, GL/GLES context, main-callback loop on desktop and web
- [x] 2.2b platform: timing, prefs path, IDBFS mount on web, URL query parameters, audio stream, gamepad/touch events
- [x] 2.3 Fixed 60 Hz loop with interpolation; headless mode and accelerated speed flag — with unit tests
- [x] 2.4 Save system (text format, versioned) — with unit tests for round-trip and corruption handling

## 3. Rendering (M1a)

- [x] 3.1 GL helper layer and shader preamble for GL 3.3 / GLES 3.0 (decision: no recording null backend — GL paths are covered by headless llvmpipe game tests instead)
- [x] 3.2 Low-res FBO per view, nearest upscale, split-screen layouts (1–4) and stereo views
- [x] 3.3 Forward lights (≤ 32, looped per pixel at low resolution instead of clustering), directional shadow map, 4 cube-map shadowed point lights (instead of dual-paraboloid)
- [x] 3.4 Meshes (glTF subset), 8-direction normal-mapped sprites, greedy-meshed .vox voxels
- [x] 3.5 Post: palette 3D-LUT, Bayer dither, light bands, fog/distance darkness, flat shading
- [ ] 3.5b Era options still open: vertex lighting, affine texture mapping, vertex snapping
- [x] 3.6 Golden-frame harness with llvmpipe and tolerance compare; perf counters (draw calls, triangles)
- [x] 3.7 Unit tests for every pure renderer module (mesh, voxel, palette, font, procgen, view, glTF, textures); GL code covered by game tests

## 4. Input & controls (M1a)

- [x] 4.1 Action map with per-player device routing and remapping — with unit tests
- [x] 4.2 Gamepad hot-plug and aim assist — with unit tests on synthetic events
- [x] 4.3 Touch overlay with automatic switching (decision: drawn in-engine from SDL touch events, so it works on every platform; per-portal layouts via the input API)
- [ ] 4.4 WebXR JS library (moved to M2, progressive enhancement): session, eye matrices, controller poses; snap turn, teleport, vignette
- [x] 4.5 Playwright tests: hall renders, URL override, phone emulation touch mode (WebXR emulator entry follows 4.4)

## 5. Calendar & hub (M1b, target 29 Nov)

- [x] 5.1 Calendar unlock rule, season states, countdown, dev override — with unit tests per scenario
- [x] 5.2 Hall level: galleries, balconies, stairs, columns, apse, voxel tree with ornaments, wreath, banners, candles, snow (notice board, mirror, gramophone, fireplace still open)
- [x] 5.3 Portal states (sealed/still thawing/stars missing/thawing/open/mastered), ice reveal, swirling membranes, entry/return flow (render-to-texture previews still open)
- [x] 5.4 Progression: snowflakes, candles, ornaments, epilogue trigger — with unit tests
- [x] 5.5 Shining Star pickup/save, Star Arch sockets, portal 24 gate, tree crowning — with unit tests per scenario
- [x] 5.6 Friends config file, mark entities, Wall of Legends, content checker (≥ 2 marks per portal, ≥ 12 portals per friend) — with unit tests
- [x] 5.7 Hub walker bot and scenario scripts (reach portal 24 within 40 s, sealed portal blocks entry)

## 6. Portal framework & kits

- [x] 6.1 Portal descriptor, arena lifecycle, results/snowflakes, checkpoints, difficulty — with unit tests
- [x] 6.2 Scenario DSL (.gts) interpreter and headless runner — with unit tests
- [ ] 6.3 Replay recorder/player and cross-platform hash comparison job (state hash primitives exist)
- [x] 6.4 FPS kit: movement, collision with step-up, weapons with auto-aim, enemies with sight/chase/attack, projectiles with splash, pickups, key doors — with unit tests; shared test range portal stands in for unshipped days
- [ ] 6.5 Vehicle kit (Day 2) — with unit tests
- [ ] 6.6 RTS kit incl. lockstep-safe fixed-point combat (Day 3) — with unit tests
- [ ] 6.7 Platformer kit (Day 4) — with unit tests
- [ ] 6.8 RPG/Tactics kit (Day 10) — with unit tests

## 7. Release candidate (M1c, target 30 Nov)

- [ ] 7.1 OpenSpec changes add-portal-01-sleighed, add-portal-02-aurora-carpet, add-portal-03-frostfront
- [ ] 7.2 Packaging: Windows zip, Linux tar.gz + AppImage, web deploy to GitHub Pages
- [ ] 7.3 Originality review checklist template and LICENSE/notice bundling check

## 8. Multiplayer (optional, December)

- [ ] 8.1 Transport abstraction with loopback backend and network simulator — with unit tests
- [ ] 8.2 Lockstep with desync detection — two-sim hash tests
- [ ] 8.3 Snapshot replication with prediction/interpolation — lossy-link bot tests
- [ ] 8.4 WebRTC backend + signalling relay; room codes; shared hub and party portals

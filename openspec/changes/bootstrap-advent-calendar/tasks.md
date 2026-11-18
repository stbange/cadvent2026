# Tasks

## 1. Repository & quality infrastructure (M0, target 22 Nov)

- [ ] 1.1 CMake project with presets (linux-gcc, linux-clang, windows-msvc, windows-mingw, web, coverage, ci workflow)
- [ ] 1.2 Vendor SDL3, glad (GL 3.3 core), stb_image, cgltf, Unity into third_party/ with licence notices
- [ ] 1.3 Unity test runner + gcovr config with 95 % line / 85 % branch gate and explicit exclusion list
- [ ] 1.4 Scenario-to-test traceability script (parses openspec specs, matches test names); test it with fixtures
- [ ] 1.5 GitHub Actions: spec validation, Linux, Windows, Web, coverage, sanitizers, Pages deploy
- [ ] 1.6 Asset credits checker (every file in assets/ listed in assets/CREDITS.md); test with fixtures

## 2. Core & platform (M0–M1a)

- [ ] 2.1 core: arenas, deterministic math, PCG RNG, FNV-1a state hash, logging, config parsing — with unit tests
- [ ] 2.2 platform: SDL3 window, GL/GLES context, timing, prefs path, IDBFS mount on web
- [ ] 2.3 Fixed 60 Hz loop with interpolation; headless mode and accelerated speed flag — with unit tests
- [ ] 2.4 Save system (text format, versioned) — with unit tests for round-trip and corruption handling

## 3. Rendering (M1a)

- [ ] 3.1 GL device abstraction with recording null backend; shader preamble for GL 3.3 / GLES 3.0
- [ ] 3.2 Low-res FBO per view, nearest upscale, split-screen layouts (1–4) and stereo views
- [ ] 3.3 Clustered forward lights (≤ 32), directional shadow map, 4 dual-paraboloid shadowed lights
- [ ] 3.4 Meshes (glTF subset), 8-direction normal-mapped sprites, greedy-meshed .vox voxels
- [ ] 3.5 Post: palette 3D-LUT, Bayer dither, light bands, fog/distance darkness, flat/vertex/affine/snap options
- [ ] 3.6 Golden-frame harness with llvmpipe and tolerance compare; perf counters (draw calls, triangles)
- [ ] 3.7 Unit tests for every renderer module via the null backend

## 4. Input & controls (M1a)

- [ ] 4.1 Action map with per-player device routing and remapping — with unit tests
- [ ] 4.2 Gamepad hot-plug and aim assist — with unit tests on synthetic events
- [ ] 4.3 Web shell touch overlay with pointer-type auto switching and per-portal layouts
- [ ] 4.4 WebXR JS library: session, eye matrices, controller poses; snap turn, teleport, vignette
- [ ] 4.5 Playwright tests: desktop controls, phone emulation touch mode, WebXR emulator entry

## 5. Calendar & hub (M1b, target 29 Nov)

- [ ] 5.1 Calendar unlock rule, season states, countdown, dev override — with unit tests per scenario
- [ ] 5.2 Hall level (greybox → art): galleries, balcony, apse, tree, wreath, notice board
- [ ] 5.3 Portal states (sealed/thawing/open/mastered), render-to-texture previews, entry/return flow
- [ ] 5.4 Progression: stars, candles, ornaments, epilogue trigger — with unit tests
- [ ] 5.5 Hub walker bot and scenario scripts (reach portal 24 within 40 s, sealed portal blocks entry)

## 6. Portal framework & kits

- [ ] 6.1 Portal descriptor, arena lifecycle, results/stars, checkpoints, difficulty — with unit tests
- [ ] 6.2 Scenario DSL (.gts) interpreter and headless runner — with unit tests
- [ ] 6.3 Replay recorder/player and cross-platform hash comparison job
- [ ] 6.4 FPS kit (needed for Day 1) — with unit tests
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

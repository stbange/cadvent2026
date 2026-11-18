# 5 · Production Plan, Scope & Risks

## 5.1 Reality check

Today is 18 Nov 2026; Day 1 opens 1 Dec 2026. Twenty-four complete games
can't all be ready in 13 days. The calendar format solves this itself:
**a portal only needs to be ready on its day.** Plan for a rolling release:

* **Before 1 Dec (M0–M1):** engine core, hub, calendar, FPS kit, Day 1 and Day 2
  release-quality, Day 3 in progress. Web build live.
* **During December (M2):** about one portal per day, building on shared kits.
  The day order below is the user's order; kit maturity is the critical path
  (RTS kit needed by Day 3, Vehicle kit by Day 2, RPG kit by Day 10,
  Platformer by Day 4).
* A portal that slips stays *sealed with a "still thawing…" sign* and opens a few days
  late; the calendar keeps working. **Scope lever**: each portal design
  lists *Core* (must) vs *Garnish* (nice to have) features.

## 5.2 Milestones

| ID | Date (target) | Deliverable |
|---|---|---|
| M0 | 22 Nov | Repo skeleton, CMake presets, CI with coverage gate, Unity, OpenSpec bootstrap change approved, SDL3 window + GL triangle on all 3 targets |
| M1a | 26 Nov | Renderer v1 (low-res FBO, lights, 1 shadow, sprites, voxels, palette LUT), input & touch layer, save system |
| M1b | 29 Nov | Hub hall complete, calendar, portal switching, Day 1 portal (FPS kit), completion bot |
| M1c | 30 Nov | Release candidate, web deploy, Day 2 portal (vehicle kit), Day 3 (RTS kit) feature complete |
| M2 | 1–24 Dec | Daily portals, each with its OpenSpec change, tests, bot |
| M3 | 24 Dec | Epilogue, multiplayer hub (if done), final packaging |
| M4 | Jan 2027 | Boxing-Day marathon mode, polish, post-mortem |

## 5.3 Content production per portal (template, ~1–3 dev days with kits)

1. OpenSpec change `add-portal-NN-slug` (goal, rules, win/lose, stars).
2. Grey-box level in text map format (`.map` brush/grid text) — playable in 2 h.
3. Mechanic implementation + unit tests (TDD against scenarios).
4. Bot + scenario scripts; tune to 20 min human / par times.
5. Art pass (palette, sprites/voxels/meshes, lights), audio pass.
6. Touch layout + XR level check, golden frames, release.

## 5.4 Asset strategy

* Everything original, created for this project, **CC BY 4.0**, credited in
  `assets/CREDITS.md` (author, title, licence, source file).
* Low-res by design: 64×64 textures, 64-px sprites, ≤ 64³ voxel models keep
  production fast and downloads small.
* Pipelines: MagicaVoxel → `.vox`; Blender → glTF; Aseprite/LibreSprite → PNG
  sheets; tracker music → module file.
* Shared libraries of snow, ice, wood, stone, metal, neon materials across portals.

## 5.5 Risks

| Risk | Impact | Mitigation |
|---|---|---|
| Schedule (24 games) | high | kits, rolling release, Core/Garnish split, sealed-late fallback |
| 95 % coverage slowing gameplay iteration | medium | pure sim design, scenario-first tests, null GL backend |
| WebGL2 / mobile perf | medium | low-res FBO, budgets, per-portal perf smoke tests |
| WebXR fragmentation | medium | treat XR as progressive enhancement; tabletop mode for RTS |
| Float determinism wasm vs native | medium | fixed-point in lockstep sims, cross-platform replay tests |
| NAT traversal for online play | low | STUN only, room codes, LAN/split-screen fallback |
| IP / trademark | high if ignored | strict originality policy below |

## 5.6 Intellectual property policy

The portals are **homages to genres and mechanics**, not remakes:

* **No** original game names, logos, character names or likenesses, story,
  maps/level layouts, textures, sprites, models, sounds, voice lines or music.
* **Yes** to genre conventions and game mechanics (strafe-jumping, keycards,
  harvest-build-fight, 6DOF, jetpack skiing, ring menus, time units…), which
  are not protectable by copyright, re-expressed in our own winter-themed worlds.
* In-game titles are original. The *docs* name the inspirations for
  orientation, and the hub's notice board may mention them as "fond memories of
  1996" — nominative reference only, no logos.
* The project is free and non-commercial; still, each portal gets a short
  originality review before release (does anything look/sound like a copy?).

## 5.7 Licences

* **Code**: MIT (`LICENSE`).
* **Art, audio, maps, text content in `assets/`**: CC BY 4.0
  (`LICENSE-ASSETS.md`); attribution string: *"cAdvent 2026 contributors, CC BY 4.0"*.
* Third-party code keeps its licences (SDL3: zlib; stb: MIT/PD; cgltf: MIT;
  Unity: MIT; glad: MIT/PD-generated).

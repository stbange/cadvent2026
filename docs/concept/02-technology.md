# 2 · Technology: Engine, Rendering, Platforms, Controls

## 2.1 Stack

| Layer | Choice | Notes |
|---|---|---|
| Language | **C17** (no C++), warnings-as-errors | `-std=c17 -Wall -Wextra -Wpedantic -Werror`, MSVC `/std:c17 /W4 /WX` |
| Platform | **SDL3** (3.2+) | window, GL context, input, gamepad, touch, audio streams, filesystem, time |
| Graphics | **OpenGL 3.3 Core** (desktop) / **OpenGL ES 3.0 = WebGL 2** (web) | one shader source, version preamble injected at load |
| GL loader | glad2 (generated, vendored) | desktop only; emscripten provides GLES3 |
| Images | stb_image (PNG) | assets are tiny; no compressed textures needed |
| Models | glTF 2.0 subset via cgltf; MagicaVoxel `.vox` via own loader | |
| Audio | SDL3 audio stream + own mixer + tiny tracker/synth for music | music composed in an open tracker format, CC BY 4.0 |
| Build | **CMake ≥ 3.25** + presets | `linux-gcc`, `linux-clang`, `windows-msvc`, `windows-mingw`, `web`, `coverage` |
| Web | **Emscripten** (latest 3.x/4.x), `-sUSE_SDL=3`, WebGL2, IDBFS | single-page `index.html` shell with touch layer |
| XR | WebXR through a thin JS library (`--js-library webxr.js`) | immersive-vr session, stereo render loop |
| Tests | **Unity** (ThrowTheSwitch) + **gcovr/lcov** | see [04-quality-and-testing.md](04-quality-and-testing.md) |
| Spec | **OpenSpec** (`openspec/`) | spec-driven workflow |

Vendored third-party code lives in `third_party/` with its own licences
(zlib/MIT/public domain) and is excluded from the coverage gate.

## 2.2 Repository layout (target)

```
cadvent2026/
├─ CMakeLists.txt, CMakePresets.json
├─ src/
│  ├─ core/       arena allocators, math (fixed det. math), rng, log, hash, config
│  ├─ platform/   SDL3 window/context/time/fs — the only code that includes SDL
│  ├─ gfx/        GL abstraction, shaders, lights, shadows, sprites, voxels, post
│  ├─ audio/      mixer, synth, tracker player, 3D panning
│  ├─ input/      action maps, devices (kbm, pad, touch, XR), per-player routing
│  ├─ sim/        entity store, physics, collision, nav (grid A*, flow fields)
│  ├─ ui/         bitmap font, retro HUD widgets, radial menu, touch overlay
│  ├─ net/        transport abstraction, lockstep, snapshot replication
│  ├─ kits/       fps/, rts/, vehicle/, rpg/, platformer/   (reusable game logic)
│  ├─ hub/        the Hall, calendar, progression, save
│  ├─ portals/    p01_*/ … p24_*/   (one module per day)
│  └─ main.c      entry point, game loop, module switching
├─ assets/        CC BY 4.0 art, audio, maps (text formats where possible)
├─ tools/         asset baker, map compiler, palette tool (C, also tested)
├─ tests/
│  ├─ unit/       Unity tests mirroring src/
│  ├─ game/       scripted scenario tests & completion bots
│  ├─ golden/     reference frames for render tests
│  └─ web/        Playwright tests (touch layout, WebXR emulator smoke)
├─ web/           emscripten shell html/js/css, webxr.js
├─ openspec/      specs and change proposals
└─ docs/concept/  this concept
```

## 2.3 Architecture

### Deterministic simulation, decoupled rendering

```
 ┌──────────┐  actions   ┌──────────────────────────┐ snapshot ┌──────────┐
 │  input   │──────────▶│ sim (fixed 60 Hz, pure C) │────────▶│  gfx     │──▶ screen(s)
 │ devices  │            │ portal->update(world,in)  │ (interp) │  audio   │
 └──────────┘            └──────────────────────────┘          └──────────┘
        ▲                         ▲      │ state hash
        │                         │      ▼
        └──── net (lockstep / snapshots) ─┘     tests / replays / bots
```

* **Simulation** runs at a fixed 60 Hz step and never calls SDL or GL. It
  consumes a per-tick `cadv_input_frame` (actions for up to 4 players) and
  produces state. This makes it unit-testable, replayable, bot-drivable and
  network-friendly.
* **Determinism**: integer/fixed-point where it matters (RTS lockstep), our
  own `cmath`-free trig/sqrt implementations, `-ffp-contract=off`, no `-ffast-math`,
  seeded `pcg32` RNG per world. A **state hash** (FNV-1a over canonical state) is
  exposed for tests and desync detection.
* **Rendering** interpolates between the two most recent sim states.

### Portal module interface

```c
typedef struct cadv_portal_desc {
    uint8_t     day;              /* 1..24 */
    const char *id;               /* "p07_tinsel_strike" */
    const char *title;
    uint32_t    caps;             /* CADV_CAP_COOP | CADV_CAP_VERSUS | CADV_CAP_XR | ... */
    uint8_t     max_local_players, max_net_players;
    size_t      state_size;       /* the whole game state lives in one arena block */
    bool (*init)     (cadv_world *w, const cadv_portal_cfg *cfg);
    void (*tick)     (cadv_world *w, const cadv_input_frame *in);  /* pure sim */
    void (*render)   (const cadv_world *w, cadv_view *views, int nviews, float alpha);
    void (*audio)    (const cadv_world *w, cadv_audio *a);
    cadv_result (*result)(const cadv_world *w);                   /* running/won/lost + stars */
    uint64_t (*hash) (const cadv_world *w);
    void (*bot)      (const cadv_world *w, int player, cadv_actions *out); /* test/AI driver */
} cadv_portal_desc;
```

The hub is itself just "portal 0". Switching games frees the whole
per-portal arena — no leaks across portals by construction.

### Gameplay kits

| Kit | Provides | Used by days |
|---|---|---|
| **FPS** | capsule movement (strafe-jump, crouch, swim, ladders), hitscan & projectiles, weapon state machines with alt-fire, pickups, doors/keys, triggers & scripted sequences, sprite & mesh enemy AI (patrol/chase/attack/flee, hearing), secrets | 1, 7, 8, 13, 16, 17, 19, 20, 22 |
| **RTS** | first-person commander body, wrist-tablet build UI, construction placement beam, resource nodes & harvesters, unit orders (point-command, group hotkeys, rally), fog of war, flow-field pathing, deterministic lockstep combat, AI opponent scripts | 3, 5, 18, 21, 23, 24 |
| **Vehicle** | 6DOF flight, hover/ski physics, walker locomotion with torso twist, heat model, heightmap & voxel terrain, terrain deformation | 2, 6, 9, 11 |
| **RPG / Tactics** | stats, inventory grid, loot tables, party followers, radial ring menu, dialogue, turn-based time-unit system | 10, 12, 14, 15 |
| **Platformer** | grid-snapped traversal (ledge grab, shimmy, block push, swan dive), breath meter | 4 |

## 2.4 Rendering — "low-res, high-mood"

### Pipeline

1. **Scene pass** into a low-resolution FBO (default 400×225 for 16:9, user
   choice 320×180 … 640×360; per-portal default — e.g. Doom-homage at 320×200
   aspect-corrected).
2. **Shadow passes**: one cascaded-lite directional shadow map (2 cascades,
   512²) + up to **4 shadow-casting point/spot lights** per view using
   dual-paraboloid maps (WebGL2-friendly, no cube-array needed). Other lights
   (up to 32 per view, clustered on a coarse 8×4×8 grid) are unshadowed.
3. **Forward lighting**: Lambert + optional Blinn specular, quantised into
   *N light bands* (configurable) for a hand-painted 90s look; per-vertex
   lighting toggle for the oldest-looking portals.
4. **Fog & atmosphere**: linear/exp fog, height fog, coloured "distance
   darkness" (Doom/Build-style light diminishing).
5. **Post**: palette quantisation via a 32³ **3D-LUT built from each portal's
   256-colour palette**, optional 4×4 Bayer dither, scanlines/CRT curvature
   toggle, then **nearest-neighbour upscale** to the window.

### Era emulation knobs (per portal)

| Knob | Effect | Typical users |
|---|---|---|
| `light_bands` | stepped light falloff | Doom, Duke, Blood homages |
| `affine_uv` / `vertex_snap` | wobbly PS1-style textures | Tomb, Secret-of-Mana homages |
| `palette` | 256-colour LUT | nearly all |
| `colored_light` | allow saturated coloured lights | Unreal, Quake homages show off |
| `flat_shading` | per-face normals | MechWarrior, Descent, TA homages |
| `sprite_dirs` | 1/5/8-direction billboards | all sprite-heavy games |

### Assets: three kinds of geometry

* **3D meshes** (glTF, ≤ 1 500 tris per character, 64×64…128×128 textures):
  levels, vehicles, mechs, buildings.
* **Sprites**: billboarded, 8-direction sheets authored at 64 px height, with a
  **normal map** baked from a voxel or hand-painted height so sprites *receive
  dynamic light* and cast shadow via alpha-tested shadow pass.
* **Voxels** (MagicaVoxel `.vox`): greedy-meshed at load, used for the hub tree,
  ornaments, pickups, destructible props, Magic-Carpet-homage terrain features and
  RTS units (small voxel models read well from a first-person commander height).

### Budgets (to hold 60 fps on a mid-range phone in WebGL2)

* ≤ 150 draw calls per view (instancing for sprites/units, texture atlases).
* ≤ 120 k triangles per view, ≤ 32 dynamic lights, ≤ 4 shadowed.
* Split screen: the low-res target is *per view*; 4 views at 320×180 cost about
  one 640×360 frame.

## 2.5 Platforms

| Target | Toolchain | Packaging | Notes |
|---|---|---|---|
| Windows x64 | MSVC 2022 and MinGW-w64 | zip with `cadvent.exe`, `SDL3.dll`, `assets/` | |
| Linux x64 | GCC 13+/Clang 17+ | tar.gz + AppImage | Wayland & X11 via SDL3 |
| Web | Emscripten | static site (GitHub Pages) `index.html`, `.wasm`, `.data` | WebGL2 required; assets preloaded per portal lazily via fetch to keep first load < 10 MB |
| WebXR | same web build | "Enter VR" button when `navigator.xr` supports `immersive-vr` | see below |

macOS is not a target (OpenGL deprecated there) but is expected to build.

## 2.6 WebXR

* `web/webxr.js` exposes `xr_is_supported`, `xr_request_session`, and per-frame
  view/projection matrices + controller poses to C via `EM_JS`/js-library.
* In XR the engine renders two views (one per eye) into the XR framebuffer at a
  low-res per-eye target (e.g. 480×480) and upscales — the retro look hides the
  low resolution and keeps frame times sane on standalone headsets.
* **Comfort**: snap-turn (30°), teleport or smooth locomotion, vignette on
  acceleration, seated mode. Cockpit/vehicle portals are naturally comfortable.
* **XR support levels** (declared per portal through `CADV_CAP_XR_*`):
  * **Full** — designed for hands: hub, 6 (mech cockpit), 11 (6DOF ship),
    17 (energy blade), 7, 8, 13, 16, 19.
  * **Seated** — playable with controllers as gamepad + head look: 1, 2, 4, 9,
    10, 12, 14, 20, 22.
  * **Tabletop** — RTS and tactics portals offer a "war-table" view in XR where
    the battlefield becomes a miniature you lean over, while non-XR players
    keep the commander-on-the-ground view: 3, 5, 15, 18, 21, 23, 24.

## 2.7 Input & controls

### Action mapping

All gameplay reads **abstract actions** (`MOVE_X/Y`, `LOOK_X/Y`, `FIRE`,
`ALT_FIRE`, `JUMP`, `CROUCH`, `USE`, `RADIAL`, `CMD_SELECT`, `CMD_ORDER`, …),
never devices. Bindings are data-driven and remappable per player.

| Device | Default scheme |
|---|---|
| Keyboard + mouse | WASD, mouse look, LMB/RMB fire/alt, Space jump, C crouch, E use, Q radial, 1–9 weapons/groups, Tab map |
| Gamepad | twin-stick, triggers fire/alt, A jump, B crouch, X use, Y radial, bumpers cycle, light aim-assist (toggle) |
| **Touch** (auto) | left virtual stick (floating), right half = drag-to-look, context buttons (fire, jump, use) bottom-right, radial button; RTS portals add *tap-to-command* and pinch to zoom the tablet map |
| XR controllers | left stick move, right snap-turn, triggers fire, grips grab/use, point-to-command in RTS |

### Mobile detection & touch switching

* The web shell switches to touch UI when `matchMedia('(pointer: coarse)')`
  matches **or** the first input event is a touch event; it switches back when
  a mouse move/keypress or gamepad is detected (no hard device sniffing).
* Orientation: landscape lock request; a friendly "rotate your device" card in portrait.
* Pointer lock on desktop web after the first click; fullscreen button.
* Touch layouts are **per-portal data** (`assets/portals/pNN/touch.layout`)
  so the Mech portal can have a throttle slider, the Mana portal a ring-menu
  button, etc.

### Accessibility

Remappable everything, hold-to-toggle options, subtitles for all voice lines,
colour-blind-safe palettes for team/key colours, FOV slider, motion-blur/
screen-shake off switches, "story difficulty" in every portal (enemies deal
less damage, generous checkpoints).

## 2.8 Audio

* All music and SFX are **original** and licensed CC BY 4.0. Music is written
  in the *style* of the era (MIDI/tracker/FM/CD-audio vibe) for each genre but
  never imitates specific melodies.
* Runtime synth + module player keeps downloads tiny; positional audio with
  simple occlusion (ray test against level geometry) and reverb zones.

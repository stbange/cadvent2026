# cAdvent 2026

A first-person 3D advent calendar for retro gamers. Walk through a solemn,
candle-lit festive hall with **24 portals**. From **1 to 24 December 2026** one
more portal opens each day, and behind each one waits a ~20-minute original game
that celebrates a classic PC/console game from about 25 years ago, re-imagined in
first person with low-res retro 3D, dynamic lighting and shadows.
Collect the **Shining Star** hidden in each of the first 23 portals and set them
into the Star Arch to open the 24th on Christmas Eve, and watch out for the
marks the crew left everywhere: *"Dr.Doom did this"*, *"Bloodmaker was here"*,
*"Beware of Messerjocke"*…

* **Tech:** C17 · SDL3 · OpenGL 3.3 / WebGL 2 · Emscripten · WebXR
* **Targets:** Windows, Linux, Web (touch controls on mobile), WebXR
* **Process:** [OpenSpec](openspec/) spec-driven development, ≥ 95 % unit-test coverage, automated game tests and completion bots
* **Multiplayer (optional):** split screen and online via WebRTC

## Plan

**[docs/PLAN.md](docs/PLAN.md)**: milestones, the pre-season sprint, the December
production schedule for all 24 portals, definition of done and fallbacks.

## Concept

Start with **[docs/concept/README.md](docs/concept/README.md)**: vision & hub,
technology, multiplayer, quality, production plan and all 24 portal designs.

Current OpenSpec change: [`bootstrap-advent-calendar`](openspec/changes/bootstrap-advent-calendar/proposal.md)

```bash
openspec validate --strict
```

## Building

Requirements: CMake ≥ 3.25, Ninja, a C17 compiler (GCC 13+, Clang 17+ or MSVC 2022).
SDL3 3.4.20 is downloaded and built automatically.

**Linux**

```bash
./scripts/install-linux-deps.sh
```

```bash
cmake --preset linux-gcc
```

```bash
cmake --build --preset linux-gcc
```

```bash
ctest --preset linux-gcc
```

```bash
./build/linux-gcc/cadvent
```

Controls: WASD/arrows + mouse (click to capture the mouse), Space jump,
C crouch, Shift run, E use, mouse buttons fire, 1–5 weapons, Esc leave a
portal. Gamepads and touch screens work too (touch controls appear
automatically).

Useful flags (see `--help`): `--date 2026-12-05` pretends it is that day,
`--unlock-all` opens every portal (development; does not touch your real
save), `--portal N` starts inside a portal, `--script test.gts --headless`
runs a scenario without a window, `--screenshot out.ppm --frames N` captures a
frame. Rendering without a display works with `SDL_VIDEO_DRIVER=offscreen`
(Mesa llvmpipe). In the browser, add `?dev=unlockall`, `?date=2026-12-05` or
`?portal=3` to the URL.

Golden frames live in `tests/golden/`; after an intended visual change,
regenerate them with `CADV_UPDATE_GOLDEN=1 ctest --preset linux-gcc -R golden`.

**Local CI (coverage gate: ≥ 95 % lines, ≥ 85 % branches)**

```bash
cmake --workflow --preset ci
```

**Windows**: `cmake --preset windows-msvc`, then `cmake --build --preset windows-msvc`.
From Linux: `cmake --preset mingw-cross && cmake --build --preset mingw-cross`.

**Web** (with an activated [emsdk](https://emscripten.org) 6.0.13):

```bash
cmake --preset web && cmake --build --preset web
```

```bash
python3 -m http.server --directory build/web 8000
```

Then open <http://localhost:8000/cadvent.html>. Browser tests: `cd tests/web && npm ci && npx playwright test`.

## Licence

* Code: [MIT](LICENSE)
* Art, audio, maps & content in `assets/`: [CC BY 4.0](LICENSE-ASSETS.md)

All portal games are original works inspired by genres and mechanics. They
contain no names, characters, art, levels, sounds or music from the games that
inspired them. Those titles belong to their respective owners.

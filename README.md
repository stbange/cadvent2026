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

**F1** (or H, the gamepad's Back button, or the "?" touch button) opens the
on-screen help for wherever you are: the goal, the controls and tips; the
game pauses while it is open.

Controls: WASD/arrows + mouse (click to capture the mouse), Space jump,
C crouch, Shift run, E use, mouse buttons fire (right button: alt-fire or
claim), 1–6 weapons or spells, Esc leave a portal. On the carpet (Day 2),
Space climbs and C sinks. As commander (Day 3): Q or the mouse wheel picks a
tablet item and E buys it; a finished structure is placed by aiming and
firing; fire picks a unit (Shift adds), right button sweep-selects, then fire
on ground or an enemy orders the squad (Shift = plain move); C clears the
squad, Shift+1–4 sets and 1–4 recalls groups, 5 selects the army, 6 the
engineers, Space launches the recon drone, Tab opens the map. With no squad
selected, fire uses the sidearm. In the ice tomb (Day 4): Shift runs (a
running jump clears two tiles), hold E in the air to catch ledges, E with
forward/back pushes or pulls blocks, C lets go or dives. As the Captain of
Kindlewick (Day 5) the stave works like Day 3's tablet (Q/wheel, E, fire to
place, pick and order; 6 selects workers); fire on a tree, the mine or the
tallow platform sends selected workers there, fire on your ferry boards the
squad, Space boards or leaves the ferry, and with no squad fire is a shield
bash. In the walker cockpit (Day 6): W/S set the throttle, A/D turn the legs,
the mouse twists the torso and aims, the left button fires the selected
weapon group (1-3, 4 toggles chain fire), the right button fires everything,
Space fires the jump jets, C is all stop, Shift centres the torso and E asks
Santa Claude for a coolant drop; watch the heat gauge. Gamepads and touch
screens work too (touch controls appear automatically).

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

**Release packages**: pushing a tag `vX.Y.Z` runs `.github/workflows/release.yml`,
which builds self-contained archives (Linux `tar.gz`, Windows `zip`, web `zip`)
and attaches them to a GitHub release. Locally, any build can be packaged with

```bash
cmake -DEXE=build/linux-release/cadvent -DSRC=. -DOUT=dist/cadvent -DARCHIVE=dist/cadvent.tar.gz -P cmake/CadvPackage.cmake
```

The executable finds `assets/` next to itself, so the extracted folder runs anywhere.

**GitHub Pages** (playable at <https://stbange.github.io/cadvent2026/play/>):
Pages publishes the `docs/` folder of `main` (Settings → Pages → Deploy from a
branch → `main` / `/docs`). To update the online game, rebuild and copy the web
build into `docs/play/`, then commit and push:

```bash
cmake --preset web && cmake --build --preset web --target publish-web
```

## Licence

* Code: [MIT](LICENSE)
* Art, audio, maps & content in `assets/`: [CC BY 4.0](LICENSE-ASSETS.md)

All portal games are original works inspired by genres and mechanics. They
contain no names, characters, art, levels, sounds or music from the games that
inspired them. Those titles belong to their respective owners.

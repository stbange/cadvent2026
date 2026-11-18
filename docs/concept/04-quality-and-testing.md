# 4 · Quality: OpenSpec, Unit Tests (≥ 95 %), Automated Game Tests, CI

## 4.1 Spec-driven workflow with OpenSpec

* Every behaviour starts as an **OpenSpec change** in `openspec/changes/<id>/`
  (`proposal.md`, `design.md`, `tasks.md`, delta `specs/`).
* Requirements use `### Requirement:` with at least one `#### Scenario:`
  (WHEN/THEN). **Every scenario maps to at least one automated test**; the
  test name carries the scenario slug (e.g. `test_calendar__portal_sealed_before_its_day`),
  and a CI script checks that every scenario has a matching test.
* When a change is implemented and green, it is archived
  (`openspec archive <id>`) which merges the deltas into `openspec/specs/`.
* `openspec validate --strict` runs in CI.

Initial change: [`openspec/changes/bootstrap-advent-calendar`](../../openspec/changes/bootstrap-advent-calendar/proposal.md).
Each portal gets its own change `add-portal-NN-<slug>` before its implementation starts.

## 4.2 Unit tests and the 95 % gate

* Framework: **Unity** (single C file, MIT) with a tiny runner generator; tests
  in `tests/unit/` mirror `src/`.
* Coverage: `gcc --coverage` builds (`coverage` CMake preset), measured with
  **gcovr**; CI fails if **line coverage < 95 %** *or* **branch coverage < 85 %**
  over `src/` and `tools/`.
* Exclusions are explicit and minimal: `third_party/`, and the
  *platform shim* (`src/platform/sdl_*.c`, `src/main.c`, web JS glue) which is
  kept under ~3 % of the codebase and covered by the game/smoke tests instead.
  Exclusions are listed in `gcovr.cfg` and reviewed like code.
* How 95 % stays realistic for a game:
  * Simulation is pure C with no SDL/GL dependency → directly testable.
  * GL is behind `gfx_device` with a **recording null backend** for unit tests
    (asserts on emitted draw calls/state) plus a real backend tested headless
    via **Mesa llvmpipe / EGL surfaceless** in CI.
  * Audio mixing is tested on sample buffers; input mapping on synthetic event streams.
  * Portal logic is tested per mechanic (e.g. "heat above 100 % triggers shutdown").
* Sanitizers: ASan + UBSan job; Valgrind-free memcheck via ASan; `-fanalyzer`
  and clang-tidy as non-blocking reports initially, blocking from Day 1 release.

## 4.3 Automated game tests

Run headless: `cadvent --headless --portal 7 --script tests/game/p07/round_win.gts --speed max`.

| Test type | What it proves | How |
|---|---|---|
| **Scenario scripts** (`.gts`) | specific mechanics in the real game loop | tiny line-based DSL: `spawn`, `press FIRE 10`, `wait_until enemy_count==0`, `assert player.health>50`, `expect_event portal_complete` |
| **Completion bots** | every portal can be finished, at all difficulties | each portal implements `bot()` (waypoint graph + goal planner); CI asserts `result==WON` within a tick budget, and records the *simulated play time* to check the 15–25 min target is plausible (bots are tuned to play at ~human speed in "pace" mode) |
| **Determinism / replay** | same seed + inputs ⇒ same state hash | replays recorded once, verified on Linux, Windows **and** wasm (Node) — catches float divergence across compilers |
| **Golden frames** | rendering didn't regress | at fixed ticks capture the low-res frame (llvmpipe), compare with tolerance (per-pixel ΔE, ≤ 0.5 % differing pixels) |
| **Multiplayer loopback** | lockstep has no desync, snapshot code converges | 2–4 sims in one process over the loopback transport with a lossy network simulator |
| **Calendar tests** | unlock rules incl. time zones, before/after season | injected clock |
| **Web E2E** (Playwright, Chromium) | page loads, touch UI appears on mobile emulation, keyboard/mouse UI on desktop, entering a portal works, WebXR "Enter VR" flow with the Immersive Web Emulator polyfill | `tests/web/*.spec.ts` |
| **Performance smoke** | budgets hold | headless frame-time stats on fixed camera paths; warn at 80 % of budget |

## 4.4 CI/CD (GitHub Actions)

```
on: push to main, pull_request
jobs:
  spec:       openspec validate --strict; scenario↔test mapping check
  linux:      gcc + clang, Debug/Release, unit + game tests (llvmpipe)
  coverage:   gcovr gate (95 % lines / 85 % branches), HTML report artefact
  sanitize:   ASan/UBSan unit + game tests
  windows:    MSVC + MinGW build, unit tests, determinism replays
  web:        emsdk build, Node-run unit tests (wasm), Playwright E2E
  release:    (tags / main) package zip, tar.gz, AppImage; deploy web build to GitHub Pages
```

Branching: per project owner's request all work lands on **`main`**, kept
green by running the full CI locally (`cmake --workflow --preset ci`) before
pushing; feature flags (`CADV_PORTAL_NN_ENABLED`) hide unfinished portals.

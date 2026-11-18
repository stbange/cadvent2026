# cAdvent 2026 — Master Plan

The single place that answers *what gets built, in which order, by when, and
how we know it's done*. Design details live in [docs/concept/](concept/README.md);
requirements live in [openspec/](../openspec/changes/bootstrap-advent-calendar/proposal.md).

Status on **18 Nov 2026**: **M0 done** (build, CI, coverage gate, SDL3/GL
triangle on Linux, Windows and Web). 13 days until Day 1.

---

## 1. The shape of the plan

```
 Nov 18 ─ M0 ✔ ── M1a ✔ ── M1b ✔ ── M1c ── Dec 1 ────── daily portals ────── Dec 24 ── Jan
          skeleton  renderer  hall +   Day 1–3   ▲ one portal ships every evening    epilogue  marathon
                    input     calendar ready     │ for the next morning              + MP      polish
                    assets    FPS kit            └ work runs 2–4 days ahead
```

Three rules make 24 games in 24 days possible:

1. **A portal only has to exist on its day.** Production runs a few days ahead
   of the calendar, never all at once.
2. **Kits before portals.** Each new genre needs its gameplay kit first; the
   kit's first portal takes longest, the rest reuse it (§4).
3. **Core first, garnish if time allows.** Every portal has a must-ship *Core*
   (listed in §5). If a portal is late, it opens with a "still thawing…" sign
   for a day or two; the calendar keeps working (§8).

---

## 2. Milestones

| ID | Window | Goal | Exit criteria |
|---|---|---|---|
| **M0** ✔ | 18 Nov | Skeleton | presets, CI, coverage gate ≥ 95/85, triangle on all targets |
| **M1a** ✔ | 19–22 Nov | Engine foundation | low-res renderer with lights + shadows, sprites, voxels, meshes, palette post; input + touch; asset loading; deterministic math; save; audio mixer |
| **M1b** ✔ | 23–26 Nov | The Hall | walkable hall, calendar, portal states, portal framework, Star Arch, Wall of Legends, FPS kit |
| **M1c** | 27–30 Nov | Release candidate | Day 1 shippable, Day 2 nearly done, Day 3 started; web build live on Pages; packaged Windows/Linux builds — *status 18 Nov: Days 1 and 2 playable end to end (bot-verified), RTS kit slice 1 done, release workflow ready; open: human pace tuning, Day 3 portal, Pages switch-on* |
| **M2** | 1–24 Dec | Daily portals | each portal ships by the evening before its day (§5) |
| **M3** | 24 Dec | Christmas Eve | Day 24 + epilogue; multiplayer hub if ready (§6) |
| **M4** | Jan 2027 | Boxing Day | marathon mode, speed-run timers, bug fixes, post-mortem |

---

## 3. Pre-season sprint (19–30 Nov), day by day

| Date | Focus | Deliverables (each with tests) |
|---|---|---|
| Thu 19 Nov | Rendering 1 | GL device layer + recording null backend; low-res FBO + nearest upscale; camera + mesh pipeline; stb_image / cgltf vendored |
| Fri 20 Nov | Rendering 2 | forward lights (≤ 32, clustered), directional shadow map, first dual-paraboloid point shadow; golden-frame harness |
| Sat 21 Nov | Rendering 3 | 8-dir normal-mapped sprites, `.vox` loader + greedy meshing, palette LUT + dither + light bands + fog |
| Sun 22 Nov | Input & platform | action maps, gamepad, web touch overlay with auto switching, timing, prefs/IDBFS saves, audio mixer + tiny synth |
| Mon 23 Nov | Framework | portal descriptor + arena lifecycle, results/snowflakes, checkpoints, scenario DSL (`.gts`), replay + state hash |
| Tue 24 Nov | Hall 1 | grey-box hall (galleries, balcony, apse), first-person controller, calendar unlock rule + countdown |
| Wed 25 Nov | Hall 2 | portal states + previews, entry/return transition, Shining Star pickup/socket/gate logic, Wall of Legends, friends config |
| Thu 26 Nov | FPS kit | movement (strafe, crouch, swim, ladders), weapons with alt-fire, pickups, doors/keys, sprite enemy AI, secrets |
| Fri 27 Nov | Day 1 | *Sleighed!* grey-box → playable; bot; Hall art pass |
| Sat 28 Nov | Day 1 + Vehicle kit | Day 1 art/audio/touch layout; vehicle kit (flight, heightmap terrain + deformation) |
| Sun 29 Nov | Day 2 | *Aurora Carpet* playable; bot |
| Mon 30 Nov | **RC** | Day 1 final, Day 2 polish, RTS kit started; Pages live; release packages; teaser mode tested |

The hall and Day 1 are the release-critical path. If M1a slips, cut in this
order: point-light shadows (keep directional), voxels in the hall (use meshes),
audio synth (use short samples).

---

## 4. Gameplay kits — when each one must be ready

| Kit | Must be ready | First portal | Later portals (reuse) | Effort |
|---|---|---|---|---|
| **FPS** | 26 Nov | Day 1 | 7, 8, 13, 16, 17, 19, 20, 22 | large, front-loaded |
| **Vehicle** (flight, terrain, walker) | 28 Nov | Day 2 | 6 (walker), 9 (ski/jet), 11 (6DOF), 20 (cyberspace) | medium |
| **RTS** (commander, build, harvest, orders, lockstep) | 1 Dec | Day 3 | 5, 18, 21, 23, 24 | **largest**; Day 3 is the riskiest early day |
| **Platformer** (grid traversal) | 2 Dec | Day 4 | — | small |
| **RPG** (stats, loot, inventory, party, ring menu) | 8 Dec | Day 10 | 12, 14 | medium |
| **Tactics** (turn-based TUs, LOS) | 12 Dec | Day 15 | — | medium (built on RPG + RTS grid) |
| **6DOF extension** | 9 Dec | Day 11 | 20 | small |

---

## 5. December production schedule

*Build window* = when the portal is actively built. *Ship* = the build goes
live the evening before its day (target 20:00 CET), so it is ready at local
midnight. Each portal ships its **Core** list; everything else in its concept
section is garnish.

| Day | Portal | Kit | Build window | Ship | Core (must ship) |
|---|---|---|---|---|---|
| 1 | Sleighed! | FPS | 27–29 Nov | 30 Nov | 3 zones, 5 weapons incl. gift-bomb + shrinkray, vending/arcade props, jetpack, boss, 6 secrets, Shining Star from the Siphon, friend marks |
| 2 | Aurora Carpet | Vehicle | 28–30 Nov | 1 Dec | flying, terrain deformation, mana orbs + balloons, castle growth, rival wizard AI, 4 spells, 75 % win, star at the aurora |
| 3 | Frostfront | RTS | 30 Nov–2 Dec | 2 Dec | commander on foot, wrist tablet, 1 resource + power, 5 units, 5 buildings, AI waves, HQ destroy, star from the vault |
| 4 | Ice Tomb of the Winter King | Platformer | 1–3 Dec | 3 Dec | grid jumps + ledge grab, block puzzle, timed door, swimming + air, wolves, colossus chase, 3 gears → star |
| 5 | Hearthguard | RTS | 2–4 Dec | 4 Dec | 3 resources, workers, farms, 5 units, 3 raids, ferry across the lake, Longhall, star in the fire |
| 6 | Iron Yule | Vehicle | 3–5 Dec | 5 Dec | cockpit, torso twist, heat, 3 weapon groups, component damage, **Santa Claude** escort + star, 3 missions |
| 7 | Tinsel Strike | FPS | 4–6 Dec | 6 Dec | 1 map, 2 sites, buy menu + economy, 5 weapons, smoke/flash, 5v5 bots named after the crew, best-of-9 |
| 8 | Cascade | FPS | 5–7 Dec | 7 Dec | monorail intro, cascade set piece, follower NPCs, squad soldiers, antenna finale, no cutscenes |
| 9 | Skifrost | Vehicle+FPS | 6–8 Dec | 8 Dec | skiing + jetpack physics, disc launcher, CTF vs bots, generator/turrets, trophy star |
| 10 | Neon Syndic | RPG+FPS | 7–9 Dec | 9 Dec | 3 AI agents + orders, Charm Beacon crowd, overclock, 3 relays, extraction, neon night lighting |
| 11 | Deep Thaw | Vehicle (6DOF) | 8–10 Dec | 10 Dec | 6DOF flight, 5 robot types, 3 keys, hostages, reactor + escape timer, flares |
| 12 | Below the Chapel | RPG | 9–11 Dec | 11 Dec | village, 4 generated levels, loot affixes, belt, 3 classes, Pale Bishop |
| 13 | Hellfrost Station | FPS | 10–12 Dec | 12 Dec | 3 compact maps, keycards, barrels, infighting, 6 enemy types, tally screen |
| 14 | Ring of the Evergreen | RPG | 11–13 Dec | 13 Dec | ring menu, charge attacks, 2 AI party members, 3 shrines, Hollow Stag |
| 15 | Frostfall Protocol | Tactics | 12–14 Dec | 14 Dec | turn-based first person, TUs, reaction fire, crew as soldiers, 2 missions |
| 16 | Rune of Rime | FPS | 13–15 Dec | 15 Dec | hub + 4 short realms, rocket jump, 6 enemies, Old Cold boss |
| 17 | Way of the Glowblade | FPS+melee | 14–16 Dec | 16 Dec | directional blade, deflection, 4 powers, Dawn/Dusk meter, Vesh duel |
| 18 | Hivemind | RTS | 15–17 Dec | 17 Dec | hive-eye, creep, larvae morphing, pheromone orders, Bastion |
| 19 | Skyfall Isle | FPS | 16–18 Dec | 18 Dec | coloured-light showcase, 5 weapons with alt-fire, dodging Varkh, shuttle |
| 20 | Ghost in the Orbiter | FPS+6DOF | 17–19 Dec | 19 Dec | 3 decks, audio logs, keypad code, security level, cyberspace finale |
| 21 | Ages of Midwinter | RTS | 18–20 Dec | 20 Dec | 4 resources, 3 ages, villagers, relics, Monolith wonder win |
| 22 | Grave Tidings | FPS | 19–21 Dec | 21 Dec | 3 areas, fire spreading, dynamite modes, hex doll, bell boss |
| 23 | Total Thaw | RTS+Vehicle | 20–22 Dec | 22 Dec | Commander in first person, streaming economy, reclaim, ballistics, enemy Commander |
| 24 | Polar Alert | RTS (full) | **17–23 Dec** | 23 Dec | 2 factions, naval, arc pylons vs Warp Lantern, Star Beacon, then the epilogue |

The finale overlaps the last week on purpose: it reuses the RTS kit but adds
naval units and two factions, and it carries the epilogue.

### Daily rhythm in December

| When | What |
|---|---|
| morning | play the portal that opened today; triage bug reports from friends |
| day | build the portal due in 2–3 days (§5 window) |
| afternoon | finish tomorrow's portal: bot green, art/audio pass, touch layout, originality review |
| ~20:00 CET | ship: tag `day-NN`, CI publishes web + desktop builds |
| any day | hotfix yesterday's portal if needed (it's already live for everyone) |

### Definition of done (per portal)

- [ ] OpenSpec change `add-portal-NN-slug` written and validated
- [ ] Core features in, win condition works, Shining Star reachable at every difficulty
- [ ] Completion bot finishes it (story + default) in CI; pace estimate 12–28 min
- [ ] Unit tests for its mechanics; overall coverage stays ≥ 95 % lines / 85 % branches
- [ ] All its spec scenarios have tests (traceability check)
- [ ] Friend marks placed (≥ 2), snowflake conditions implemented
- [ ] Touch layout covers every action; keyboard/mouse and gamepad work
- [ ] Golden frames recorded; performance smoke within budget
- [ ] Assets credited in `assets/CREDITS.md`; originality review recorded
- [ ] Played start to finish once by a human

---

## 6. Parallel tracks

| Track | Plan |
|---|---|
| **Art** | Most art is produced *by code* in `tools/`: procedural textures (64×64), voxel models defined in text, palettes per portal. This keeps it CC BY, tiny and fast to iterate. Friends can contribute hand-made models/sprites any time via `assets/` + a CREDITS row. |
| **Audio** | Tiny runtime synth + tracker player (M1a); each portal gets 1 music loop + ~15 SFX generated or composed in the era's style. Voice lines are subtitled text plus synth "barks" (no voice actors needed). |
| **Split screen** | Cheap with the multi-view renderer: enabled per portal from Day 1 where the concept table says so. |
| **Online multiplayer** | Optional. Transport + lockstep tests in week 2 of December, WebRTC hub session in week 3, party portals for Days 7, 9, 16 and 24 first. Ships only if it doesn't threaten the daily portal. |
| **WebXR** | Progressive: hub + cockpit portals (6, 11) first, the rest as "seated". Tabletop RTS view after Christmas. |
| **Quality** | CI on every push; sanitizer and coverage jobs block. A weekly cleanup slot (Sundays) for refactoring kits before the next portals reuse them. |

---

## 7. Releases & distribution

| Channel | How | When |
|---|---|---|
| Web (main channel for friends) | GitHub Pages from `docs/play/` on `main`, refreshed with `--target publish-web` + commit | from 30 Nov; needs Settings → Pages → Deploy from a branch → `main` / `/docs` |
| Windows | zip from CI (`cadvent-windows-x64`) | 30 Nov, then weekly or on request |
| Linux | tar.gz (+ AppImage later) | 30 Nov, then weekly |
| Tags | `rc-1` (30 Nov), `day-01` … `day-24`, `v1.0` (24 Dec) | every evening |

Each portal is hidden behind its date at runtime, so shipping early is safe:
friends can't open Day 9 before 9 Dec without the dev override.

---

## 8. Risks, early warnings and fallbacks

| Risk | Early warning | Fallback |
|---|---|---|
| M1a renderer takes longer | shadows not working by 21 Nov | ship directional shadows only; point shadows added in December |
| Hall not ready by 30 Nov | no walkable hall on 26 Nov | a minimal hall (one corridor of arches) ships; full hall during week 1 |
| RTS kit too big for Day 3 | kit not playable by 1 Dec | Day 3 ships a smaller scenario (fewer units); Day 5 benefits from the finished kit |
| A portal is late | bot not green the afternoon before | "still thawing…" sign, opens 1–2 days late; the Star Arch logic tolerates it |
| Coverage gate slows work | portal code below 95 % | write tests during the day, not at the end; extract logic into pure functions |
| WebGL/mobile performance | perf smoke above 80 % budget | lower default internal resolution on phones; fewer shadowed lights |
| Bugs in live portals | friends report issues | hotfix slot every morning; replays from bug reports reproduce issues |
| IP concerns | anything looks/sounds like a copy | originality review before every release; replace the asset |

---

## 9. After Christmas (M4, January)

- Boxing Day mode: speed-run timers per portal, 24-portal marathon.
- Online multiplayer completed for all portals that declare it.
- XR tabletop mode for RTS portals.
- Archive all OpenSpec changes, so `openspec/specs/` describes the finished game.
- Post-mortem; prepare the project to be re-run as *cAdvent 2027* (new friends file, new portals).

---

## 10. Open decisions for the project owner

| Decision | Default if nobody decides |
|---|---|
| Enable GitHub Pages (Settings → Pages → Deploy from a branch → `main` / `/docs`) | web build only available as a CI artifact |
| Release time each evening | 20:00 CET |
| Who playtests each portal before release | the owner, plus any friend who wants early access via the dev URL |
| Do friends contribute art/music? | procedural art and synth music only |
| Is online multiplayer a must? | optional; split screen first |

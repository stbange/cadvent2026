# 1 · Vision, Calendar & The Hall

## 1.1 Elevator pitch

**cAdvent 2026** is a first-person 3D advent calendar for people who grew up on
mid-90s PC games. You walk through a large, solemn festive hall with 24
sealed portals. From **1 December 2026** until **24 December 2026** one more
portal thaws each day. Behind each portal waits a compact, original
first-person game, about **20 minutes** long, that pays homage to a genre-defining
classic from roughly 25 years ago, re-imagined so that *everything* is
played through your own eyes.

Design mantra: **"Recognisable by how it plays, not by what it copies."**
Every portal captures the *signature mechanics, camera feel, colour mood, UI idiom
and pacing* of its inspiration, wrapped in an original winter-themed world.
No original names, characters, logos, maps, sounds or music are reused (see
[05-production-plan.md §5.6](05-production-plan.md#56-intellectual-property-policy)).

## 1.2 Design pillars

| Pillar | What it means in practice |
|---|---|
| **One a day, done in 20** | Each portal has a clear goal, a beginning, a climax and an end screen. A first clear takes 15–25 min; mastery (3 stars) invites replays. |
| **First person, always** | RTS, isometric, top-down and third-person originals are re-framed (commander on the battlefield, squad leader, hive-eye, cockpit…). The re-framing *is* the creative core of each design. |
| **Retro but alive** | Low internal resolution, chunky texels, palette-quantised post-processing — but with real-time dynamic lights, shadow maps, fog and particle light. "How we *remember* those games looked." |
| **Shared tech, distinct soul** | Five reusable gameplay kits (FPS, RTS, Vehicle, RPG/Tactics, Platformer) keep 24 games feasible; each portal still gets at least one mechanic nobody else has. |
| **Friends first** | Couch split-screen and online play wherever the genre allows; the hub itself is a social space. |
| **Tested like a product** | OpenSpec-driven requirements, ≥95 % unit-test coverage, and a bot that finishes every portal in CI. |

## 1.3 The Hall of Long Nights (hub)

### Look & mood — "grave festive"

A vast, cathedral-like winter hall at midnight. Black basalt columns, frost on
the stained-glass windows, banners in deep crimson and pine green, iron
chandeliers with real (dynamic, flickering, shadow-casting) candlelight. A
light snowfall drifts through a broken skylight onto a **giant voxel fir tree**
at the centre. The atmosphere is solemn, a little eerie, but warm: a choir hum,
crackling braziers, distant bells. Think "a museum of memories that only opens
in December".

### Layout

```
                 ┌──────────── North Apse: Day 24 grand portal ────────────┐
                 │                                                          │
   West gallery  │   13  14  15  16  17  18     (upper balcony, stairs)     │  East gallery
   (ground)      │                                                          │  (ground)
   1  2  3  4  5 6│            ✶  THE TREE (voxel, ornaments = trophies)    │ 7 8 9 10 11 12
                 │                                                          │
                 │   19  20  21  22  23         (upper balcony)             │
                 │                                                          │
                 └──────────── South: Entrance, Fireplace, Notice board ────┘
```

* **Days 1–12** on the ground floor (west and east galleries), **13–23** on the
  balcony, **Day 24** in the north apse behind the tree, a twice-as-tall portal.
* Distances are short (all doors reachable in < 40 s walk) and a "sled rail"
  fast-travel along the walls opens once 6 portals are cleared.

### Portal states

| State | When | Visual | Interaction |
|---|---|---|---|
| **Sealed** | before its day | Frozen solid; day number glows faintly under the ice; a countdown on the brass plate. | Touching it shows "Opens in 3 days 4 h". |
| **Thawing** | day has come, never entered | Ice cracks with a light-shaft burst the first time you look at it that day (once-per-portal moment), then reveals a swirling preview. | Walk in to play. |
| **Open** | entered at least once | A live **render-to-texture preview** of the game's attract mode loops inside the arch, lit in that game's palette. | Walk in; plate shows best time / stars. |
| **Mastered** | 3 stars | Golden frame, a unique ornament appears on the tree. | — |

### Progression & rewards

* Each portal awards **0–3 stars** (1 = completed, 2 = completed under a par
  condition, 3 = a portal-specific mastery challenge, e.g. "no saves", "all
  secrets", "win with ≤ 8 units").
* Stars light candles on the **Advent wreath** above the entrance (one candle per
  6 stars collected, i.e. 4 candles × 18 stars).
* Every completed portal hangs a themed **ornament** on the tree (voxel model).
  The tree is the trophy cabinet.
* **Christmas Eve epilogue**: when Day 24 is completed and at least 24 stars are
  collected, the tree's star ignites and the hall's roof opens to a night-sky
  credits sequence flying past all 24 worlds.
* After 24 December all portals remain open forever; a "Boxing Day" mode adds
  speed-run timers and a combined 24-portal marathon.

### Hub features

* **Notice board** (south): daily "chronicle" — one paragraph of nostalgia per
  day about the genre the new portal celebrates, plus controls of the day.
* **Fireplace**: settings (resolution scale, palette filter, controls, accessibility).
* **Mirror**: choose your avatar (original festive characters: a nutcracker, a
  polar bear knight, a gingerbread robot, a krampus-ish goat, a snow witch, an
  elf mechanic — all CC BY 4.0 voxel models).
* **Gramophone**: jukebox for the original soundtrack of unlocked portals.

## 1.4 The calendar rule

* The unlock rule is evaluated against the **device's local date** (so friends
  in different time zones unlock at their own midnight).
* Portal *n* (1 ≤ n ≤ 24) is unlocked iff `local_date ≥ 2026-12-n` .
* Before 1 Dec 2026 the hall is visitable in "teaser" mode (all sealed, Day 1 countdown).
* Clock-tampering is *not* fought (it's a gift for friends), but the web build
  can optionally ask a tiny time endpoint, and a build flag
  `CADV_UNLOCK_ALL=1` / URL parameter `?dev=unlockall` exists for development and
  tests. The year is a build constant so the project can be re-run as
  *cAdvent 2027* with new portals.

## 1.5 Session flow

1. Title card → "Press any key / Tap to enter" (doubles as audio-unlock on web).
2. Spawn at the entrance, the newest unlocked portal is highlighted by a
   beam of light through the skylight.
3. Walk into a portal → 1.5 s "portal tunnel" transition (palette morph from
   hub to game palette) → game-specific **briefing** (in-world, never a
   flat menu: a CRT, a parchment, a radio…).
4. Play. Pause menu: resume, restart checkpoint, controls, return to hall.
5. End screen in the game's own UI style → stars → back to the hall,
   ornament flies to the tree.

Saves: per-portal checkpoint + best results; stored in a small text save file
(desktop: user data dir via `SDL_GetPrefPath`; web: IDBFS-backed file).

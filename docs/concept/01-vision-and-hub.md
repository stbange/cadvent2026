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
| **One a day, done in 20** | Each portal has a clear goal, a beginning, a climax and an end screen. A first clear takes 15–25 min; mastery (3 snowflakes) invites replays. |
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
| **Open** | entered at least once | A live **render-to-texture preview** of the game's attract mode loops inside the arch, lit in that game's palette. | Walk in; plate shows best time, snowflakes and whether its Shining Star was found. |
| **Mastered** | 3 snowflakes | Golden frame, a unique ornament appears on the tree. | — |

### Progression & rewards

* Each portal awards a rating of **0–3 snowflakes ❄** (1 = completed,
  2 = completed under a par condition, 3 = a portal-specific mastery challenge,
  e.g. "no saves", "all secrets", "win with ≤ 8 units").
* Snowflakes light candles on the **Advent wreath** above the entrance (one
  candle per 6 snowflakes collected, i.e. 4 candles × 18 snowflakes).
* Every portal 1–23 holds one collectible **Shining Star**, the key to Day 24
  (see [§1.5](#15-the-23-shining-stars-and-the-24th-portal)).
* Players' friends turn up everywhere as tags, names and graffiti (see
  [06-friends-marks.md](06-friends-marks.md)); found marks fill the
  **Wall of Legends** in the hall.
* Every completed portal hangs a themed **ornament** on the tree (voxel model).
  The tree is the trophy cabinet.
* **Christmas Eve epilogue**: completing Day 24 wins the **24th Shining
  Star**. The player carries it to the top of the tree; the star ignites and the
  hall's roof opens to a night-sky credits sequence flying past all 24 worlds.
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
* **Wall of Legends** (east gallery): six carved plaques, one per friend
  (Dr. Doom, Bloodmaker, C6P, X-Ray Junkie, Messerjocke, Foen-X). Every friend's
  mark found inside a portal adds a glowing notch to that friend's plaque.
* **Star Arch** (north apse): the 23 star sockets around portal 24, see §1.5.

## 1.4 The calendar rule

* The unlock rule is evaluated against the **device's local date** (so friends
  in different time zones unlock at their own midnight).
* Portal *n* (1 ≤ n ≤ 23) is unlocked iff `local_date ≥ 2026-12-n`. Portal 24 additionally needs all 23 Shining Stars set into the Star Arch (§1.5).
* Before 1 Dec 2026 the hall is visitable in "teaser" mode (all sealed, Day 1 countdown).
* Clock-tampering is *not* fought (it's a gift for friends), but the web build
  can optionally ask a tiny time endpoint, and a build flag
  `CADV_UNLOCK_ALL=1` / URL parameter `?dev=unlockall` exists for development and
  tests. The year is a build constant so the project can be re-run as
  *cAdvent 2027* with new portals.

## 1.5 The 23 Shining Stars and the 24th portal

The Shining Stars are the meta-quest that ties the calendar together.

* **One star per portal 1–23**, a physical, glowing pickup (voxel star with its
  own dynamic light) that is won at the portal's climax. Each portal's design
  says exactly where (e.g. Day 1: it tumbles out of the exploding Light Siphon;
  Day 6: Santa Claude hands it over). Completing a portal at any difficulty,
  including story mode, always makes the star reachable, so nobody gets
  stuck.
* The star must be **picked up and carried out**. It is saved the moment the
  player leaves the portal with it (a won result without picking the star up is
  possible but leaves the socket empty; the portal's plate then shows a hint).
* **The Star Arch**: portal 24 in the north apse is framed by **23 numbered
  sockets**. Back in the hall, the player walks to the arch and presses *use*; every
  carried star flies into the socket of its day with a chime. Stars glow in their
  socket and cast light across the apse, so the apse gets brighter through December.
* **Portal 24 opens only if both conditions hold:** the date is 24 Dec 2026 or
  later **and** all 23 sockets are filled. On 24 Dec with missing stars, the arch
  shows which days are still missing, and light beams in the hall point to those
  portals.
* **Inside portal 24** the 24th Shining Star is won (capturing the Star Beacon) and
  placed on the top of the tree, which triggers the epilogue.
* **Multiplayer**: a star picked up in co-op counts for every participant's
  save. In versus portals each player who finishes the match (win or lose)
  gets it, so friends never block each other. In a shared hub session anyone can
  set their own stars; the arch shows the host's sockets.
* **Dev/test override**: `?dev=unlockall` also fills all sockets (marked "DEV").

## 1.6 Session flow

1. Title card → "Press any key / Tap to enter" (doubles as audio-unlock on web).
2. Spawn at the entrance, the newest unlocked portal is highlighted by a
   beam of light through the skylight.
3. Walk into a portal → 1.5 s "portal tunnel" transition (palette morph from
   hub to game palette) → game-specific **briefing** (in-world, never a
   flat menu: a CRT, a parchment, a radio…).
4. Play. Pause menu: resume, restart checkpoint, controls, return to hall.
5. End screen in the game's own UI style → snowflakes → back to the hall,
   ornament flies to the tree; a newly found Shining Star floats above the
   player's head until it is set into its socket at the Star Arch.

Saves: per-portal checkpoint + best results; stored in a small text save file
(desktop: user data dir via `SDL_GetPrefPath`; web: IDBFS-backed file).

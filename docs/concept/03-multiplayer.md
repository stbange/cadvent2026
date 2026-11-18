# 3 · Multiplayer (optional milestone, designed in from day one)

Multiplayer is optional for the first release, but the architecture
(pure deterministic sim, per-player input frames, multi-view renderer) is
built so it can be switched on per portal without rewrites.

## 3.1 Modes

| Mode | Platforms | Players | How |
|---|---|---|---|
| **Split screen** | Windows, Linux (web: 2 players) | 2–4 local | one window, N views; each player bound to a device (keyboard+mouse, gamepad 1–3). Layouts: 2 = vertical split, 3 = 2+1, 4 = quad. |
| **Online** | all, *web-first* | 2–8 (portal-dependent) | host-based sessions via WebRTC data channels; join by 6-letter room code or link (`…/?room=SNOWFX`). |
| **Hub social** | all | up to 8 | everyone sees each other's avatars in the Hall, can emote, ring the hall bell, and enter a portal together ("party portal": stand in the arch, the host confirms). |

## 3.2 Network architecture

```
 browser A (host) ◀── WebRTC DataChannel (unreliable + reliable) ──▶ browser B
        ▲                                                             ▲
        └──────── signalling only (tiny WebSocket relay) ─────────────┘
```

* **Transport abstraction** `cadv_net_transport` with three backends:
  `loopback` (tests), `webrtc` (web via JS library; desktop via
  libdatachannel, optional), `udp` (desktop LAN fallback).
* **Signalling server**: ~200 lines (Node or C), stateless room-code
  relay; deployable to any free tier. A STUN server is enough for most home
  networks; TURN is out of scope (documented limitation).
* No accounts, no personal data; nicknames are chosen per session.

### Two replication models, chosen per kit

| Model | Used for | Details |
|---|---|---|
| **Deterministic lockstep** (with input delay 2–4 ticks) | RTS kit (3, 5, 18, 21, 23, 24), turn-based (15) | only inputs are sent; every peer runs the same sim; state hash every 30 ticks detects desync and dumps a replay for debugging. |
| **Host-authoritative snapshots** with client prediction & interpolation | FPS / Vehicle / RPG kits | 20 Hz delta-compressed snapshots, client-side prediction of own movement, lag-compensated hitscan (rewind ≤ 200 ms). |

## 3.3 Per-portal multiplayer matrix

| Day | Portal | Split screen | Online | Notes |
|---|---|---|---|---|
| 1 | Sleighed! | co-op 2 | co-op 4, deathmatch 4 | |
| 2 | Aurora Carpet | versus 2 | versus 4 | rival wizards collecting mana |
| 3 | Frostfront | — | 1v1 | lockstep RTS |
| 4 | Ice Tomb of the Winter King | — | co-op 2 | ghost race / co-op puzzle |
| 5 | Hearthguard | — | 1v1, 2v2 | |
| 6 | Iron Yule | versus 2 | versus 4 | arena duel |
| 7 | Tinsel Strike | 2v2 (+bots) | 5v5 (bots fill) | the multiplayer showcase |
| 8 | Cascade | co-op 2 | co-op 2 | |
| 9 | Skifrost | 2v2 | 4v4 CTF (bots fill) | |
| 10 | Neon Syndic | co-op 2 (each controls 2 agents) | co-op 4 | |
| 11 | Deep Thaw | co-op 2 | co-op 4, anarchy | |
| 12 | Below the Chapel | co-op 2 | co-op 4 | |
| 13 | Hellfrost Station | co-op 2, DM 4 | co-op 4, DM 4 | |
| 14 | Ring of the Evergreen | **co-op 3** (like the original's party) | co-op 3 | AI fills missing party members |
| 15 | Frostfall Protocol | hot-seat 2 | co-op 2 (each owns soldiers) | |
| 16 | Rune of Rime | DM 4 | DM 8 | |
| 17 | Way of the Glowblade | duel 2 | duel 2 | blade duels |
| 18 | Hivemind | — | 1v1 | |
| 19 | Skyfall Isle | co-op 2 | co-op 4 | |
| 20 | Ghost in the Orbiter | solo | solo | intentionally lonely |
| 21 | Ages of Midwinter | — | 1v1, 2v2 | |
| 22 | Grave Tidings | co-op 2 | co-op 2, DM 4 | |
| 23 | Total Thaw | — | 1v1 | |
| 24 | Polar Alert | 2v2 | 1v1 … 4-player FFA | the finale party |

## 3.4 Engineering rules

* Every portal must pass its **completion bot test in loopback multiplayer**
  if it declares a multiplayer capability (2 bots, one host one client).
* Desync = test failure: lockstep portals run 2 sims side by side in unit tests
  with identical input and compare hashes every tick.
* Network simulator in tests (latency, jitter, 5 % loss) for snapshot portals.

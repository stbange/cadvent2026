# cAdvent 2026 — Game Concept

A first-person 3D advent calendar: one solemn festive hall, 24 portals, one new
retro-inspired game per day from **1 to 24 December 2026**.

| # | Document | Contents |
|---|---|---|
| 1 | [Vision, Calendar & The Hall](01-vision-and-hub.md) | pitch, pillars, hub layout, portal states, unlock rule, progression |
| 2 | [Technology](02-technology.md) | C17 + SDL3 + OpenGL/WebGL2, architecture, rendering, platforms, WebXR, controls & touch |
| 3 | [Multiplayer](03-multiplayer.md) | split screen, WebRTC online, lockstep vs snapshots, per-portal matrix |
| 4 | [Quality & Testing](04-quality-and-testing.md) | OpenSpec workflow, ≥ 95 % coverage, game bots, golden frames, CI |
| 5 | [Production Plan](05-production-plan.md) | schedule, milestones, assets, risks, IP policy, licences |
| P | Portals [Week 1](portals/week-1.md) · [Week 2](portals/week-2.md) · [Week 3](portals/week-3.md) · [Week 4](portals/week-4.md) | the 24 game designs |

## The 24 portals at a glance

All titles, worlds and characters are **original**; the right-hand column
names the classic whose *genre and mechanics* each portal celebrates.

| Day | Portal | Kit | First-person twist | Goal | Inspired by |
|---|---|---|---|---|---|
| 1 | **Sleighed!** | FPS | interactivity in every room | blow up the Light Siphon on the mall roof | Duke Nukem 3D (1996) |
| 2 | **Aurora Carpet** | Vehicle | flying carpet, spell hands, terraforming | own 75 % of the world's mana | Magic Carpet 2 (1995) |
| 3 | **Frostfront** | RTS | commander on foot, wrist-tablet sidebar | destroy the Ember Spire HQ | Command & Conquer (1995) |
| 4 | **Ice Tomb of the Winter King** | Platformer | grid-snapped first-person parkour | recover 3 Sun Gears | Tomb Raider (1996) |
| 5 | **Hearthguard** | RTS | village captain with signal horn | survive 3 raids, burn the Longhall | Warcraft II (1995) |
| 6 | **Iron Yule** | Vehicle | mech cockpit, heat & torso twist | 3-mission campaign, ace duel | MechWarrior 2 (1995) |
| 7 | **Tinsel Strike** | FPS | — (native) | win best-of-9 bomb rounds | Counter-Strike (1999) |
| 8 | **Cascade** | FPS | no-cutscene storytelling | reverse the resonance beam | Half-Life (1998) |
| 9 | **Skifrost** | Vehicle/FPS | skiing + jetpack | 5 flag captures | Starsiege: Tribes (1998) |
| 10 | **Neon Syndic** | RPG/FPS | lead agent + 3 AI agents, Charm Beacon | hack 3 relays, extract | Syndicate Wars (1996) |
| 11 | **Deep Thaw** | Vehicle | 6DOF mine flight | reactor down, escape | Descent (1995) |
| 12 | **Below the Chapel** | RPG | first-person ARPG, orbs in view | slay the Pale Bishop on level 4 | Diablo (1996) |
| 13 | **Hellfrost Station** | FPS | — (native, speed-faithful) | close the Abyss gate | Doom (1993) |
| 14 | **Ring of the Evergreen** | RPG | ring menu as holo-wheel, 3-player co-op | rekindle 3 shrines, beat the Hollow Stag | Secret of Mana (1993) |
| 15 | **Frostfall Protocol** | Tactics | turn-based, inhabit each soldier | secure crash site, defend base | X-COM: UFO Defense (1994) |
| 16 | **Rune of Rime** | FPS | — (native) | 4 runes, defeat the Old Cold | Quake (1996) |
| 17 | **Way of the Glowblade** | FPS/melee | directional blade, Dawn/Dusk choice | defeat Vesh | Jedi Knight (1997) |
| 18 | **Hivemind** | RTS | you are a flying hive-eye, pheromone trails | destroy the Command Bastion | StarCraft (1998) |
| 19 | **Skyfall Isle** | FPS | lighting showcase, alt-fire everywhere | steal a shuttle, free the Ooli | Unreal (1998) |
| 20 | **Ghost in the Orbiter** | FPS/imm-sim | wrist-MFD, cyberspace 6DOF | wipe the AI NOEL | System Shock (1994) |
| 21 | **Ages of Midwinter** | RTS | chieftain among villagers, ages transform around you | build & hold the Solstice Monolith | Age of Empires (1997) |
| 22 | **Grave Tidings** | FPS | — (native) | silence the Hollow Bell | Blood (1997) |
| 23 | **Total Thaw** | RTS/Vehicle | you *are* the Commander walker | destroy the enemy Commander | Total Annihilation (1997) |
| 24 | **Polar Alert** | RTS | ground, ship bridge, dirigible views | capture the Star Beacon (finale) | C&C: Red Alert (1996) |

## How the concept maps to OpenSpec

The concept is turned into requirements in
[`openspec/changes/bootstrap-advent-calendar`](../../openspec/changes/bootstrap-advent-calendar/proposal.md).
Each portal then gets its own change (`add-portal-NN-<slug>`) derived from its section here.

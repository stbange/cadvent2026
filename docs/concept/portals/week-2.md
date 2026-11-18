# Portals — Week 2 (Days 7–12)

---

## Day 7 — **TINSEL STRIKE**
*Genre homage: Counter-Strike (Valve / Le & Cliffe, 1999–2000) · Kit: FPS*

**Inspiration.** Round-based team tactics: short rounds, no respawn, **buy
phase with an economy** (win/lose/kill rewards), bomb plant/defuse at two sites,
hostage rescue, weapon **recoil patterns** and movement inaccuracy, flash and
smoke grenades, footstep audio as information, compact readable maps with
chokepoints and mid control. Built on a mod of a 1998 shooter: muted, realistic textures.

**Premise.** In the snowy market town of **Glühstadt**, the *Humbug Syndicate* (masked pranksters) tries to plant a **glitter bomb** in the town's
Christmas market; the *Nutcracker Guard* defends. Non-lethal, festive "tagging":
defeated players pop into a puff of confetti.

**First-person adaptation.** Native.

**Goal.** Win a **best-of-9** match (first to 5 rounds) as either side against bots.

**20-minute flow.** 9 rounds × ~2 min incl. buy phase. Half-time side switch after round 4.

**Mechanics.**
* Core: 1:45 round timer, 15 s buy phase, economy ($800 start, win/loss/plant bonuses, loss streak bonus), 7 weapons with learnable spray patterns, armour, defuse kit, flashbang, smoke (volumetric-ish sprite clouds that block bot vision), footsteps (walk key silences), two plant sites A/B, 40 s bomb timer, defuse 10 s / 5 s with kit, bot AI with roles (entry, lurk, anchor) and callouts.
* Garnish: hostage-rescue variant map (rescue kidnapped carolers), spectator cam after death, mini-radar.

**Cast.** 5v5 (you + 4 bots); bots use a tactical waypoint graph with cover nodes.

**Look.** Grounded half-timbered market, string lights (many small unshadowed
lights, a few shadowed lamp posts), snow on roofs, warm stalls vs cold
alleys. Muted palette, crisp sprite-free mesh characters (low-poly with 1 texture).

**Stars.** ★ win the match · ★★ win both halves · ★★★ win a round with a 1v3 clutch or a pistol-round ace.

**Multiplayer & XR.** The flagship MP portal: 2v2 split with bots, 5v5 online (bots fill). XR full.

**Bot & tests.** Bots play both teams; CI runs 50 seeded bot-vs-bot matches and checks balance (CT win rate 40–60 %). Tests: economy rules table, defuse timing, recoil pattern determinism, smoke LOS blocking.

---

## Day 8 — **CASCADE**
*Genre homage: Half-Life (Valve, 1998) · Kit: FPS*

**Inspiration.** Story told **entirely in first person without cutscenes**:
tram ride intro through a research facility, an experiment goes catastrophically
wrong, scientists and security guards as NPC companions, crab-like parasites
and alien wildlife, then military clean-up squads with smart squad AI,
scripted set pieces, environmental puzzles, a silent protagonist with a crowbar.

**Premise.** At **Polaris Deep**, an Arctic research station drilled into an
ancient meteorite, junior technician *Ines Kowalczyk* arrives on the day the
"Lumen Shard" resonance test goes wrong. Space folds; snow-white **skitterlings**
pour out; the corporation sends a "clean-up" unit, the *Blackice Detail*.

**First-person adaptation.** Native. No cutscene ever takes control from
the player.

**Goal.** Reach the surface antenna array and reverse the resonance beam before the station implodes.

**20-minute flow.**
1. *Arrival (3 min)*: monorail through the station, NPC chatter, put on the thermal suit.
2. *The Test (2 min)*: you push the sample cart; the cascade (screen-wide light distortion, lights die, alarms).
3. *Ruins (6 min)*: ice axe, skitterlings, a guard joins you; power puzzle to restart the lift.
4. *Blackice (6 min)*: squad soldiers flank and throw grenades; scientists must be escorted to a door scanner.
5. *Array (3 min)*: climb the antenna in a blizzard while a giant tentacled thing breaches the ice; flip 3 breakers.

**Mechanics.**
* Core: ice axe, 9mm, shotgun, crossbow, grenades; NPC follower system (use to follow/wait), retina/door scanners requiring scientists; squad AI (flank, suppress, retreat when hurt, grenade flush-out); scripted sequences as non-blocking triggers; thermal-suit energy with wall charger stations; physics crates (stack to climb).
* Garnish: hidden "observer" in a suit watching from afar (original enigmatic figure "The Archivist"), radio chatter of the soldiers.

**Look.** Industrial-scientific: grey panels, yellow-black hazard stripes,
frosted glass labs, blinking consoles (dozens of small lights). The cascade
moment flips lighting to a pulsating violet with strong shadows. Soldiers
meshes, skitterlings as sprites.

**Stars.** ★ finish · ★★ all 3 scientists survive · ★★★ under 16 min.

**Multiplayer & XR.** Co-op 2. XR full.

**Bot & tests.** Bot follows story graph, issues NPC follow orders. Tests: follower pathing to scanner, squad AI state transitions, sequence triggers re-entrancy, suit charger limits.

---

## Day 9 — **SKIFROST**
*Genre homage: Starsiege: Tribes (Dynamix, 1998) · Kit: Vehicle (+FPS weapons)*

**Inspiration.** Huge open terrain, **jetpacks with energy**, **skiing**
(frictionless downhill sliding) for enormous speed, team **Capture the Flag**,
three armour classes, **slow-moving disc launcher** requiring leading shots,
base generators powering turrets and inventory stations, vehicles.

**Premise.** Two winter tribes, **the Rime Wolves** and **the Ember Owls**,
settle the festival games of the Northern Reach with the most dangerous
sport ever invented.

**First-person adaptation.** Native.

**Goal.** First to **5 flag captures** (or most captures after 20 min).

**20-minute flow.** Quick training course (2 min: ski, jet, disc), then a CTF match vs bots on the map **"Snowglobe Basin"**: rolling hills ideal for skiing, two hill-top bases.

**Mechanics.**
* Core: ski (hold jump on slopes = no friction), jetpack energy, 3 armours (light/medium/heavy change speed, energy, HP, weapon slots), *Disc Launcher* (slow projectile, splash, knockback), chaingun, grenade launcher, energy pack; generator → turrets & inventory station (change loadout); flag physics (drop, return timer).
* Garnish: hover-sled vehicle, sensor jammer, mid-air "pinball" kill announcer.

**Look.** Wide outdoor terrain (heightmap), aurora sky, long draw distance with
fog, chunky angular armour meshes in tribal colours; disc projectiles are bright
point lights that light the snow as they fly. Bases are fortresses with
generators that glow (and go dark when destroyed).

**Stars.** ★ win · ★★ win 5–0 · ★★★ make a capture with flag-carry speed > 150 km/h.

**Multiplayer & XR.** 2v2 split, 4v4 online CTF with bots. XR seated (speed!) with strong vignette.

**Bot & tests.** Bots use ski routes (precomputed downhill lines) and role assignment. Tests: ski momentum conservation, energy drain/regen, flag return timer, generator → turret power dependency.

---

## Day 10 — **NEON SYNDIC**
*Genre homage: Syndicate Wars (Bullfrog, 1996) · Kit: RPG/Tactics + FPS*

**Inspiration.** Isometric cyberpunk tactics: a **squad of four cybernetic
agents**, a megacity with fully **destructible** buildings, the
**persuasion device** that turns civilians and enemies into followers, drug-like
**adrenaline / perception / intelligence** sliders, minigun, laser, Gauss, cars
to steal, corporate vs church factions, rain and neon at night.

**Premise.** In the neon megacity **Nivalis**, the *Halcyon Corp* runs the
city's mood through a cheer-broadcast network. You lead a 4-agent cell from the
underground *Sleepless Choir* to cut the signal.

**First-person adaptation.** You are the **lead agent**; three AI agents
follow in formation and receive quick orders (radial: hold, follow, attack
target, cover). Squad **neural sliders** on the wrist (overclock: speed, aim,
awareness) drain a stamina pool. The *Charm Beacon* converts nearby civilians
into a crowd that follows and shields you; persuaded guards fight for you.

**Goal.** Hack 3 broadcast relays across a city district, then extract by
VTOL from the plaza.

**20-minute flow.** *Market (6 min)* relay 1 among crowds; *Industrial
district (6 min)* steal a hover-car, relay 2 in a factory; *Halcyon Tower
(5 min)* relay 3 with a turret gauntlet; *Plaza (3 min)* hold for extraction.

**Mechanics.**
* Core: 4 agents with individual HP/loadout, squad orders, Charm Beacon (followers count limit grows with persuaded people), destructible facades (voxel chunks), vehicles, overclock sliders with crash afterwards, alert levels, hacking mini-game (timing ring).
* Garnish: agent cyber-upgrade between relays, rain with reflective puddles.

**Look.** The *showcase for dynamic lights at night*: rain, neon signs
(many lights), car headlights with shadows, holographic ads. Voxel-destructible
building frontages. Low res, dark purple/cyan palette.

**Stars.** ★ complete · ★★ no agent lost · ★★★ 30+ persuaded followers at once.

**Multiplayer & XR.** Co-op 2 split (each controls 2 agents), 4 online. XR seated.

**Bot & tests.** Bot path through district with squad orders. Tests: persuasion range/limit, voxel destruction chunking, squad formation, overclock crash timing.

---

## Day 11 — **DEEP THAW**
*Genre homage: Descent (Parallax, 1995) · Kit: Vehicle (6DOF)*

**Inspiration.** True **six-degrees-of-freedom** flight in twisting mine
tunnels; no up or down; hostile mining robots; colour **keys (blue,
yellow, red)**; **rescue hostages**; destroy the **reactor**, then a timed
**escape** sequence; flares to light dark tunnels; homing missiles; automap in 3D.

**Premise.** A robot-run ice mine under the North Pole has gone rogue after a
firmware virus. Pilot *Jun Arkwright* flies the ice-cutter ship **Rimecutter-9** in to rescue trapped elf miners.

**First-person adaptation.** Native.

**Goal.** Rescue all 6 trapped miners (optional but needed for stars), collect 3
keys, destroy the reactor core, escape within 45 s.

**20-minute flow.** Mine level 1 "Glacier Shafts" (8 min), level 2 "The
Core Hollow" (10 min) with the reactor boss and escape run (2 min).

**Mechanics.**
* Core: 6DOF physics with drag and bank, primary (lasers, spreadfire), secondary (concussion, homing missiles), energy & shield, flares (thrown dynamic lights that stick to walls), keys & coloured doors, hostages, reactor with countdown, 3D wireframe automap, 7 robot types with distinct AI (melee driller, sniper, cloaker, homing launcher…).
* Garnish: guide-bot that leads to next objective, secret level.

**Look.** Portal/cell-based tunnel geometry, **gouraud-shaded robots**, ice
walls with blue-green fluorescence, flares and laser bolts as the main lights (dark
tunnels make dynamic light essential); shadowed headlight spot.

**Stars.** ★ escape · ★★ all miners rescued · ★★★ escape with > 20 s left.

**Multiplayer & XR.** Co-op 2 split / 4 online, "anarchy" dogfight. XR full (seated 6DOF; comfort options mandatory).

**Bot & tests.** 6DOF bot uses tunnel graph + steering. Tests: 6DOF integration stability, homing missile lead, key/door logic, reactor countdown and exit trigger.

---

## Day 12 — **BELOW THE CHAPEL**
*Genre homage: Diablo (Blizzard North, 1996) · Kit: RPG*

**Inspiration.** Isometric **action RPG**: a cursed **town** above a
**randomly generated** cathedral → catacombs → caves → hell descent;
click-to-attack, **colour-coded loot** (magic blue, unique gold), item
affixes, potion belt, gold, health/mana **orbs**, gothic dread, a town
healer and blacksmith, lightning-fast loot loop, iconic boss with a growl.

**Premise.** The mountain village of **Hollowbell** hasn't heard its chapel
bells since the snow turned grey. Beneath the chapel, the **Pale Bishop**
sleeps no more.

**First-person adaptation.** First-person **melee/spell combat** with
hit-feedback, a **belt hotbar**, and loot that pops and sparkles in the
light. Health and mana are shown as two **glass orbs held in your vision's
lower corners**, with liquid sloshing. Classes: Warden (melee),
Huntress (bow), Seer (spells), chosen at the well.

**Goal.** Descend 4 procedurally generated levels and defeat the Pale Bishop.

**20-minute flow.** Village (2 min: healer, smith, quest from the bell-ringer)
→ Crypt L1–L2 (8 min) → Ice caves L3 (5 min) → The Hollow Cathedral L4 + boss (5 min).
Town portal scroll to return and sell.

**Mechanics.**
* Core: seeded room-and-corridor generator (per run), loot tables with prefix/suffix affixes, rarity colours, inventory grid, belt (4 potion slots), 3 classes × 3 skills, XP & 5 levels, 8 monster types + 2 elites with modifiers, boss with 3 phases, town portal.
* Garnish: shrines with random effects, a mini-quest (cursed bell clapper), hardcore toggle.

**Look.** Gothic stone, braziers, and the defining trick: **a light radius
around the player** (a shadowed point light on the character) with everything
beyond fading to black; ice caves glow faintly. Enemies as 8-dir sprites;
loot items as tiny voxel models with rarity-coloured glints.

**Stars.** ★ defeat the Bishop · ★★ on "Nightmare" difficulty · ★★★ find a unique (gold) item and win without dying.

**Multiplayer & XR.** Co-op 2 split / 4 online. XR seated.

**Bot & tests.** Bot explores via frontier search, fights and drinks potions. Tests: generator connectivity for 1 000 seeds, affix roll ranges, belt/inventory rules, XP curve.

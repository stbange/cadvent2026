# Portals — Week 3 (Days 13–18)

---

## Day 13 — **HELLFROST STATION**
*Genre homage: Doom (id Software, 1993) · Kit: FPS*

**Inspiration.** Pure speed: fast run-and-gun with no jumping or vertical aim,
**keycards** (red/blue/yellow) gating non-linear maps, explosive barrels,
the shotgun as the perfect weapon, chainsaw, BFG-style super weapon,
**secrets** behind walls, demons as sprites with 8 rotations, infighting
between monsters, light-level sectors and flickering lights, heavy metal-inspired MIDI.

**Premise.** On the lunar ice mine **Selene-12**, a teleport experiment opens a
door to the **Frozen Abyss**. Horned, frost-burned horrors pour through. A
lone marine-in-a-parka, *the Night Shift*, is all that's left.

**First-person adaptation.** Native, faithful to the *feel*: no mouse-look
by default (optional), auto-aim vertically, very high run speed, momentum stop.

**Goal.** Clear 3 compact maps (Hangar, Coolant Works,
Gate) and close the Abyss gate by killing the **Gatekeeper**.

**20-minute flow.** 3 maps × 5–7 min, with secrets and an end-level tally screen (kills %, items %, secrets %, time vs par).

**Mechanics.**
* Core: fist, pistol, shotgun, double-barrel, chaingun, rocket launcher, plasma, *Frostbringer* (super weapon); keycards; barrels with chain reactions; monster infighting; 7 enemy types (zombie miners, imps of ice, charging brutes, floating eye-maws, burning wisps, heavy knights, the Gatekeeper); radiation-suit puddles of liquid nitrogen; armour & megapickups; 9 secrets; automap.
* Garnish: crushers, teleporters, "nightmare" mode with respawning enemies.

**Look.** 320×200 aspect, strict light banding & distance darkness, flicker and strobe sectors (dynamic lights with shadows from lamps), texture style of grimy tech + icy hell stone (original), nitrogen pools glowing cyan. All monsters are hand-painted 8-direction sprites.

**Stars.** ★ finish · ★★ 100 % kills on one map · ★★★ all secrets and beat par on all maps.

**Multiplayer & XR.** Co-op 2 split / 4 online, deathmatch 4. XR full.

**Bot & tests.** Bot uses a key-dependency planner. Tests: infighting target switch, barrel chain timing, keycard door matrix, tally calculation.

---

## Day 14 — **RING OF THE EVERGREEN**
*Genre homage: Secret of Mana (Square, 1993) · Kit: RPG*

**Inspiration.** A colourful, real-time action RPG for **3 players at once**
(multitap co-op!), the **ring menu** that pauses action, weapon **charge
attacks** (charge meter, level up weapons through use), elemental spirits
granting magic, a lush world with a sacred tree, cute but deadly enemies
(rabbit-like blobs), boss fights against giant creatures, an iconic soundtrack.

**Premise.** The **Evergreen**, the world-tree that keeps winter gentle, is
withering. Three friends (the boy-smith *Tobin*, the bard *Liesl*, the
sprite-child *Pip*: original) must rekindle 3 **seed shrines**.

**First-person adaptation.** You play one of the three in first person; the
others are AI companions (or friends). The **ring menu** appears as a
*holographic wheel of item icons around your view*, pausing time. The
weapon charge meter fills on the bottom of your view; charged swings have
dramatic arcs. Swap characters any time.

**Goal.** Rekindle 3 shrines (forest, frozen lake, crystal cave), then
defeat the **Hollow Stag** at the Evergreen's roots.

**20-minute flow.** Village & forest (5 min) → frozen lake shrine with a
spirit (5 min) → crystal cave shrine (5 min) → boss (5 min).

**Mechanics.**
* Core: real-time melee with stamina/charge %, 4 weapon types (sword, spear, whip for swinging across gaps, bow), weapon levels by use, ring menu (items, magic, weapons, party orders), 2 spirits (frost & bloom) with 3 spells each, party AI (aggressive/defensive), cute enemies with knockback, 3 bosses.
* Garnish: cannon-travel easter egg (shoot yourself across the map), a flying dragon mount at the end.

**Look.** Bright and pastel — a strong contrast to the gloomy portals. Sprites
for characters and enemies with soft shadows, affine textures, painterly
skyboxes, sun shafts through trees (shadowed sun), fireflies as many tiny lights.

**Stars.** ★ beat the Hollow Stag · ★★ no party KO · ★★★ reach weapon level 3 with every weapon.

**Multiplayer & XR.** **Couch co-op 3** (split in 3), 3 online. XR seated.

**Bot & tests.** Bot + 2 AI party members. Tests: charge-meter timing, ring menu pause (sim frozen), weapon level XP, spell element multipliers.

---

## Day 15 — **FROSTFALL PROTOCOL**
*Genre homage: X-COM: UFO Defense / UFO: Enemy Unknown (MicroProse/Mythos, 1994) · Kit: Tactics*

**Inspiration.** **Turn-based squad tactics** with **time units (TUs)**,
reaction fire, **line of sight & fog**, destructible terrain, permadeath,
terrifying alien reveals, crash-site missions at night, plus a strategic
Geoscape layer (bases, research, interceptors).

**Premise.** Strange green lights over Lapland. The international
**Frostfall Protocol** task force investigates a crashed craft of the
**Glimmerkin**, tall silent beings with mirror faces.

**First-person adaptation.** **Turn-based first person**: you *inhabit each
soldier in turn*, seeing only what that soldier sees. Moving spends TUs in
real steps (a ghost trail previews the path and TU cost); shooting shows
hit-chance in the scope. Ending a soldier's turn fades to the next one.
Enemy turn: you watch through the eyes of your soldier who is being shot at
(tense!). A **tactical tablet** gives the top-down map of only explored tiles.

**Goal.** Mission 1: secure the night crash site (kill/stun all Glimmerkin
and recover the **navigation core**). Mission 2: defend your field base from
the retaliation raid. A light strategic interlude between missions (research
one of 3 techs with the recovered core, heal/promote soldiers).

**20-minute flow.** Mission 1 (~10 min), interlude (2 min), Mission 2 (~8 min).

**Mechanics.**
* Core: tile grid with levels, TU costs (walk, kneel, turn, aimed/snap/auto shots), reaction fire based on remaining TUs, accuracy model, cover & destructible walls, flares (dynamic lights!) at night, morale & panic, stun device, 4–6 soldiers with names/ranks, permadeath, enemy AI with flanking.
* Garnish: mind-control enemy in mission 2, proximity grenades.

**Look.** Night snow fields lit by thrown flares, the crashed craft's eerie
green interior glow, soldiers' torches as shadowed spot lights. Low-poly meshes
and voxel terrain props that break.

**Stars.** ★ win both missions · ★★ no soldier lost · ★★★ capture a Glimmerkin alive in mission 1.

**Multiplayer & XR.** Hot-seat 2, co-op 2 online (each owns half the squad). XR tabletop (miniature battlefield) or embodied.

**Bot & tests.** Bot uses utility AI with TU planning. Tests: TU cost table, reaction fire priority, LOS on multi-level grid, permadeath save rules.

---

## Day 16 — **RUNE OF RIME**
*Genre homage: Quake (id Software, 1996) · Kit: FPS*

**Inspiration.** First true-3D polygonal shooter: Lovecraftian
gothic-industrial worlds, brown palette, **slipgates**, a hub with
episode portals, **four runes** to collect, the nailgun, the rocket jump
born here, lightmaps with dark corners, an atmospheric ambient
industrial soundtrack, strong online deathmatch culture.

**Premise.** A cult has driven four **Rime Runes** into the world, freezing
time in four forgotten realms. You, a nameless ranger, step through the
**ice gates**.

**First-person adaptation.** Native.

**Goal.** Recover all 4 runes from 4 short realms, then defeat the **Old Cold**
(a towering ice horror) in the final shrine.

**20-minute flow.** Mini-hub (1 min) → 4 realms (4 min each: a metal base, a
drowned castle, a crystal cathedral, a wind-swept mausoleum) → boss (3 min,
defeated by an environmental trick, not bullets).

**Mechanics.**
* Core: axe, shotgun, double shotgun, nailgun, super nailgun, grenade & rocket launchers, thunder rod; **rocket jumping**, bunny-hop air control, quad damage, invulnerability ("Ward of Midwinter"), water/lava-ice volumes, 8 enemy types (grunt cultists, ice hounds, frost knights, ogre-like brutes with chainsaws, fish, scrags, wraith casters, the giant shambling "Thaw").
* Garnish: secret realm reachable only by rocket jump, gibs that bounce.

**Look.** Low-poly fully 3D monsters, brown/teal palette, chunky
textures, and **lightmap-like dynamic lighting**: torches and flickering lights
with real shadows, plus coloured light (the one portal that shows what
"Quake with real-time shadows" might have looked like).

**Stars.** ★ finish · ★★ all realms without dying · ★★★ find the secret realm.

**Multiplayer & XR.** Deathmatch 4 split / 8 online (the classic). XR full.

**Bot & tests.** Bot including rocket-jump links in nav graph. Tests: rocket-jump impulse, air-control acceleration, quad damage multiplier, rune gate logic.

---

## Day 17 — **WAY OF THE GLOWBLADE**
*Genre homage: Star Wars Jedi Knight: Dark Forces II (LucasArts, 1997) · Kit: FPS + melee*

**Inspiration.** A shooter that turns into a **sword-and-powers** game: an
energy blade that deflects bolts, **mystic powers** (jump, speed, push, pull,
heal, choke/lightning), a **light vs. dark** choice that changes the ending,
seven antagonists dueled one by one, vertigo-inducing vertical levels.

**Premise (original universe).** The monastery of **Kael-Morrow** guards
the **Glowblade** technique, a blade of focused aurora light. A fallen
student, **Vesh**, has turned the frost-dark path and seeks the Heart
Lantern. You, the last novice, take up your master's blade.

**First-person adaptation.** First person throughout; blade combat
uses directional swings (mouse/stick direction = swing direction, or the
motion controller in XR) and timed parries; bolt deflection aims toward
your crosshair.

**Goal.** Ascend the monastery's 3 tiers, defeat 2 lieutenants, then duel Vesh.
Your choices (spare/strike, heal/destroy) shift a **Lantern meter** between
*Dawn* and *Dusk*, unlocking different final powers and endings.

**20-minute flow.** Courtyard & blaster pirates (5 min, learn deflect) → cliff
ascent with Flow-jump puzzles (5 min) → lieutenant duels (5 min) → Vesh (5 min).

**Mechanics.**
* Core: Glowblade (3 swing directions, parry window, bolt deflection), blaster for ranged, 6 *Flow* powers (leap, haste, push, pull, mend; Dawn: shield, Dusk: storm), power points per tier, Lantern alignment, 3 duelists with distinct styles, enemy pirates with blasters.
* Garnish: cockpit fly-in intro, saber-lock clash minigame.

**Look.** Blade is a coloured dynamic light that illuminates the snowy
monastery and casts moving shadows (the iconic effect); prayer flags, mist,
night sky with a big moon. Mesh characters.

**Stars.** ★ finish · ★★ beat Vesh without healing · ★★★ get both endings (2 runs).

**Multiplayer & XR.** Duel 2 split/online. XR **full**: swing with your controller.

**Bot & tests.** Bot with duel AI. Tests: deflection angle math, parry window frames, alignment thresholds, power cooldowns.

---

## Day 18 — **HIVEMIND**
*Genre homage: StarCraft (Blizzard, 1998) · Kit: RTS*

**Inspiration.** Three **strongly asymmetric** races (humans, psionic
aliens, an insectoid swarm); **minerals & gas**, **supply** caps,
**creep** spreading from hive structures, larvae that morph into
anything, burrowing, high APM micro, iconic voice lines and a dramatic
campaign; became an esport.

**Premise.** You are the **Broodmind's eye** of the **Skrael**, a crystal-shelled
swarm awakening under an Antarctic ice shelf, opposed by the
human **Austral Mining Consortium** dug in around a gas geyser.

**First-person adaptation.** You **are a flying hive-eye** (a floating
organic overseer) that hovers over the battlefield at low height. Your hive
structures and the **frost-creep** they spread are your territory; **larvae**
are morphed by looking at them and choosing from a radial "genome" menu.
Commanding the swarm: paint a "pheromone trail" by holding the order button
and sweeping your gaze, swarms follow the trail and attack what lies at its end.
(Asymmetric to Days 3/5/21/23/24, where you command humans.)

**Goal.** Destroy the Consortium's **Command Bastion** and its two outposts.

**20-minute flow.** Awaken (3 min: drones, extractor, spawn pool) → spread
creep to the second crystal field (5 min) → ambush a convoy (4 min) →
morph flyers, break the bastion's siege tanks (8 min).

**Mechanics.**
* Core: 2 resources (crystal & geyser gas), supply via "Brood-Mothers", larvae timer, creep spread & recession, buildings must be on creep, 7 morphs (drone, scuttler, spitter, burrower, siege-beetle, flyer, overseer), burrow (invisible until detected), human AI with bunkers, siege mode tanks, detection towers.
* Garnish: "infest" a captured human building to get special unit, voice of the Broodmind (original, subtitled whispers).

**Look.** Bioluminescent creep spreading as a decal/voxel layer that glows
purple in the dark; human base with floodlights casting long shadows across
the creep. Units are voxel creatures.

**Stars.** ★ win · ★★ win with ≤ 20 supply lost · ★★★ win under 14 min.

**Multiplayer & XR.** 1v1 online lockstep (Skrael vs Consortium playable). XR tabletop.

**Bot & tests.** Bot with build orders. Tests: creep growth over grid, larva queue, supply cap, burrow detection.

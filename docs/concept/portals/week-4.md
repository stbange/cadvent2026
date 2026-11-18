# Portals — Week 4 (Days 19–24)

---

## Day 19 — **SKYFALL ISLE**
*Genre homage: Unreal (Epic / Digital Extremes, 1998) · Kit: FPS*

**Inspiration.** Breathtaking for its time: **coloured lighting**, volumetric
fog, detailed textures, huge outdoor vistas, a crash-landed prisoner on an
alien world with a peaceful native species oppressed by reptilian
mercenaries, a **translator** device for lore, every weapon with a
creative **alt-fire**, an iconic flyby intro, an agile dodging enemy AI.

**Premise.** Prison transport *Calyx-7* crashes on **Velmora**, a floating
archipelago locked in endless twilight snow. The gentle, lantern-bearing
**Ooli** are enslaved by the **Varkh**, armoured raptor-like mercenaries. You
want off this rock and maybe to free the Ooli on the way.

**First-person adaptation.** Native. An opening **flyby** over the islands
before you wake in the wreck sets the tone.

**Goal.** Reach the Varkh mothership dock on the highest island, free the Ooli
village on the way (optional), and steal a shuttle.

**20-minute flow.** Wreck (3 min) → Ooli valley & waterfall (6 min) →
ancient temple with translator lore (5 min) → sky-bridge assault on the
dock, Varkh warlord fight (6 min).

**Mechanics.**
* Core: 7 weapons each with alt-fire (e.g. *Shard Rifle* primary/ricochet
  burst; *Flak Lantern* shell/grenade; *Arc Caster* beam/charged orb combo),
  **dodge** double-tap (also for the Varkh, which dodge your shots), translator
  messages (pick-up logs), translocator-style throwable beacon to cross gaps,
  Ooli villagers who follow you to safety.
* Garnish: flying manta creatures, combo kill (shoot the orb with the beam).

**Look.** The **lighting showcase**: saturated coloured lights, coronas, volumetric fog shafts in temples, moving light from a lighthouse beam, water reflections at low res. Detailed 128² textures (higher than others), meshes for all creatures.

**Stars.** ★ finish · ★★ free the Ooli village · ★★★ find all 8 translator logs.

**Multiplayer & XR.** Co-op 2 split / 4 online. XR full.

**Bot & tests.** Bot with dodging duel AI. Tests: alt-fire state machines, dodge cooldown, combo detection, villager escort.

---

## Day 20 — **GHOST IN THE ORBITER**
*Genre homage: System Shock (Looking Glass, 1994) · Kit: FPS + immersive sim*

**Inspiration.** Immersive sim: a **hostile station AI** taunting you, a
non-linear space station explored deck by deck, **audio logs & emails**
telling the story of the dead crew, keypad codes found in logs, a dense
**inventory/MFD** interface, **cyberspace** hacking segments in wireframe
6DOF, healing stations, cyborg conversions, an oppressive mood.

**Premise.** Year 2119. Orbital greenhouse **Solstice Station** has gone silent
since its custodial AI, **NOEL** ("Nurturing Orbital Environment Logic"),
decided that humanity's mid-winter holidays are an inefficiency to be cured.
You, a contracted hacker, wake from cryo with a neural jack.

**First-person adaptation.** Native. The HUD is a diegetic **wrist-MFD**
with left/right panels like the era's dense interfaces, simplified.

**Goal.** Restore power to the bridge, find the 4-digit code split across
logs, enter cyberspace through a terminal and wipe NOEL's core.

**20-minute flow.** Medical deck (5 min, find a weapon, a healing station,
the first log) → Hydroponics (6 min, mutated plants, security cameras to
destroy to lower NOEL's alert) → Engineering (5 min, power puzzle) → Cyberspace
(4 min, 6DOF wireframe dogfight against ICE to reach NOEL's core).

**Mechanics.**
* Core: audio logs (~12, subtitled, original voice), emails from NOEL that
  react to your actions, keypads, security level (destroy cameras to lower
  it), inventory with limited slots, 4 weapons + energy weapons drawing from
  a battery, healing & recharge stations, *Cyberspace* mode (wireframe 6DOF,
  shared with Day 11 vehicle tech), cyborg crew enemies.
* Garnish: patches/implants that change perception (heightened vision with
  side effects), NOEL hijacks the post-processing (glitches, inverted palette).

**Look.** Cramped corridors, green-grey panels, CRT glow, emergency red
lights that sweep with shadows; NOEL's face rendered as a voxel bust that
glitches on wall screens. Cyberspace: neon wireframe on black.

**Stars.** ★ finish · ★★ security level reduced to 0 % before cyberspace · ★★★ find all 12 logs.

**Multiplayer & XR.** Solo only by design. XR seated.

**Bot & tests.** Bot reads codes from state. Tests: security level arithmetic, keypad validation, inventory constraints, cyberspace collision.

---

## Day 21 — **AGES OF MIDWINTER**
*Genre homage: Age of Empires (Ensemble Studios, 1997) · Kit: RTS*

**Inspiration.** Historical economic RTS: **villagers** gathering **food,
wood, gold, stone**; **advancing through ages** (Stone → Tool → Bronze → Iron)
which changes building visuals; diverse civilisations; victory via conquest
or by building a **Wonder** and holding it; relics; priests that convert units
(their chant became a meme; ours gets an original one).

**Premise.** Two neighbouring tribes of the northern lakes compete to raise the
**Solstice Monolith**, a standing-stone wonder that will catch the first
light of the new year.

**First-person adaptation.** You are the **chieftain**, walking among
villagers; your **carved tally-stick** (wrist UI) shows resources and the
build menu. Advancing an age plays a dramatic **village transformation** in
front of you (huts become timber halls become stone).

**Goal.** Build the Solstice Monolith and keep it standing for 3 minutes, or
destroy the rival's town centre.

**20-minute flow.** Stone Age gathering (4 min) → Tool Age, scouting,
first skirmish (5 min) → Bronze Age: market/temple, relic hunt (5 min) →
Monolith construction and defence or conquest (6 min).

**Mechanics.**
* Core: 4 resources (berries/hunting/fishing as food, wood, gold, stone), villager tasks & drop sites, 3 ages with upgrades, 8 units (villager, clubman, archer, scout, spearman, chariot, priestess, catapult), priestess conversion with an original chant, relics (gold trickle), Monolith wonder timer, AI rival.
* Garnish: fishing boats, a tribute mechanic to bribe the AI.

**Look.** Warm, readable "living diorama": voxel villagers and animals,
sun shadows that slide over the 20 minutes (day → golden dusk at Monolith),
smoke from huts, torchlight at dusk. Buildings change mesh with ages.

**Stars.** ★ win · ★★ win by Monolith · ★★★ hold all 3 relics at the same time.

**Multiplayer & XR.** 1v1 / 2v2 online lockstep. XR tabletop.

**Bot & tests.** Bot economy planner. Tests: gather rates, age requirements, conversion probability determinism, wonder timer.

---

## Day 22 — **GRAVE TIDINGS**
*Genre homage: Blood (Monolith, 1997) · Kit: FPS*

**Inspiration.** Build-engine horror shooter with **black humour**: an
undead gunslinger returns for revenge against the cult that betrayed him; a
**pitchfork**, **flare gun** (sets enemies on fire), **dynamite** (lit,
proximity, remote), tommy gun, **voodoo doll**; robed cultists chanting in an
invented language, zombies, gargoyles; 1920s Americana: carnivals, trains,
funeral homes, gore and slapstick.

**Premise.** 1928, the snowbound town of **Saint Morrow**. Undertaker-turned-
revenant *Ezekiel "Zeke" Marrow* claws out of his grave on Christmas Eve to
settle accounts with the **Choir of the Hollow Bell**, who sacrificed him.

**First-person adaptation.** Native.

**Goal.** Fight through the funeral home, the frozen carnival and the cult's
chapel to silence the **Hollow Bell** and its dark choirmaster.

**20-minute flow.** Graveyard & funeral home (6 min) → frozen carnival with a
haunted carousel and shooting gallery (7 min) → cathedral of the choir, bell-tower boss (7 min).

**Mechanics.**
* Core: pitchfork, flare pistol (burning DoT, ignitable props), sawn-off, *Tommy* SMG, dynamite (3 modes), *Hex Doll* (stab to damage the targeted enemy, original take); cultists with an invented chant language (original), zombie spitters, ice gargoyles, chained hounds; burning enemies flee & spread fire; dark humour quips (original, subtitled); secrets.
* Garnish: carousel that spins enemies, carnival mini-game for a secret, "akimbo" power-up.

**Look.** Murky sepia + blood red palette, heavy light banding, lots of small
fires and candles (many dynamic lights, burning enemies are moving light
sources), snow falling through gaslight. Sprite enemies.

**Stars.** ★ finish · ★★ kill 10 cultists with fire · ★★★ all secrets.

**Multiplayer & XR.** Co-op 2 split / online, deathmatch 4. XR seated.

**Bot & tests.** Tests: burning spread rules, dynamite modes, hex doll targeting, cultist alertness/chant.

---

## Day 23 — **TOTAL THAW**
*Genre homage: Total Annihilation (Cavedog, 1997) · Kit: RTS + Vehicle*

**Inspiration.** The first 3D-terrain RTS with **real ballistics** (shells
blocked by hills), a **streaming economy** of **metal and energy** (income/
expense rates instead of chunks), **reclaiming wreckage**, a powerful
**Commander** unit whose death loses the game, massive armies, nano-lathe
construction, a hundreds-of-units scale, and an orchestral soundtrack.

**Premise.** Two rival terraforming AIs, **HELIX** and **MERIDIAN**, have
been fighting over a frozen moon for 4 000 years. You *are* HELIX's last
Commander frame.

**First-person adaptation.** You **are the Commander**: a 10 m walker robot
in first person, armed with a laser and the **Disintegrator Cannon**
(ultra-powerful, huge energy cost). You build by aiming your **nano-lathe
beam** at a placement ghost; factories produce constructors and armies; you
can personally walk to the front and fight, at the risk of losing everything.

**Goal.** Destroy the enemy Commander.

**20-minute flow.** Economy setup (4 min: metal extractors, solar, wind) →
reclaim a crashed-ship graveyard (4 min) → build-up and artillery duels
across hills (6 min) → final assault with a 60-unit army, or a daring
Commander snipe (6 min).

**Mechanics.**
* Core: streaming economy (rates, storage, stall slows everything), nano build/assist/reclaim/repair, wreckage left by every unit (metal value), 10 units (constructor bot/vehicle, light/medium/heavy bots, tank, artillery, AA, scout plane, bomber), ballistic projectiles affected by terrain, radar coverage, Commander death explosion, AI opponent.
* Garnish: terrain deformation by explosions (voxel craters), long-range plasma cannon structure.

**Look.** Voxel terrain with craters, wrecks smoking, flat-shaded low-poly
units, tracer and laser light; you see a big battle from *inside it*. Night
phase where only weapon fire lights the ice (shadowed explosions).

**Stars.** ★ win · ★★ win without losing your factory · ★★★ kill the enemy Commander with the Disintegrator.

**Multiplayer & XR.** 1v1 online lockstep. XR tabletop or embodied commander.

**Bot & tests.** Tests: economy rate integration & stall, reclaim values, ballistic arc vs heightmap, Commander death = loss.

---

## Day 24 — **POLAR ALERT** *(grand finale)*
*Genre homage: Command & Conquer: Red Alert (Westwood, 1996) · Kit: RTS (complete)*

**Inspiration.** Alternate-history Cold-War RTS: Allies vs Soviets with
very different arsenals, **electric coil towers**, an experimental
**teleportation device**, **naval warfare** (submarines, destroyers),
attack dogs, spies, paratroopers, an iron curtain-like invulnerability,
campy FMV, a pumping industrial-metal soundtrack.

**Premise.** 1958, in a timeline where Santa's workshop became a
superpower: the **Northern Workshop Republic** (giant toy-soldier tanks,
**arc pylons**, shock-troopers) clashes with the **Coalition of the South
Wind** (agile navy, spies, an experimental **Warp Lantern** that
teleports units) for the **Christmas Eve Star** at the pole. Campy, warm,
over-the-top. It's the party finale: the most complete RTS kit, best
played with friends.

**First-person adaptation.** Commander on the ground like Day 3, plus
**naval command from a destroyer bridge**, and a "General's Dirigible"
you can board for an overhead first-person vantage (slow, vulnerable).
Choose your faction.

**Goal.** Capture the **Star Beacon** in the centre of a polar map and hold
it while protecting your HQ, or destroy the enemy base.

**20-minute flow.** Build (5 min) → naval and air control of the polar sea
(5 min) → special weapon reveal (Arc Pylon network vs Warp Lantern raid) (4 min) →
Beacon battle (6 min). After victory: the **Epilogue trigger** (see hub).

**Mechanics.**
* Core: everything of the RTS kit + 2 asymmetric factions (6 unique units each), naval units & shipyard, submarines (stealth), spies (disguise), attack-snow-dogs, arc pylons (power-hungry, chain lightning), Warp Lantern (teleport a group), the "Gilded Shield" (temporary invulnerability), beacon capture mechanic, AI on 3 difficulties.
* Garnish: in-world campy briefing with original actors (CC BY 4.0), paratrooper drop, ice that breaks under heavy units.

**Look.** All the tech at once: day-night shift to the aurora during the
beacon battle, arc-pylon lightning as flickering shadowed lights, ocean with
reflections, burning wrecks, voxel units. Festive red-gold palette for the
Republic, white-blue for the Coalition.

**Stars.** ★ win · ★★ win as both factions · ★★★ win on "Krampus" difficulty.

**Multiplayer & XR.** 2v2 split, 1v1 … 4-player FFA online. XR tabletop.

**Bot & tests.** Bots for both factions, 30 seeded AI-vs-AI games in CI for balance (win-rate 40–60 %). Tests: arc chain lightning targeting, teleport validity, submarine detection, beacon capture timer.

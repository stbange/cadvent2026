# Portals — Week 1 (Days 1–6)

Every portal follows the same template: **Inspiration** (what defined the
classic: our research notes), **Premise**, **First-person adaptation**,
**Goal**, **20-minute flow**, **Mechanics** (Core / *Garnish*), **Cast**,
**Look**, **Snowflakes** (0–3 rating), **Shining Star** (where the portal's collectible star is won), **Friends' marks**, **Multiplayer & XR**, **Bot & tests**.
All names, characters, places and art are original.

---

## Day 1 — **SLEIGHED!**
*Genre homage: Duke Nukem 3D (3D Realms, 1996) · Kit: FPS*

**Inspiration.** A Build-engine shooter famous for (1) a brash, wise-cracking
hero, (2) *interactive, real-world places*: cinemas, strip malls, toilets,
vending machines, pool tables, breakable glass, (3) inventive weapons:
remote pipe-bombs, shrink ray then stomp, freeze gun, trip mines, (4) items like
jetpack, portable medkit, night-vision goggles, (5) rooms-over-rooms
sector design, earthquakes, collapsing buildings, many secrets.
Graphics: 320×200/640×480, sprite enemies with 5/8 rotations, sector lighting,
mirrors, a saturated palette.

**Premise.** *Brick Holloway*, a retired, over-confident mall Santa, finds the
**Glitterdome Mall** overrun by space-pig invaders who are stealing the city's
Christmas lights to power their mothership. He takes it personally.

**First-person adaptation.** Already first person; the focus is
**interactivity density**: at least one usable/breakable thing in every room.

**Goal.** Fight from the parking garage through the mall to the roof and
blow up the **Light Siphon** beaming power to the mothership.

**20-minute flow.**
1. *Parking garage (4 min)*: intro one-liner, shooting basics, first secret behind a cracked wall, elevator.
2. *Mall atrium (8 min)*: three wings (toy store, food court, cinema) to collect the 3 **fuse candles**; non-linear, rooms-over-rooms escalators; cinema projector shows the invaders' broadcast.
3. *Service tunnels (3 min)*: dark, night-vision goggles, trip-wire mines.
4. *Rooftop (5 min)*: jetpack section around the Siphon; boss **Boarlord Overseer** in a mech-sled; destroy 3 emitters with gift-bombs; escape the collapsing dome.

**Mechanics.**
* Core: weapons: *Candy-Cane Kick* (melee), *Snow Pistol*, *Scattershot*, *Gift-Bomb* (throw, remote detonate), *Shrinkray* (shrink then stomp), *Snowblower Chaingun*.
  Interactive props: vending machines (heal), arcade cabinets (score secret), toilets/fountains (+1 HP), breakable glass/vents, light switches that affect enemy sight.
  Jetpack fuel, portable medkit, night vision. 6 secrets.
* Garnish: mirrors (planar reflection at low res), earthquake script, hero quips system (original lines, ~40 one-liners, subtitled), destructible lamps that change dynamic lighting.

**Cast.** Trooper Hogs (pistol), Snowplow Boars (charge), Drone Elves (hijacked, flying), Gremlin Gift-boxes (jump scare mimic), boss Boarlord Overseer.

**Look.** Sprite enemies (8 dirs, normal-mapped), mesh levels with bold
signage textures (fictional shops: "Gnome Depot", "Pretzel Planet"), neon
coloured lights, light banding on, 320×200 feel. Dynamic light from muzzle
flashes, sparking wires, the Siphon's pulsing beam that casts long shadows on the roof.

**Snowflakes.** ❄ finish · ❄❄ under 18 min · ❄❄❄ all 6 secrets.

**Shining Star.** Locked inside the Light Siphon's core: it tumbles free when the Siphon explodes and must be grabbed (jetpack) before the dome collapses.

**Friends' marks.** Toilet-stall graffiti "**Bloodmaker was here**"; the arcade cabinet's high-score table is topped by **DR.DOOM**; the mall's hair salon "**Foen-X Hair Studio**" has dryers you can switch on to blow small enemies away; a wanted poster "**Beware of Messerjocke**" in the security office.

**Multiplayer & XR.** Co-op 2 split / 4 online, "Mall Brawl" deathmatch. XR seated.

**Bot & tests.** Waypoint graph through wings; scenarios: shrink-and-stomp kill, gift-bomb remote detonation radius, vending machine heal cap, fuse-candle gate logic.

---

## Day 2 — **AURORA CARPET**
*Genre homage: Magic Carpet 2 (Bullfrog, 1995) · Kit: Vehicle*

**Inspiration.** Fly a magic carpet over a 3D landscape that you can
**reshape with spells**; collect **mana** orbs by blasting creatures, then claim them
with a spell so **balloons** carry them to your **castle**, which grows as mana
accumulates. Rival wizards steal your mana and attack castles. Spells: fireball,
lightning, raise/lower land, earthquake, volcano, castle, possession, rebound.
MC2 added night levels and underground caverns. Graphics: voxel-ish terrain
heightmap, gouraud shading, red/cyan stereo 3D option.

**Premise.** The aurora is fading because the **Frost Sorcerer Vael** hoards the
north's starlight mana. You, an apprentice skyweaver, take your grandmother's
carpet to the **Shattered Fjords**.

**First-person adaptation.** Direct; the carpet's tassels and your casting
hands are visible. Mouse/stick to steer, two hands = two spell slots.

**Goal.** Own **75 % of the world's mana** (shown as a ring in the sky) while
keeping your castle standing; or destroy Vael's castle for an early win.

**20-minute flow.**
1. *Dusk tutorial (3 min)*: fly, fireball a snow-wolf, release the orb, place your first castle on a plateau.
2. *Expansion (8 min)*: mana nodes across 3 islands; balloons (fragile) ferry mana; Vael's balloons compete; learn *Raise Land* to block raids and *Frost Spire* (tower).
3. *Night falls (5 min)*: dark world lit only by fireballs, castle braziers and aurora; ice caverns hold a *Volcano* scroll.
4. *Siege (4 min)*: Vael attacks; use Volcano on his castle or out-collect him.

**Mechanics.**
* Core: heightmap terrain (257²) with real-time deformation; mana orbs with ownership; balloon AI carriers; castle levels 1–4 (grows with mana, adds defensive turrets); 6 spells in a spellbook (radial), mana as spell fuel; creatures: wolves, snow-griffins, frost-worms.
* Garnish: day/night cycle, possession spell (convert a griffin), anaglyph red/cyan mode as an easter egg.

**Cast.** Vael (rival wizard on an ice-crystal carpet), his balloons, frost-worms that burrow and collapse terrain.

**Look.** Terrain from a voxel/heightmap with vertex-lit gouraud shading,
painted 64² textures blended by height (snow, rock, water). Ocean reflects the
aurora. Fireballs are dynamic point lights that shadow the terrain (shadowed
light #1 = the closest fireball). Castles are voxel models that grow in steps.

**Snowflakes.** ❄ win · ❄❄ castle never below level 2 · ❄❄❄ win in < 15 min.

**Shining Star.** Rises from the aurora's apex when you pass 75 % mana (or from the ruins of Vael's castle); fly straight up to catch it before it fades.

**Friends' marks.** An island shaped like a dagger on the map, "**Messerjocke's Rock**"; giant terrain runes **C6P** visible only from high altitude; Vael taunts that "even **Dr. Doom** couldn't hold this much mana".

**Multiplayer & XR.** Versus 2 split / 4 online (rival wizards). XR seated (carpet = cockpit).

**Bot & tests.** Bot alternates harvest/defend utility AI. Tests: terrain deformation conserves bounds, mana ownership transfer, balloon pathing over raised land, 75 % victory threshold.

---

## Day 3 — **FROSTFRONT**
*Genre homage: Command & Conquer (Westwood, 1995) · Kit: RTS*

**Inspiration.** The template for modern RTS: base building with a **sidebar
build menu**, a single resource (**tiberium**) collected by harvesters,
power plants, construction yard, infantry/tanks/air, two asymmetric factions,
FMV briefings, campaign map, "Construction complete" EVA voice, fog of war shroud.
Graphics: 640×400 pre-rendered sprites, top-down isometric-ish view.

**Premise.** Glowing **Frostite** crystals are spreading across the tundra,
mutating the ice. The *Polar Accord* sends you, Field Commander, to stop the
*Order of the Pale Sun* from weaponising it.

**First-person adaptation.** You are **on the battlefield in a command
armour suit**. Your **wrist tablet** is the sidebar (build queue, power,
credits, minimap). Buildings are placed by pointing a **holo-placement beam**;
units obey **point-and-command**: look at units, select (click / sweep
gesture), look at ground or enemy, order. A **recon drone** key lifts your
view 30 m up for 3 s (cooldown) as a nod to the overhead classic. You can
fight with a sidearm but are fragile: commanding is the point.

**Goal.** Destroy the Order's **Ember Spire** HQ.

**20-minute flow.**
1. *Landfall (3 min)*: MCV-style "Mobile Yard" deploys; build power + refinery; harvester drives off to crystal field.
2. *Build-up (7 min)*: barracks, factory; first raids from the north; protect harvester.
3. *Push (6 min)*: take a bridge, capture a tech building with an engineer (gives a guard tower).
4. *Spire (4 min)*: lancer pylons guard the HQ; call an "Orbital Mirror" strike (charged superweapon) or storm it.

**Mechanics.**
* Core: one resource; power balance (low power ⇒ slower build, defences off); 8 unit types (riflemen, rocket troops, engineer, harvester, light tank, heavy tank, APC, recon buggy); 6 structures; control groups 1–4; rally points; fog of war; AI opponent with build orders and attack waves. Crystal fields hurt infantry.
* Garnish: in-world CRT briefing with talking head (original character), one-liner unit acknowledgements, superweapon.

**Look.** Low-poly meshes and voxel units at a human scale (tanks feel big
when you stand next to them); crystals emit green-cyan light and are the
scene's main dynamic light sources at dusk; muzzle flashes and burning
wrecks add more. Tablet UI uses chunky 90s sidebar icons (original).

**Snowflakes.** ❄ win · ❄❄ harvester never lost · ❄❄❄ win under 15 min.

**Shining Star.** Sealed in the Ember Spire's vault; after the Spire falls, an engineer must carry it out to you.

**Friends' marks.** The recon buggy's callsign is "**X-Ray Junkie**" (it reveals shroud); the capturable tech building is the "**Dr. Doom Research Lab**"; an enemy radio intercept: "**Bloodmaker** squad, hold the bridge!".

**Multiplayer & XR.** 1v1 online lockstep. XR tabletop war-table.

**Bot & tests.** Bot runs scripted build order + attack-move. Tests: power math, harvester state machine (seek/harvest/return/unload), placement validation (adjacency, overlap), lockstep hash equality.

---

## Day 4 — **ICE TOMB OF THE WINTER KING**
*Genre homage: Tomb Raider (Core Design, 1996) · Kit: Platformer*

**Inspiration.** Third-person action-adventure built on a strict **block
grid**: running jumps precisely sized to tiles, ledge grabbing and shimmying,
pushing blocks, levers, timed doors, swimming with an air meter, dual pistols
with auto-aim, wolves, bats, bears and a big dinosaur surprise; lonely ancient
tombs, minimal music, discovery moments.

**Premise.** *Mara Voss*, an explorer-cartographer (original), searches a
glacier tomb for the **Winter King's three Sun Gears**, said to bring back the
spring.

**First-person adaptation.** First-person *parkour* with visible hands
and a **grid-aware traversal system**: jumps snap to tile lengths, holding
"grab" catches ledges; climbing view tilts. A short third-person
"photo mode" pull-out in safe zones gives a nod to the classic view.

**Goal.** Retrieve 3 Sun Gears and place them in the solar door to escape.

**20-minute flow.**
1. *Entrance cavern (3 min)*: jumping tutorial, first wolves.
2. *Hall of Pillars (6 min)*: pushable block puzzle, lever + timed door, Gear 1.
3. *Flooded crypt (5 min)*: swimming, air pockets, underwater lever, Gear 2.
4. *Frozen colossus (6 min)*: a giant awakening ice beast (the "surprise" boss) chases you through a collapsing gallery to Gear 3, then the solar door.

**Mechanics.**
* Core: 1024-unit grid (1 tile = 1 m), standing jump 1 tile, running jump 2 tiles, ledge grab, shimmy, pull-up, block push/pull, levers, timed doors, swimming with air meter, two pistols (infinite ammo) + auto-aim lock, health packs, save crystals.
* Garnish: swan dive, secret dragons (3 jade ornaments), grappling icicles.

**Cast.** Wolves, ice bats, a frost bear, the Colossus.

**Look.** Big monumental rooms, textured with 64² stone/ice tiles that make
the grid readable (classic level-design trick), affine texture wobble on,
torch light with shadows on stairs, caustics in water, cold blue fog.

**Snowflakes.** ❄ finish · ❄❄ ≤ 3 save crystals used · ❄❄❄ all 3 secret jade ornaments.

**Shining Star.** The three Sun Gears lock into the solar door and form the Star; it hovers in the light beam as the door opens.

**Friends' marks.** A carving next to a swinging-blade trap: "**Beware of Messerjocke**"; an earlier explorer's journal signed **Foen-X** hints that warm air melts the ice wall (the puzzle solution); scratched tally marks "**C6P** — day 41".

**Multiplayer & XR.** Co-op 2 online (puzzles need 2 levers in co-op variant), ghost race. XR seated with comfort vignette.

**Bot & tests.** Bot follows authored traversal graph (jump/grab edges). Tests: jump distance per tile class, ledge-grab tolerance window, air meter drowning, block push collision.

---

## Day 5 — **HEARTHGUARD**
*Genre homage: Warcraft II: Tides of Darkness (Blizzard, 1995) · Kit: RTS*

**Inspiration.** Fantasy RTS with **gold, lumber and oil**, peasants that chop
trees (forests are a resource *and* terrain), farms for supply, town hall
upgrades, naval units, a fog of war that hides re-explored terrain, humorous unit
voices when clicked repeatedly, two mirrored factions, sappers and dragons.

**Premise.** The gnome village of **Kindlewick** must survive the
**Troll raiders of the Rimeback Clans** until the midwinter fire is lit.

**First-person adaptation.** You are the **village Captain**, a hero unit.
Commands come from a **signal horn** and pointing; the build menu is a
carved **command stave** (radial). Workers respond with original funny barks
(repeated clicking gets increasingly annoyed lines; subtitled).

**Goal.** Survive 3 raids, then burn the trolls' **Longhall** across the frozen lake.

**20-minute flow.**
1. *Settle (4 min)*: town hall, gold mine, chop the forest edge, farms.
2. *Raid 1 & 2 (8 min)*: build barracks, archers, a watchtower; repair walls.
3. *The ice lake (4 min)*: the lake thaws in spring rhythm; build an ice-breaker boat (the "oil" stand-in is **whale tallow** from a coastal platform) to ferry troops.
4. *Longhall (4 min)*: siege ballista + your hero charge.

**Mechanics.**
* Core: 3 resources (gold, lumber, tallow); supply via farms; worker AI; 6 units (worker, footman, archer, knight, ballista, ferry), 7 buildings, town hall upgrade; hero with a shield-bash; fog of war; raid waves.
* Garnish: annoyed-bark easter egg, chopping changes forest pathing, gryphon-esque snow-owl scout.

**Look.** Stylised fantasy, voxel units with bold silhouettes and team
colours, warm firelight from homes vs cold blue night; trolls carry torches
(their lights warn of raids). Thick forests as instanced sprites with shadows.

**Snowflakes.** ❄ win · ❄❄ no building lost in raids · ❄❄❄ win with hero alive and ≤ 20 units built.

**Shining Star.** Appears in the midwinter fire pit when the Longhall burns; the Captain must bring it home to the hearth.

**Friends' marks.** Workers bark "**Messerjocke** sharpened my axe!"; the troll warlord is **Bloodmaker the Unwashed**; the snow-owl scout is called **X-Ray Junkie**; the tavern sign reads "**Foen-X**: Warm Mead, Hot Air".

**Multiplayer & XR.** 1v1 / 2v2 online lockstep. XR tabletop.

**Bot & tests.** Bot build order + defence. Tests: tree depletion & pathing update, supply cap, raid timers, ferry loading.

---

## Day 6 — **IRON YULE**
*Genre homage: MechWarrior 2: 31st Century Combat (Activision, 1995) · Kit: Vehicle*

**Inspiration.** Pilot a giant walking mech from the **cockpit**: legs and
torso are controlled separately (**torso twist**), throttle, **heat
management** (weapons heat you, overheating shuts down), **weapon groups** and
chain fire, **component damage** (lose an arm, its weapons are gone; leg damage
slows you), jump jets, mech lab customisation, nav points, flat-shaded
polygon worlds, moody synth-orchestral soundtrack.

**Premise.** Pilot *Sable* of the **Hollowfrost Militia** defends the
Winterhold valley from the corporate strip-miners of **Grail-Rex Industries**.
The valley's lifeline is **Santa Claude**, a jovial, slightly too
helpful convoy AI who drives the red-and-white supply crawler *Sleighbell*.
His "beard" is a white antenna array, he speaks in calm, polite, very
thorough radio messages ("I've considered three routes; the safest one is also
the most scenic. Ho ho ho!"), and he always delivers.

**First-person adaptation.** Native (cockpit). Physical cockpit with
MFDs, a throttle lever, heat gauge and damage silhouette; in XR the cockpit
is fully interactive.

**Goal.** A 3-mission mini-campaign: (1) escort Santa Claude's supply crawler
through an ambush canyon, (2) destroy the ice dam pump station while Santa Claude
relays enemy positions, (3) duel the enemy ace "Krampus" walker; Santa
Claude's crawler parks at the crater rim and air-drops coolant canisters
if you ask nicely over the radio.

**20-minute flow.** ~5 min mech lab tutorial & mission 1, ~8 min mission 2,
~7 min mission 3 (duel in a frozen crater with steam vents that cool you).

**Mechanics.**
* Core: legs/torso separation, throttle with reverse, heat model (sinks, vents, snow/water cools), 3 weapon groups (energy, ballistic, missile), lock-on for missiles, 8 component hit zones, jump jets, nav points, simple mech lab (swap 4 hardpoints within tonnage).
* Core (Santa Claude): escort AI with its own HP and route choice, radio advice (subtitled, original lines), coolant air-drop support call (1 per mission). If the crawler is destroyed, mission 1 restarts from the last checkpoint with Santa Claude saying "Let's try that again, I believe in you."
* Garnish: night mission with thermal vision, ejection seat on death, salvage, Santa Claude's "present" crates hiding upgrade parts.

**Look.** **Flat-shaded** low-poly terrain and mechs (era signature), fog
banks, searchlights from the mech's shoulders (shadowed spot lights), PPC-style
arcs lighting the snow, missile trails. Cockpit glass grime and MFD green.

**Snowflakes.** ❄ complete campaign · ❄❄ never shut down from heat · ❄❄❄ finish duel without losing a component.

**Shining Star.** A gift from **Santa Claude** at the end of mission 1, handed over from the crawler's cargo bay with a "ho ho ho".

**Friends' marks.** Your wingmen are **C6P** and **Foen-X** (Foen-X's walker vents hot air from oversized jump jets); the mech-lab paint scheme "**Dr. Doom** Black"; a destroyed enemy walker stencilled "**Bloodmaker** was here".

**Multiplayer & XR.** Versus 2 split / 4 online arena. XR full (seated cockpit, grab throttle).

**Bot & tests.** Bot uses steering + heat-aware fire control. Tests: heat accumulation/dissipation, torso twist limits, component destruction removes weapons, missile lock time.

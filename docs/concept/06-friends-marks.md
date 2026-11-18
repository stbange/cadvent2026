# 6 · Friends' Marks: the crew is everywhere

The calendar is a gift for a specific group of friends, so their gamer tags
are hidden throughout all 24 portals, the way 90s games hid developer names,
graffiti and in-jokes in their levels.

**The crew:** **Dr. Doom** · **Bloodmaker** · **C6P** · **X-Ray Junkie** · **Messerjocke** · **Foen-X**

## 6.1 Rules

* **Text and voice only.** The names appear as tags, signs, graffiti, gravestones,
  unit/bot/soldier names, log authors, credits and radio callsigns. No friend is
  drawn as a recognisable character. Tags like *Dr. Doom* are shown only as
  the friend's gamer tag, never with a comic-book character's look.
* **Affectionate.** Friends can be rivals or villains (the troll warlord
  *Bloodmaker the Unwashed*), but never in a mean-spirited way.
* **Each friend gets a running gag** so the marks feel personal:

| Friend | Running gag | Typical mark |
|---|---|---|
| **Dr. Doom** | mad scientist, things go wrong in their lab | "Dr.Doom did this", "Dr. Doom Research Lab — DO NOT ENTER" |
| **Bloodmaker** | leaves a mess everywhere, gravestones | "Bloodmaker was here", "Here lies Bloodmaker, he made too much" |
| **C6P** | a fussy little robot or maintenance droid model | maintenance bot C6P, "another C6P-model mistake" |
| **X-Ray Junkie** | sees through everything: scouts, scanners, radar | recon callsign, radiology lab, agent with wall-vision |
| **Messerjocke** | knives and blades | "Beware of Messerjocke", knife-grinder, blade traps |
| **Foen-X** | hot air and hair dryers | "Foen-X Hair Studio", warm-wind scrolls, hot-air jump jets |

* **Data-driven.** Names live in one file, `assets/text/friends.cfg` (display
  name + short tag variant, e.g. `DR.DOOM` for 7-character high-score tables).
  Swapping in another group's names is a one-file change, so the project stays
  reusable as an open-source gift.
* **Collectible.** Every mark is a "mark entity" with an id; looking at it
  from close range (or reading the log) registers it. Found marks add notches to
  the friend's plaque on the **Wall of Legends** in the hall. Finding all marks
  of one friend lights that plaque in gold and unlocks that friend's tag as an
  avatar nameplate colour. Marks are optional and not needed for Shining Stars.

## 6.2 Where every friend appears

Full descriptions are in each portal's *Friends' marks* line. Every friend
appears in at least 12 portals; Days 7, 15, 16, 24 feature the whole crew.

| Day | Dr. Doom | Bloodmaker | C6P | X-Ray Junkie | Messerjocke | Foen-X |
|---|---|---|---|---|---|---|
| 1 Sleighed! | arcade high score | toilet graffiti | | | wanted poster | hair studio |
| 2 Aurora Carpet | Vael's taunt | | terrain runes | | island name | |
| 3 Frostfront | research lab | radio intercept | | recon callsign | | |
| 4 Ice Tomb | | | tally scratches | | blade-trap carving | explorer journal |
| 5 Hearthguard | | troll warlord | | owl scout | worker bark | tavern sign |
| 6 Iron Yule | paint scheme | wreck stencil | wingman | | | wingman |
| 7 Tinsel Strike | bot + sprays | bot | bot | bot | bot (knife) | bot |
| 8 Cascade | lab door | | maintenance bot | radiology lab | | |
| 9 Skifrost | | kill-feed | bot | bot | | halfpipe |
| 10 Neon Syndic | | | neon ad | agent codename | alley graffiti | |
| 11 Deep Thaw | | mine shaft | miner tag | miner tag | automap note | |
| 12 Below the Chapel | | gravestone | | | unique knife | healer |
| 13 Hellfrost Station | frost scrawl | par time joke | dog tags | | | |
| 14 Ring of the Evergreen | tough rabbit-blob | | | | knife-grinder | innkeeper |
| 15 Frostfall Protocol | soldier | soldier | soldier | soldier | soldier | soldier |
| 16 Rune of Rime | rune lab sign | wall graffiti | bot | bot | bot | bot |
| 17 Way of the Glowblade | | | training remote | | Wall of Masters | Wall of Masters |
| 18 Hivemind | enemy general | | | | fort name | camp name |
| 19 Skyfall Isle | | | chalk tag | translator logs | wanted poster | |
| 20 Ghost in the Orbiter | audio logs | audio logs | NOEL insult | | | locker label |
| 21 Ages of Midwinter | | rival chieftain | | relic | relic | relic |
| 22 Grave Tidings | gravestone | gravestone | | | knife booth | high score |
| 23 Total Thaw | MERIDIAN signature | | wreck name | radar wreck | | |
| 24 Polar Alert | AI general + credits | AI general + credits | AI general + credits | AI general + credits | AI general + credits | AI general + credits |

## 6.3 Santa Claude (Day 6 guest star)

Besides the crew, **Santa Claude** appears in *Iron Yule* (Day 6): a calm,
very polite, very thorough convoy AI driving the red-and-white supply crawler
*Sleighbell*, with a white antenna array for a beard. You escort him, he
relays intel and air-drops coolant, and he hands over Day 6's Shining Star.
He also has one cameo line on the hall's notice board on 6 December
("Remember to set your star into the arch. Ho ho ho!").

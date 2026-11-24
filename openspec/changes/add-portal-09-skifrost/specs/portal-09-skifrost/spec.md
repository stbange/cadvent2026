# Spec Delta

## Purpose
Day 9, Skifrost: Capture the Flag on Snowglobe Basin between the Rime Wolves and the Ember Owls.

## ADDED Requirements

### Requirement: Training course
The portal SHALL open with a skippable course: ski to 100 km/h, jet to a beacon, hit a circling drone with a disc; then a countdown to the match.

#### Scenario: Training course
- **WHEN** the player skis down the hill, reaches the beacon and the drone is hit, or presses use to skip
- **THEN** the steps advance in order and the match counts down and starts with 20 minutes

### Requirement: Capture the flag
Two hill-top bases SHALL each hold a flag on a low stand that skiers glide over; first to five captures wins, or the lead after 20 minutes, with sudden death on a tie. A checkpoint SHALL be saved after each of your captures and resume with them kept and the Owls back at zero.

#### Scenario: Capture the flag
- **WHEN** the player snatches the Owls' flag at speed and brings it home, once with their own flag stolen
- **THEN** the capture counts only with the own flag home, the speed is recorded and the checkpoint advances

#### Scenario: Flag return timer
- **WHEN** the player tosses the flag down a slope and walks away, then an Owl touches a dropped flag
- **THEN** it falls to the snow, goes home after 30 seconds, and an Owl's touch returns it at once

#### Scenario: Time and sudden death
- **WHEN** the clock runs out level, ahead or behind, or the Owls reach five
- **THEN** sudden death starts, the lap starts, or the match is lost

### Requirement: Bases
Each base SHALL have a generator powering a turret and an inventory station; destroyed generators SHALL go dark and be repaired after 50 seconds. The station SHALL cycle light (+energy pack), medium (+energy pack) and heavy armour and refill.

#### Scenario: Generator powers turret
- **WHEN** an Owl stands in range of the Wolves' turret, the generator is destroyed and later repaired, and the Owls' generator takes a disc
- **THEN** the turret fires, falls silent, the station is dark, power returns, and the Owls' generator goes down

#### Scenario: Station changes armour
- **WHEN** the player uses the station three times
- **THEN** the armour cycles, the weapon slots follow, and the heavy armour comes full

### Requirement: Weapons
The disc SHALL fly at 65 m/s plus half the shooter's speed and explode with splash and knockback (a little self damage: disc jumps); the chaingun SHALL spin up and fire hitscan rounds; grenades SHALL arc, bounce and burst on a fuse. Explosives SHALL damage enemy generators. A mid-air disc kill SHALL be announced in the kill feed.

#### Scenario: Discs splash and push
- **WHEN** the player discs an Owl, an airborne Owl, and the snow at their own feet
- **THEN** the direct hit hurts, the mid-air kill is announced as a special, the Owl respawns at home, and the disc jump lifts the player

#### Scenario: Chaingun and grenades
- **WHEN** the player spins the chaingun on an Owl and on the generator, then lobs a grenade
- **THEN** both take damage, the barrel winds down, and the grenade bounces before it bursts

### Requirement: Victory lap and scoring
After the win the Shining Star SHALL appear at centre field; carrying it to your stand wins the portal. Snowflakes: 1 for the win, +1 for a 5-0 win, +1 for a capture carried faster than 150 km/h.

#### Scenario: Victory lap
- **WHEN** the portal resumes at the lap and the player carries the star home
- **THEN** the portal is won with the star and the snowflakes, and resumes keep the right score

#### Scenario: Marks and queries
- **WHEN** the player meets the friends on both teams and skis into the halfpipe
- **THEN** each mark is found once and the scripting queries report the state

#### Scenario: Bot wins on every difficulty
- **WHEN** the completion bot plays from the start and from a checkpoint on each difficulty
- **THEN** it wins the match and carries the star home

#### Scenario: The match from the hall
- **WHEN** the completion bot enters portal 9 on 9 December
- **THEN** it trains, wins the match and takes the star

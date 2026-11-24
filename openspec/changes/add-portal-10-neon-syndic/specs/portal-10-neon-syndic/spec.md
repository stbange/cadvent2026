# Spec Delta

## Purpose
Day 10, Neon Syndic: a night in Nivalis cutting Halcyon's cheer-broadcast relays with a cell of four.

## ADDED Requirements

### Requirement: The cell
Three AI agents SHALL follow in formation and take orders from quick keys or the radial ring: follow, hold, attack the target under the crosshair, take cover; an attack order with no target SHALL be refused and end when the target falls. Agents can be lost.

#### Scenario: Squad orders
- **WHEN** the lead walks, orders hold, picks cover on the ring and orders attacks with and without a target
- **THEN** the cell keeps formation, stays put, takes cover, refuses, then attacks and returns to follow

#### Scenario: Losing the lead
- **WHEN** the lead stands in the turret gauntlet, or an agent is left there hurt
- **THEN** the portal is lost when the lead falls, and a fallen agent is announced and counts as lost

### Requirement: The Charm Beacon
Holding the beacon SHALL convert civilians nearby into a following crowd whose allowance grows with its size, and sway guards once the crowd is big enough; followers SHALL draw enemy fire.

#### Scenario: Charm beacon crowd
- **WHEN** the lead holds the beacon in the market crowd and near a guard
- **THEN** the crowd grows past thirty, the guard joins, and the record counts for the snowflake

### Requirement: Overclock
Shift, C and Tab SHALL toggle the speed, aim and awareness sliders; speed makes the lead faster, awareness marks hostiles through walls; an empty pool crashes the lead (slow) for a while.

#### Scenario: Overclock sliders
- **WHEN** the lead runs with speed on, turns all three on until the pool is empty, and runs during the crash
- **THEN** the run is faster, the crash comes and refuses the sliders, the run is slower, and it ends

### Requirement: Relays and the city
The three relays SHALL be hacked in order (market, factory, tower) with the timing ring; misses raise the alert; the tower relay SHALL stay locked while its turrets stand. Guards patrol calmly until the alert rises; an alarm brings vans of reinforcements and scatters civilians; the alert cools when nothing happens. Facades break under laser and gauss; a fast hover-car SHALL break the road barrier and open the road.

#### Scenario: Hacking relays
- **WHEN** the lead tries the tower relay early, misses once, abandons, and hacks the market relay
- **THEN** the early relay refuses, the miss raises the alert, and the relay falls with checkpoint 1

#### Scenario: Car rams barrier
- **WHEN** the lead drives the hover-car into the road barrier at speed and into a wall
- **THEN** the barrier breaks and opens the road, the car cannot be left at speed, and the wall stops it

#### Scenario: Facades and gauss
- **WHEN** the lead lasers and then shoots a gauss slug at a shop front
- **THEN** chunks come away, a slug is spent, and an empty gauss says so

#### Scenario: Alert and reinforcements
- **WHEN** the lead shoots a guard, the alert reaches an alarm, then everything calms down
- **THEN** guards were calm before, the alert rises, a van unloads, civilians flee, and the alert cools

### Requirement: Star and extraction
Cutting relay 3 SHALL open the antenna casing; the Shining Star inside heals the cell; the VTOL lands after a minute held near the plaza pad, with waves of guards; boarding wins. Snowflakes: 1 complete, +1 no agent lost, +1 thirty or more followers at once. Checkpoints after each relay and the star.

#### Scenario: Tower and star
- **WHEN** the turrets fall and relay 3 is hacked
- **THEN** the casing opens, the star is taken, the plaza act starts with checkpoint 3

#### Scenario: Extraction
- **WHEN** the lead holds near the pad until the VTOL lands and boards
- **THEN** the portal is won with the star, and waves come while the VTOL is out

#### Scenario: Marks and queries
- **WHEN** the lead passes the neon ad and the back-alley graffiti with custom friends
- **THEN** the marks are found with the friends' names and the scripting queries report the state

#### Scenario: Bot wins on every difficulty
- **WHEN** the completion bot plays from the start and from checkpoints on each difficulty
- **THEN** it wins with the star

#### Scenario: The night from the hall
- **WHEN** the completion bot enters portal 10 on 10 December
- **THEN** it cuts the relays, takes the star and boards the VTOL

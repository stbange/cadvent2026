# Spec Delta

## Purpose
The party for the RPG / tactics kit: squad orders, formation, overclock sliders and the radial ring.

## ADDED Requirements

### Requirement: Formation
Squad members SHALL take wedge slots behind the leader, in pairs to either side with an odd last member centred, turning with the leader.

#### Scenario: Squad formation
- **WHEN** slots are asked for a squad of three behind a leader facing one way, then the other
- **THEN** the pair stands behind to either side, the third further back in the middle, and the wedge turns round

### Requirement: Overclock
Each active slider SHALL drain the stamina pool; a dry pool SHALL switch every slider off and crash for a set time in which sliders refuse to move and levels read negative; the pool SHALL recover, at half rate during the crash.

#### Scenario: Overclock crash timing
- **WHEN** two sliders run on a full pool
- **THEN** it crashes when the pool is empty, the crash lasts its length, and the pool refills afterwards

### Requirement: Radial ring
A ring SHALL pick the sector the look points at once outside a dead zone, sector 0 up and clockwise, and return it on closing; a closed ring chooses nothing.

#### Scenario: Radial ring
- **WHEN** the look moves up, right, down and left with the ring open, then the ring closes
- **THEN** the sectors follow and the last one is chosen

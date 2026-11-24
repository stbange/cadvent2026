# Spec Delta

## Purpose
Story-driven shooter pieces for the FPS kit: triggers, followers, squad AI and chargers.

## ADDED Requirements

### Requirement: Sequence triggers
A trigger SHALL fire on the tick a point enters its volume; a one-shot trigger SHALL fire only on the first entry, a re-entrant one on every entry, and neither SHALL fire again while the point stays inside.

#### Scenario: Sequence triggers reentrancy
- **WHEN** a point enters, stays, leaves and enters again a one-shot and a re-entrant trigger
- **THEN** the one-shot fired once and the re-entrant one twice

### Requirement: Followers
A follower SHALL wait until told to follow, then walk the grid around walls to keep its distance from the leader, and SHALL walk to a given goal and report arriving there.

#### Scenario: Follower pathing to scanner
- **WHEN** a follower is told to follow a leader behind a wall, then sent to a scanner
- **THEN** it reaches the leader through the gap and arrives at the scanner

### Requirement: Squad AI
A soldier SHALL retreat when badly hurt, flank a target that slipped into cover and flush it out with a grenade when it stays hidden, flank when one squad mate already fights, suppress when two do, engage otherwise, and patrol when there is nothing to fight.

#### Scenario: Squad AI state transitions
- **WHEN** the view of one soldier changes through the cases
- **THEN** the state follows: patrol, engage, flank, suppress, retreat, flank, grenade

### Requirement: Chargers
A charger SHALL top a value up at its rate, never above the maximum, and only from what is left in its reserve.

#### Scenario: Suit charger limits
- **WHEN** a charger with a small reserve fills a suit over several seconds
- **THEN** the rate caps each second, the reserve runs dry, and the suit never exceeds full

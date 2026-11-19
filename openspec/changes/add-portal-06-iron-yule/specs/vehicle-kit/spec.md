# Spec Delta

## Purpose
Terrain and flight shared by the vehicle portals (carpet, mech, skis, ships).

## ADDED Requirements

### Requirement: Walker locomotion
A walker SHALL ease towards the speed its throttle sets (reverse at a fraction of full speed, slowed by leg damage), turn its legs and its torso independently within a twist limit, refuse slopes steeper than its climb limit, and rise on jump jets while fuel lasts.

#### Scenario: Throttle and reverse
- **WHEN** the throttle is pushed to full, then all stop, then full reverse
- **THEN** the walker reaches full speed, stops, and backs up at half speed

#### Scenario: Torso twist is independent
- **WHEN** the legs turn, then the torso twists past its limit, then centres
- **THEN** the legs keep their heading, the twist stops at the limit and returns to zero

#### Scenario: Slopes and jump jets
- **WHEN** a walker marches into a cliff, then fires its jets
- **THEN** it stops at the foot, then rises, heats up and burns fuel, lands and refuels

### Requirement: Heat
Heat SHALL be shed every second by the sinks (times any cooling bonus); reaching the capacity SHALL shut the walker down until it is cooler than its restart heat for at least two seconds.

#### Scenario: Overheating shuts down
- **WHEN** heat reaches the capacity
- **THEN** the walker shuts down, ignores controls, and restarts once cool; extra cooling sheds heat faster

### Requirement: Component damage
Damage SHALL land on one of eight components chosen by where a hit strikes; a destroyed arm or leg SHALL pass the rest inwards, a destroyed side torso SHALL take its arm, and losing the head, the centre torso or both legs SHALL destroy the walker.

#### Scenario: Component damage
- **WHEN** an arm, a side torso and a leg are shot away, then the centre torso
- **THEN** the overflow reaches the torso, the arm falls with its side, the walk slows, and the walker is destroyed

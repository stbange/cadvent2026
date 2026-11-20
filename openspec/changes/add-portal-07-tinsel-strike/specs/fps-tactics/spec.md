# Spec Delta

## Purpose
Tactical shooter pieces for the FPS kit: recoil, inaccuracy, economy, smoke and bot paths.

## ADDED Requirements

### Requirement: Recoil patterns
A spray SHALL follow a fixed pattern: the first round goes where the weapon points, the next rounds climb and then drift sideways, and the same shot index SHALL always give the same offset.

#### Scenario: Recoil pattern determinism
- **WHEN** the offsets of a 30-round spray are computed twice
- **THEN** they match shot for shot, start at zero, climb, then sway

### Requirement: Movement inaccuracy
The cone of inaccuracy SHALL grow with speed (as a fraction of full speed, clamped), grow more in the air, and halve when crouched.

#### Scenario: Movement inaccuracy
- **WHEN** a weapon is fired standing, moving, jumping and crouching
- **THEN** the spread is smallest crouched and still, larger moving, largest airborne

### Requirement: Economy rules
Round rewards SHALL depend on the result and reason (elimination, bomb, defuse, time), losses SHALL pay a base plus a step per consecutive loss up to a cap, attackers who planted SHALL get a bonus even when they lose, and money SHALL stay between zero and the maximum.

#### Scenario: Economy rules table
- **WHEN** wins, losses with streaks, planted losses and money near the cap are paid out
- **THEN** each payout matches the rules table

### Requirement: Smoke and the walk grid
Smoke SHALL block any sight line through its sphere. The walk grid SHALL give 8-way paths without corner cutting, refuse unreachable or over-long goals, and mark walkable cells beside walls as cover.

#### Scenario: Smoke blocks line of sight
- **WHEN** a sight line passes through, beside or short of a smoke cloud
- **THEN** only the line through it is blocked

#### Scenario: Walk grid paths and cover
- **WHEN** a wall with a gap separates two cells, and the gap is then closed
- **THEN** the path goes through the gap, then none exists; cover lies along the wall

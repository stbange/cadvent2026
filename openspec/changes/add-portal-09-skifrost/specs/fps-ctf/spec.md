# Spec Delta

## Purpose
Capture the Flag pieces for the FPS kit: flags, captures, generators and turrets.

## ADDED Requirements

### Requirement: Flags
A flag SHALL be taken by an enemy touch (at home or dropped), dropped with half the carrier's speed, returned home by a team mate's touch or after lying 30 seconds.

#### Scenario: Flag return timer
- **WHEN** a carried flag is dropped and left alone, then dropped again and touched by each side
- **THEN** it goes home after 30 seconds, a team mate returns it at once and an enemy picks it up

### Requirement: Captures
A carrier SHALL capture by reaching their own flag's stand, and only while their own flag is at home; the captured flag SHALL go home.

#### Scenario: Capture needs own flag home
- **WHEN** a carrier reaches their stand without, then with their own flag home
- **THEN** only the second counts and the enemy flag returns

### Requirement: Base power
Turrets SHALL fire only with their generator running, inside their range and when reloaded; a destroyed generator SHALL come back after its repair time.

#### Scenario: Generator powers turrets
- **WHEN** a turret has a target in range while its generator runs, is destroyed and is repaired
- **THEN** it fires on its cooldown, stays silent while dark, and fires again after the repair

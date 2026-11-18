# Spec Delta

## Purpose
Day 3, Frostfront: the Field Commander of the Polar Accord stops the Order of the Pale Sun from weaponising Frostite, commanding on foot.

## ADDED Requirements

### Requirement: Destroy the Ember Spire
The portal SHALL be won when the Order's Ember Spire is destroyed and an engineer has carried the Shining Star out of its vault to the commander.

#### Scenario: Spire falls and the star comes home
- **WHEN** the Spire is destroyed and an engineer reaches the commander with the star
- **THEN** the portal reports a won result with the Shining Star collected

### Requirement: Commander on foot
The commander SHALL place structures with a holo-placement beam, select and order units by looking at them and at the ground, and lift the view 30 m with the recon drone for 3 seconds (with a cooldown).

#### Scenario: Recon drone view
- **WHEN** the player triggers the recon drone
- **THEN** the view rises 30 m for 3 seconds and the drone is unavailable during its cooldown

### Requirement: Snowflakes
The portal SHALL award 1 snowflake for winning, +1 if no harvester was lost and +1 for winning within 15 minutes.

#### Scenario: Harvester never lost
- **WHEN** the player wins without losing a harvester, after 15 minutes
- **THEN** the result carries 2 snowflakes

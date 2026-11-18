# Spec Delta

## Purpose
A line-based text level format so FPS-style portals can be built and reviewed as plain data files under assets/maps/.

## ADDED Requirements

### Requirement: Text level files
FPS portals SHALL load their levels from text files with one entity per line (solids, decoration, breakables, doors, enemies, pickups, props, secrets, triggers, lights, signs, friend marks and bot waypoints), and the loader SHALL reject malformed lines with the line number.

#### Scenario: Malformed line
- **WHEN** a level file contains "spawn 1 2" on line 1
- **THEN** loading fails with an error starting "line 1:"

### Requirement: Level files are original assets
Level files SHALL be listed in the asset credits and licensed CC BY 4.0.

#### Scenario: Credits check
- **WHEN** a new level file is added without a credits entry
- **THEN** the credits check fails

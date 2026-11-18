# Spec Delta

## Purpose
Terrain and flight shared by the vehicle portals (carpet, mech, skis, ships).

## ADDED Requirements

### Requirement: Bounded deformable terrain
The terrain SHALL keep every height sample within its configured minimum and maximum, whatever deformation is applied, and SHALL bump the change counter of every render chunk whose samples changed.

#### Scenario: Deformation stays in bounds
- **WHEN** a spot is raised or lowered far beyond the limits
- **THEN** its height stops at the maximum or minimum and only the chunks around it report a change

### Requirement: Hovering flight
A flyer SHALL stay at least its clearance above the ground or water, below its ceiling and inside the world, SHALL glide up gentle slopes and SHALL be stopped by faces steeper than about 68 degrees.

#### Scenario: Flyer keeps clearance
- **WHEN** the flyer sinks over water, climbs without limit or thrusts into the world edge
- **THEN** it hovers at the clearance above the water, stops at the ceiling and stays inside the bounds

#### Scenario: Cliffs stop the flyer
- **WHEN** the flyer thrusts into a steep wall of raised land
- **THEN** it bumps and stays on its side of the wall, while a gentle slope is climbed

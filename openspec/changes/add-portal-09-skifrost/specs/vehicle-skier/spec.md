# Spec Delta

## Purpose
Armoured skiers on heightmap terrain for the Vehicle kit: skiing, jetpack energy, armour classes.

## ADDED Requirements

### Requirement: Skiing
Holding ski on the ground SHALL remove friction: on the flat the speed SHALL be kept, down a slope it SHALL grow like a frictionless slide, and touching down SHALL keep the momentum along the surface. Without ski, friction SHALL bring the skier to running speed or to a stop.

#### Scenario: Ski momentum conservation
- **WHEN** a skier skis across the flat, lets go, then skis down a long slope onto the flat
- **THEN** the flat keeps the speed, letting go stops them, the slope builds v^2 ~ 2gh past 150 km/h and the run-out keeps most of it

### Requirement: Jetpack energy
Jetting SHALL lift the skier and drain energy at the armour's rate; otherwise energy SHALL regenerate (faster with an energy pack) up to the armour's maximum. A pack that runs dry SHALL stay off for a moment.

#### Scenario: Energy drain and regen
- **WHEN** a skier jets for a second, then until empty, then rests
- **THEN** the energy drops by the drain rate, the pack locks out when empty, and refills at the regeneration rate, never above full

### Requirement: Armour classes
Light, medium and heavy armour SHALL trade running speed and energy for health, mass and weapon slots; changing armour SHALL keep the health fraction; pushes SHALL be divided by the mass.

#### Scenario: Armour classes
- **WHEN** a light skier runs, swaps to heavy, runs again and is pushed
- **THEN** the speeds match the classes, the health fraction is kept and the push is smaller

### Requirement: Solids
Skiers SHALL be blocked by solid boxes and stand on top of them.

#### Scenario: Solids block and carry
- **WHEN** a skier walks into a wall and drops onto a platform
- **THEN** the wall stops them and they stand on the platform

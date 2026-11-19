# Spec Delta

## Purpose
Block-grid traversal for first-person platformer portals.

## ADDED Requirements

### Requirement: Tile-sized jumps
A jump at walking pace SHALL clear a gap of one tile but not two, a running jump SHALL clear two tiles but not three, and a jump started within 25 degrees of a grid axis SHALL travel exactly along that axis.

#### Scenario: Jump distance per tile class
- **WHEN** the player jumps at the edge of gaps of one, two and three tiles, walking and running
- **THEN** the walking jump lands across one tile only and the running jump across two only

### Requirement: Ledges
Holding USE in the air SHALL catch a ledge whose top passes the hands within a tolerance window, which reaches walls up to two tiles high but not three; a hanging player SHALL shimmy along the ledge, pull up with JUMP or forward and drop when USE is released; JUMP with USE held in front of a one-tile step SHALL climb it.

#### Scenario: Ledge grab tolerance window
- **WHEN** the player jumps and grabs at walls one, two and three tiles high, and once without holding USE
- **THEN** the one- and two-tile walls are climbed, the three-tile wall is not, and nothing is caught without USE

### Requirement: Blocks
USE with forward SHALL push a block one tile when the tile beyond is empty and supported, USE with back SHALL pull it when the tile behind the player is free and supported.

#### Scenario: Block push collision
- **WHEN** the player pushes a block towards a wall and towards an edge, then pulls it
- **THEN** the block moves until the wall, never over the edge, and comes back one tile when pulled

### Requirement: Swimming
In water the player SHALL swim along the view, rise with JUMP and dive with CROUCH; breath SHALL run out about 30 seconds after the head goes under and come back above water.

#### Scenario: Air meter drowning
- **WHEN** the player stays on the bottom of a pool
- **THEN** drowning starts after about 30 seconds and breath returns at the surface

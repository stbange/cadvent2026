# Spec Delta

## Purpose
The first-person festive hall that connects the 24 portals, shows their state and the player's progress, and hosts the Christmas Eve epilogue.

## ADDED Requirements

### Requirement: Navigable first-person hall
The hub SHALL be a first-person 3D hall containing 24 portal arches (days 1–12 on the ground floor, 13–23 on a balcony, 24 in the north apse) and SHALL allow the player to reach any portal within 40 seconds of walking from the entrance.

#### Scenario: Reach the farthest portal
- **WHEN** an automated walker starts at the entrance and walks to portal 24 at normal speed
- **THEN** it arrives within 40 seconds

### Requirement: Portal visual states
Each portal SHALL display one of the states sealed, thawing, open or mastered according to calendar and progress.

#### Scenario: First visit of a newly unlocked portal
- **WHEN** an unlocked portal that was never entered becomes visible to the player for the first time
- **THEN** its ice-cracking reveal plays exactly once and the portal shows a live preview afterwards

#### Scenario: Mastered portal
- **WHEN** the player has earned 3 snowflakes in a portal
- **THEN** the portal shows a golden frame and its ornament hangs on the tree

### Requirement: Entering a portal
Walking into an unlocked portal SHALL start that portal's game after a transition of at most 2 seconds; sealed portals SHALL block entry.

#### Scenario: Blocked sealed portal
- **WHEN** the player walks into a sealed portal
- **THEN** the player is stopped and the countdown is displayed

#### Scenario: Return to hall
- **WHEN** a portal session ends (won, lost, or quit)
- **THEN** the player reappears in front of that portal in the hall

### Requirement: Progression display
The hub SHALL display collected snowflakes (0–3 per portal), light one wreath candle per 6 snowflakes (maximum 4), and place one ornament on the tree for each completed portal.

#### Scenario: Candle lighting
- **WHEN** the player's total snowflakes change from 11 to 12
- **THEN** the second wreath candle is lit

### Requirement: Christmas Eve epilogue
The hub SHALL play an epilogue sequence when the player places the 24th Shining Star (won in portal 24) on top of the tree, and SHALL allow replaying it afterwards.

#### Scenario: Epilogue trigger
- **WHEN** the player returns from a won portal 24 carrying the 24th Shining Star and uses the tree
- **THEN** the star is placed on the tree top and the epilogue plays

### Requirement: Wall of Legends
The hub SHALL contain a Wall of Legends with one plaque per configured friend, showing how many of that friend's marks the player has found and highlighting plaques whose marks are all found.

#### Scenario: Mark found
- **WHEN** the player registers a new mark of Messerjocke in any portal and returns to the hall
- **THEN** Messerjocke's plaque shows one more found mark

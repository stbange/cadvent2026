# Spec Delta

## Purpose
The common contract every portal game fulfils: lifecycle, objective, results, stars, saving, declared capabilities and automated play.

## ADDED Requirements

### Requirement: Clear goal and briefing
Every portal SHALL present its goal in an in-world briefing before play and SHALL keep the current objective accessible during play.

#### Scenario: Objective reminder
- **WHEN** the player opens the pause menu in any portal
- **THEN** the current objective text is shown

### Requirement: Session results and stars
Every portal SHALL end in a won or lost result and SHALL award 0–3 stars: 1 star for completion and up to 2 more for portal-specific conditions defined in that portal's spec.

#### Scenario: Best result kept
- **WHEN** the player wins with 1 star after a previous 2-star result
- **THEN** the stored result for that portal remains 2 stars

### Requirement: Play length
Every portal SHALL be completable by a first-time player in 15–25 minutes at the default difficulty, as estimated by its pace-mode completion bot and validated by playtests.

#### Scenario: Bot pace estimate in range
- **WHEN** the completion bot plays a portal in pace mode at default difficulty
- **THEN** the simulated play time is between 12 and 28 minutes

### Requirement: Checkpoints and saves
Every portal SHALL save checkpoints and best results persistently, and SHALL resume from the last checkpoint after the application restarts.

#### Scenario: Resume after restart
- **WHEN** the player quits during a portal after reaching a checkpoint and later re-enters the portal
- **THEN** the player may continue from that checkpoint

### Requirement: Isolation between portals
Leaving a portal SHALL release all of its runtime state so that no state from one portal affects another.

#### Scenario: No leaked state
- **WHEN** portals 1, 2 and 1 again are entered and left in sequence
- **THEN** the memory in use after leaving equals the memory in use before the first entry

### Requirement: Deterministic simulation
Every portal's simulation SHALL produce identical state for identical seed and input sequence on all supported platforms.

#### Scenario: Cross-platform replay
- **WHEN** a recorded replay is run on Linux, Windows and WebAssembly
- **THEN** the final state hashes are identical

### Requirement: Declared capabilities
Every portal SHALL declare its supported local and online player counts, game modes (solo, co-op, versus) and XR support level (full, seated, tabletop, none); the hub SHALL only offer declared options.

#### Scenario: Solo-only portal
- **WHEN** a party of two enters a portal that declares solo only
- **THEN** the hub explains that the portal is solo and offers to enter alone

### Requirement: Difficulty and story mode
Every portal SHALL offer at least a story difficulty (reduced challenge, generous checkpoints) and a default difficulty.

#### Scenario: Story mode selection
- **WHEN** the player selects story difficulty
- **THEN** damage received is reduced and the portal can still be completed for 1 star

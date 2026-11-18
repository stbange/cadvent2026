# Spec Delta

## Purpose
Quality obligations for every change: specification traceability, unit-test coverage, automated game tests, determinism and continuous integration.

## ADDED Requirements

### Requirement: Scenario traceability
Every scenario in the specifications SHALL be covered by at least one automated test whose name references the scenario, and CI SHALL fail when a scenario has no test.

#### Scenario: Missing test detected
- **WHEN** a new scenario is added to a spec without a matching test
- **THEN** the CI traceability check fails and names the scenario

### Requirement: Unit-test coverage gate
Unit tests SHALL achieve at least 95 % line coverage and 85 % branch coverage over first-party source and tool code, excluding only an explicitly listed platform shim and third-party code.

#### Scenario: Coverage below threshold
- **WHEN** a commit lowers line coverage to 94.8 %
- **THEN** the CI coverage job fails

### Requirement: Completion bots
Every released portal SHALL have an automated player that completes it at each offered difficulty, and CI SHALL run it headless on every commit.

#### Scenario: Bot regression
- **WHEN** a change makes a portal impossible to complete
- **THEN** that portal's bot test fails within its tick budget

### Requirement: Scripted scenario tests
The game SHALL support headless, accelerated execution of scripted scenarios that inject inputs and assert on game state and events.

#### Scenario: Headless scenario run
- **WHEN** a scenario script is run with the headless flag at maximum speed
- **THEN** the process exits with status 0 if all assertions pass and non-zero otherwise

### Requirement: Golden-frame tests
Rendering SHALL be verified by comparing captured low-resolution frames against reference images with a tolerance of at most 0.5 % differing pixels.

#### Scenario: Visual regression
- **WHEN** a shader change alters the lighting of the hub golden frame beyond tolerance
- **THEN** the golden-frame test fails and stores a diff image

### Requirement: Web end-to-end tests
The web build SHALL be tested in a headless browser for loading, desktop controls, mobile touch mode, entering a portal and the WebXR entry flow using an emulator.

#### Scenario: Mobile emulation
- **WHEN** the end-to-end suite runs with a phone device profile
- **THEN** touch controls are visible and the player can walk into portal 1

### Requirement: Sanitized builds
Unit and game tests SHALL also pass under address and undefined-behaviour sanitizers.

#### Scenario: Out-of-bounds write
- **WHEN** code writes past an array end during tests
- **THEN** the sanitizer job fails

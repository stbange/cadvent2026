# Spec Delta

## Purpose
Decides which of the 24 portals are playable on a given local date during and after the 2026 advent season.

## ADDED Requirements

### Requirement: Daily portal unlock
The system SHALL make portal n (1 ≤ n ≤ 24) date-eligible when the device's local date is on or after December n of the season year (2026). Portals 1–23 SHALL be unlocked when date-eligible; portal 24 SHALL additionally require the Star Arch to be complete (see shining-stars).

#### Scenario: Portal sealed before its day
- **WHEN** the local date is 2026-12-04 23:59
- **THEN** portal 5 is sealed and portals 1–4 are unlocked

#### Scenario: Portal opens at local midnight
- **WHEN** the local date changes from 2026-12-04 to 2026-12-05 while the game is running
- **THEN** portal 5 becomes unlocked without restarting the game

#### Scenario: Time zone independence
- **WHEN** two players in different time zones each reach local 2026-12-10 00:00
- **THEN** portal 10 unlocks for each at their own local midnight

### Requirement: Season states
The system SHALL distinguish a teaser state before the season, the advent season, and a post-season state in which all portals remain unlocked permanently.

#### Scenario: Teaser before December
- **WHEN** the local date is 2026-11-30
- **THEN** all portals are sealed and the hub shows a countdown to portal 1

#### Scenario: After Christmas Eve
- **WHEN** the local date is 2027-01-15 and the Star Arch is complete
- **THEN** all 24 portals are unlocked

#### Scenario: Portals stay open after the season
- **WHEN** the local date is 2027-01-15 and the Star Arch is incomplete
- **THEN** portals 1–23 are unlocked and portal 24 stays closed

### Requirement: Development unlock override
The system SHALL provide a development override (build option, command-line flag, or web URL parameter) that unlocks all portals, and SHALL show a visible marker while the override is active.

#### Scenario: Override via URL parameter
- **WHEN** the web build is opened with the parameter `dev=unlockall` on 2026-11-20
- **THEN** all portals are unlocked, all Star Arch sockets are filled, and a "DEV" marker is visible in the hub

### Requirement: Countdown information
The system SHALL report, for every sealed portal, the remaining time until it unlocks.

#### Scenario: Countdown on a sealed portal
- **WHEN** the player inspects portal 8 at local time 2026-12-05 20:00
- **THEN** the portal plate shows "opens in 2 d 4 h"

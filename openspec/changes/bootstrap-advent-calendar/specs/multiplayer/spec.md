# Spec Delta

## Purpose
Optional local split-screen and online play in the hub and in portals that declare multiplayer support.

## ADDED Requirements

### Requirement: Local split screen
Desktop builds SHALL support up to 4 local players and the web build up to 2, each bound to its own input device, in the hub and in portals declaring local multiplayer.

#### Scenario: Second player joins
- **WHEN** a second gamepad presses start in the hub
- **THEN** the screen splits and a second avatar appears

### Requirement: Online sessions by room code
The system SHALL allow a host to create an online session identified by a short room code or link, and other players to join it, without accounts or personal data beyond a chosen nickname.

#### Scenario: Join by link
- **WHEN** a player opens the host's invite link
- **THEN** the player joins the host's hall session and sees the host's avatar

### Requirement: Shared hub and party portals
Players in a shared session SHALL see each other in the hall and SHALL be able to enter a multiplayer-capable portal together after the host confirms.

#### Scenario: Party enters portal
- **WHEN** three players stand in portal 7's arch and the host confirms
- **THEN** all three start the same match of portal 7

### Requirement: Consistent shared simulation
Online sessions SHALL keep all participants' game state consistent; for lockstep portals a state divergence SHALL be detected within 1 second and reported.

#### Scenario: Desync detection
- **WHEN** a test injects a state difference into one lockstep peer
- **THEN** a desync is reported within 60 simulation ticks

### Requirement: Playability under latency
Action portals SHALL remain playable with 150 ms round-trip latency and 5 % packet loss.

#### Scenario: Lossy network test
- **WHEN** two bots play a co-op portal over a simulated 150 ms / 5 % loss link
- **THEN** the portal is completed and no player state diverges permanently

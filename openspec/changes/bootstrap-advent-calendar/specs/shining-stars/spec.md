# Spec Delta

## Purpose
The Shining Star meta-quest: one collectible star in each of portals 1–23 that must be set into the Star Arch to open portal 24, plus the 24th star that crowns the tree.

## ADDED Requirements

### Requirement: One Shining Star per portal
Each of portals 1–23 SHALL contain exactly one Shining Star that becomes reachable at the portal's climax at every difficulty, and that the player must pick up physically.

#### Scenario: Star reachable in story mode
- **WHEN** portal 9's completion bot wins on story difficulty
- **THEN** portal 9's Shining Star is reachable and the bot picks it up

#### Scenario: Win without pickup
- **WHEN** the player wins portal 4 but leaves without picking up its Shining Star
- **THEN** the star is not credited and portal 4's plate in the hall shows that the star is still missing

### Requirement: Star persistence
A picked-up Shining Star SHALL be saved when the player leaves the portal carrying it and SHALL never be lost afterwards.

#### Scenario: Star kept after defeat
- **WHEN** the player has collected portal 2's star and later loses a replay of portal 2
- **THEN** portal 2's star remains collected

### Requirement: Star Arch sockets
The hall SHALL show 23 numbered sockets around portal 24; using the arch SHALL move every collected but unset star into the socket of its day.

#### Scenario: Setting stars
- **WHEN** the player carries the stars of days 3 and 5 and uses the Star Arch
- **THEN** sockets 3 and 5 are filled and glow

### Requirement: Portal 24 gate
Portal 24 SHALL open only when its date has come and all 23 sockets are filled; otherwise the arch SHALL list the missing days and the hall SHALL point to those portals.

#### Scenario: All stars on Christmas Eve
- **WHEN** the local date is 2026-12-24 and all 23 sockets are filled
- **THEN** portal 24 opens

#### Scenario: Missing stars on Christmas Eve
- **WHEN** the local date is 2026-12-24 and sockets 7 and 18 are empty
- **THEN** portal 24 stays closed, the arch lists days 7 and 18, and light beams point to portals 7 and 18

#### Scenario: All stars before Christmas Eve
- **WHEN** all 23 sockets are filled on 2026-12-23
- **THEN** portal 24 stays sealed with its countdown

### Requirement: The 24th star
Winning portal 24 SHALL award the 24th Shining Star, which the player places on top of the hall's tree to trigger the epilogue.

#### Scenario: Crowning the tree
- **WHEN** the player returns from a won portal 24 and uses the tree
- **THEN** the 24th star is placed on the tree top

### Requirement: Stars in multiplayer
A Shining Star picked up in a co-op session SHALL be credited to every participant; in versus portals every participant who finishes the match SHALL receive it.

#### Scenario: Co-op pickup
- **WHEN** player 2 picks up portal 11's star in a two-player co-op session
- **THEN** both players' saves record portal 11's star

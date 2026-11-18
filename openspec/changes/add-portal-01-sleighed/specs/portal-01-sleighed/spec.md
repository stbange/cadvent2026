# Spec Delta

## Purpose
Day 1, Sleighed!: an original first-person shooter in which Brick Holloway takes the Glitterdome Mall back from light-stealing space hogs.

## ADDED Requirements

### Requirement: Fuse candles open the staff door
The staff door to the service tunnel SHALL open only when the player carries all three fuse candles from the toy store, the food court and the cinema, and SHALL report how many are missing otherwise.

#### Scenario: Door without all candles
- **WHEN** the player uses the staff door carrying no fuse candle
- **THEN** the door stays shut and the message reads "STAFF ONLY - NEEDS 3 FUSE CANDLES (0/3)"

### Requirement: Light Siphon and escape
Destroying the three Siphon emitters on the roof SHALL bring down the Siphon, release the Shining Star and start a dome-collapse countdown (75 s default, 150 s story, 45 s hard); reaching the sleigh pad with the star wins, the countdown running out loses.

#### Scenario: Win by escaping with the star
- **WHEN** all emitters are destroyed, the star is taken and the player reaches the sleigh pad
- **THEN** the portal reports a won result with the Shining Star collected

#### Scenario: Collapse
- **WHEN** the countdown reaches zero before the player escapes
- **THEN** the portal reports a lost result

### Requirement: Snowflakes
The portal SHALL award 1 snowflake for winning, +1 for winning within 18 minutes on default or hard difficulty, and +1 for finding all 6 secrets.

#### Scenario: Fast run without secrets
- **WHEN** the player wins on default difficulty within 18 minutes having found no secret
- **THEN** the result carries 2 snowflakes

### Requirement: Checkpoints
The portal SHALL record a checkpoint on entering the service tunnel and on reaching the roof, and resuming SHALL restore the fuse candles, opened door and weapons collected up to that point.

#### Scenario: Resume on the roof
- **WHEN** the portal starts with checkpoint 2
- **THEN** the player stands on the roof with the chaingun and the boss is awake

### Requirement: Freight lift keeps the player's state
The freight lift SHALL move the player to the roof without changing health, armour or carried fuse candles.

#### Scenario: Riding the lift
- **WHEN** the player enters the lift with 60 health and two fuse candles
- **THEN** the player arrives on the roof with 60 health and the same two candles

### Requirement: Completion bot
An automated player SHALL complete the portal on story, default and hard difficulty.

#### Scenario: Bot on every difficulty
- **WHEN** the bot plays from the start on each difficulty
- **THEN** each run ends won with the Shining Star collected

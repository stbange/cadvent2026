# Spec Delta

## Purpose
Day 8, Cascade: Ines Kowalczyk's day at Polaris Deep, told without a single cutscene.

## ADDED Requirements

### Requirement: No cutscenes
Every scripted moment (the monorail, the test, the cascade, arrivals, the breach) SHALL play while the player keeps full control of movement and view.

#### Scenario: No cutscene ever takes control
- **WHEN** the player looks and walks around during the monorail ride and walks away from the test
- **THEN** the view and position respond, the car arrives, and the cascade breaks the chamber open while the player walks

### Requirement: Escort to the scanner
The array door's retina scanner SHALL open only with a scientist beside it; scientists SHALL follow once freed from the safe lab and can fall to enemies. Followers SHALL obey use: follow me, wait here, addressed by looking at them.

#### Scenario: Escort to the scanner
- **WHEN** the player tries the scanner alone, frees the scientists and leads them to it
- **THEN** it refuses, then accepts; a scientist under attack can be lost

### Requirement: Reverse the beam
On the array three breakers SHALL reverse the beam; the Shining Star SHALL fall at the player's feet and taking it wins. Snowflakes: 1 for finishing, +1 if all three scientists live, +1 for under 16 minutes. Checkpoints: after the cascade, at Blackice, at the array.

#### Scenario: Reverse the beam
- **WHEN** the player survives a slam and flips all three breakers
- **THEN** the shard falls, is taken, and the portal is won with 3 snowflakes

#### Scenario: Resume at the array
- **WHEN** the portal resumes at the array checkpoint with rendering on
- **THEN** the breakers are all off and the bot reverses the beam

#### Scenario: The story from the hall
- **WHEN** the completion bot enters portal 8 on 8 December
- **THEN** it plays the story through and takes the star

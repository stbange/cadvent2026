# Spec Delta

## Purpose
Day 6, Iron Yule: Pilot Sable walks a militia machine through a three-mission campaign with the convoy AI Santa Claude.

## ADDED Requirements

### Requirement: Mech lab
Before the first mission the pilot SHALL fit a weapon to each of four hardpoints within ten tons (swaps that would exceed it are skipped), pick a paint scheme, and launch.

#### Scenario: Mech lab within tonnage
- **WHEN** the pilot swaps weapons up to and past the limit, picks the third paint and launches
- **THEN** the loadout never exceeds ten tons, the friend's paint mark is found and mission 1 begins with full ammunition

### Requirement: Escort and the star
Sleighbell SHALL drive the canyon route, holding while enemies are near or its escort lags; three ambushes SHALL trigger along the way and each beaten one becomes a checkpoint for the convoy. If Sleighbell is destroyed the mission SHALL restart from the last convoy checkpoint. At the depot Santa Claude SHALL hand over the Shining Star from the cargo bay; taking it completes mission 1 and records checkpoint 1.

#### Scenario: Escort the crawler
- **WHEN** the crawler meets the first ambush, it is beaten, and then the crawler is destroyed
- **THEN** the crawler held during the fight, and the convoy restarts at the checkpoint with the first ambush still beaten

#### Scenario: Santa Claude hands over the star
- **WHEN** Sleighbell reaches the depot and the pilot walks to its cargo bay
- **THEN** the star is collected, checkpoint 1 is recorded and mission 2 starts

### Requirement: Dam pumps and the duel
Mission 2 SHALL end when all four pumps are destroyed (the first loss calls hover reinforcements) and record checkpoint 2; mission 3 SHALL end when Krampus is destroyed, completing the campaign. Coolant SHALL be dropped once per mission on request while Sleighbell is near; water and steam vents cool the walker; losing the mech ejects the pilot and loses the portal. Snowflakes: 1 for the campaign, +1 if never shut down from heat, +1 if no component was lost in the duel.

#### Scenario: Pump station falls
- **WHEN** a pump is shot down and then the rest fall
- **THEN** reinforcements arrive, then mission 3 starts with checkpoint 2

#### Scenario: Duel with Krampus
- **WHEN** the pilot cools at a vent, loses an arm and destroys Krampus
- **THEN** the vent sheds heat fast and the campaign is won with 2 snowflakes

#### Scenario: Coolant drop on request
- **WHEN** the pilot calls for coolant twice and walks to the canister
- **THEN** one canister falls, the second call is refused, and the heat drops to zero

#### Scenario: The campaign from the hall
- **WHEN** the completion bot enters portal 6 on 6 December
- **THEN** it collects the star and wins the campaign

#### Scenario: Resume at the duel
- **WHEN** the portal resumes at checkpoint 2 with rendering on
- **THEN** the duel in the crater starts and the bot wins it

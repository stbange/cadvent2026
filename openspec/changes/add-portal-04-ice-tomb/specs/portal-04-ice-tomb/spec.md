# Spec Delta

## Purpose
Day 4, Ice Tomb of the Winter King: explorer-cartographer Mara Voss recovers three Sun Gears from a glacier tomb.

## ADDED Requirements

### Requirement: Hall of Pillars
The hall's lever SHALL open the gear alcove's door for 14 seconds (25 story, 11 hard), after which the door shuts and the lever resets unless the player stands in the doorway; taking Gear 1 SHALL jam the door open and record checkpoint 1.

#### Scenario: Lever opens the timed door
- **WHEN** the lever is pulled and 14 seconds pass, then the gear is taken
- **THEN** the door closes and the lever resets, and after the gear it stays open

### Requirement: Flooded crypt
The underwater lever in the pool SHALL remove the grate of the tunnel that leads to Gear 2; drowning SHALL hurt and finally send Mara back to her last save crystal.

#### Scenario: Pool lever and the air meter
- **WHEN** the player pulls the underwater lever and then stays under water
- **THEN** the grate is gone and, once breath and health run out, Mara respawns

### Requirement: Save crystals
Using a save crystal SHALL store the respawn point, heal and count one save; dying or falling into the abyss SHALL return Mara to the last crystal used.

#### Scenario: Save crystals and respawn
- **WHEN** the player saves twice at a crystal and then falls into the abyss
- **THEN** two saves are counted and Mara stands at the crystal again

### Requirement: The Colossus
Taking Gear 3 SHALL wake the Colossus and collapse the gallery floor behind the player; being caught or falling SHALL reset the chase (gear back, floor restored), and reaching the solar chamber SHALL end it.

#### Scenario: The Colossus chase
- **WHEN** the player takes Gear 3 and waits, then takes it again and runs to the solar chamber
- **THEN** Mara is caught and the chase resets, and the second time the chase ends safely

### Requirement: Solar door and snowflakes
With all three gears the solar door SHALL form the Shining Star; taking it wins. The portal SHALL award 1 snowflake for winning, +1 for using at most three save crystals and +1 for all three jade ornaments.

#### Scenario: Three gears open the solar door
- **WHEN** the player uses the door with two gears, then with three, and takes the star
- **THEN** the door asks for the third gear, then the star forms and the portal is won with 2 snowflakes (no jade)

### Requirement: Checkpoints
Resuming at checkpoint 1 SHALL restore Gear 1, the open door and the solved block puzzle at the first crystal; checkpoint 2 SHALL also restore Gear 2 and the open grate at the second crystal.

#### Scenario: Resume at a checkpoint
- **WHEN** the portal starts with checkpoint 1 or 2
- **THEN** the gears, doors and the starting crystal match the checkpoint

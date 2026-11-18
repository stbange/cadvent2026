# Spec Delta

## Purpose
Personal in-jokes for the friends the calendar is made for: their gamer tags appear as discoverable marks throughout all portals.

## ADDED Requirements

### Requirement: Configurable friend list
The game SHALL read the friends' display names and short tags from a single content file; the default list SHALL be Dr. Doom, Bloodmaker, C6P, X-Ray Junkie, Messerjocke and Foen-X.

#### Scenario: Renaming a friend
- **WHEN** the content file renames "C6P" to "R2X"
- **THEN** every mark that showed C6P shows R2X instead

### Requirement: Marks in every portal
Every portal SHALL contain at least two friends' marks (graffiti, signs, gravestones, unit, bot or soldier names, log authors, callsigns or credits), and every friend SHALL appear in at least 12 portals.

#### Scenario: Coverage check
- **WHEN** the content checker scans the mark definitions of all 24 portals
- **THEN** each portal has at least 2 marks and each friend appears in at least 12 portals

### Requirement: Text-only representation
Friends SHALL be represented only by their names in text, signage and subtitled voice lines, and not as drawn characters resembling any third-party character.

#### Scenario: Originality review covers marks
- **WHEN** a portal's originality review is recorded
- **THEN** it confirms that friends' marks are text or voice only

### Requirement: Discoverable marks
A mark SHALL be registered as found when the player looks at it from close range or reads it, and found marks SHALL be saved and shown on the Wall of Legends.

#### Scenario: Registering graffiti
- **WHEN** the player looks at the "Bloodmaker was here" graffiti in portal 1 from within 3 metres
- **THEN** the mark is registered as found for Bloodmaker

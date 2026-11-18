# Spec Delta

## Purpose
Licence and originality obligations for all code and content shipped by the project.

## ADDED Requirements

### Requirement: Code licence
All first-party source code SHALL be licensed under the MIT License, and third-party code SHALL retain its original licence with its notice included in distributions.

#### Scenario: Notices in release
- **WHEN** a release archive is produced
- **THEN** it contains the MIT licence and the notices of all bundled third-party libraries

### Requirement: Asset licence and credits
All first-party art, audio, music, maps and text content SHALL be licensed under CC BY 4.0 and listed with author and source in a credits file shown in-game.

#### Scenario: Uncredited asset
- **WHEN** an asset file exists without a credits entry
- **THEN** the CI credits check fails

### Requirement: Originality
Portals SHALL NOT include names, logos, characters, story, level layouts, art, sounds or music from the games that inspired them; they MAY reproduce genre conventions and game mechanics.

#### Scenario: Originality review before release
- **WHEN** a portal is prepared for release
- **THEN** a recorded originality review confirms no third-party protected content is included

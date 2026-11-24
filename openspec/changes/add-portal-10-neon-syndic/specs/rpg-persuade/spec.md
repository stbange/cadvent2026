# Spec Delta

## Purpose
Persuasion and hacking for the RPG / tactics kit.

## ADDED Requirements

### Requirement: Persuasion
A person in range SHALL convert after enough exposure (quicker when closer); the number of followers allowed SHALL grow with the followers already won; resistant minds SHALL need a crowd first and take longer; progress SHALL fade out of range.

#### Scenario: Persuasion range and limit
- **WHEN** people are exposed near and far, guards with small and big crowds, and a full following tries for more
- **THEN** near converts first, out of range never, guards only with a crowd and slower, and the limit holds

### Requirement: Hacking
A timing ring SHALL count a press inside the gap as a hit (and move the gap) and outside as a miss, and be done after the needed hits.

#### Scenario: Hacking timing ring
- **WHEN** one press misses and then presses are timed to the gap
- **THEN** the miss counts, three hits finish the hack, and a finished hack ignores presses

# Spec Delta

## Purpose
Day 5, Hearthguard: the Captain of Kindlewick holds the gnome village against the troll raiders of the Rimeback Clans and brings the midwinter star home.

## ADDED Requirements

### Requirement: The command stave
The Captain SHALL buy structures, units and the keep upgrade from a stave, place structures near the village (shipyards on the lake shore, nothing on the ice), and refuse purchases with the reason (resources, food, missing barracks, workshop, shipyard or keep, full queue).

#### Scenario: Three resource costs on the stave
- **WHEN** the stave shows a farm, a ferry and the keep upgrade
- **THEN** they cost 80 gold and 40 lumber, 100 gold, 80 lumber and 60 tallow, and 300 gold and 200 lumber

### Requirement: Raids and the thaw
Three troll raids SHALL cross the frozen lake at fixed times per difficulty; the Longhall SHALL be warded until the last raid is beaten, when spring thaws the lake, drowns anyone on the ice and records checkpoint 2. Beating the first raid SHALL record checkpoint 1.

#### Scenario: Three raids then the thaw
- **WHEN** three raids arrive and are beaten while a soldier stands on the ice
- **THEN** the Longhall shrugs off damage until then, checkpoints 1 and 2 are recorded, the lake opens and the soldier drowns

### Requirement: The star comes home
Burning the Longhall SHALL raise the Shining Star in its fire pit and scatter the trolls; the Captain SHALL carry it (dropping it if he falls) and win by bringing it within 7 m of the hall. Losing the hall loses the portal. Snowflakes: 1 for winning, +1 if no building fell before the thaw, +1 if the Captain never fell and at most 30 units were trained.

#### Scenario: The star comes home
- **WHEN** the Longhall burns, the Captain takes the star, falls, returns, takes it again and reaches the hall
- **THEN** the portal is won with 2 snowflakes

#### Scenario: Hold the raids then burn the Longhall
- **WHEN** the completion bot enters portal 5 from the hall on 5 December
- **THEN** it holds the raids, ferries the army north, burns the Longhall and wins with the star

### Requirement: Checkpoints
Resuming at checkpoint 1 SHALL restore the village after the first raid; checkpoint 2 SHALL restore a spring village with the lake open and the raids over.

#### Scenario: Resume from checkpoints
- **WHEN** the portal starts with checkpoint 1 or 2
- **THEN** raids, buildings, workers, army and resources match the checkpoint

#### Scenario: Resume in spring
- **WHEN** the portal resumes at checkpoint 2 with rendering on
- **THEN** the bot builds a shipyard and a ferry, crosses, burns the Longhall and wins

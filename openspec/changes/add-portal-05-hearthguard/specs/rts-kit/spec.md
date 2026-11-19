# Spec Delta

## Purpose
Grid, economy, orders and harvesting shared by the real-time strategy portals.

## ADDED Requirements

### Requirement: Blocking resources
A resource cell SHALL hold one of three kinds (credits/gold, lumber, tallow) and MAY block movement; harvesters SHALL work blocking cells from a neighbouring cell, and a blocking cell SHALL become passable once it is depleted, bumping the grid version so paths and meshes can be rebuilt.

#### Scenario: Tree depletion and pathing update
- **WHEN** a row of trees walls off a goal and a worker fells the one in the gap
- **THEN** no path exists before, the grid version changes when the tree falls, and the path goes through the gap afterwards

### Requirement: Supply
When a side uses supply, each unit SHALL cost its supply and structures SHALL provide it; training SHALL be refused while queued and living units would exceed the cap.

#### Scenario: Supply cap
- **WHEN** a side with a cap of 5 holds 4 workers and queues two more
- **THEN** the first is accepted and the second is refused with a supply event, until a farm raises the cap

### Requirement: Three-resource costs
Queuing SHALL check and pay credits, lumber and tallow up front, refuse when any of them falls short and refund all three on cancel; harvested cargo SHALL be credited to the resource it came from.

#### Scenario: Three resource costs
- **WHEN** a side queues a unit costing gold, lumber and tallow, cancels it, and lacks tallow for the next one
- **THEN** all three are paid and refunded, and the next one is refused

### Requirement: Water and transports
Water cells SHALL be passable only to naval units; a naval unit sent to land SHALL sail to the nearest water and unload there. Units ordered to board SHALL climb aboard within reach while there is room, ride with the transport, cannot be targeted while aboard and die with it.

#### Scenario: Ferry loading
- **WHEN** three soldiers board a ferry, it sails to the far shore and one more tries to board a full ferry
- **THEN** they ride along, unload on the far shore, and the extra soldier waits

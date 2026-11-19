# Spec Delta

## Purpose
Grid, economy, orders and harvesting shared by the real-time strategy portals.

## ADDED Requirements

### Requirement: Building placement
A structure SHALL be placeable only inside the map, on open cells free of ore, and (except a side's first structure) within three cells of a structure the same side owns.

#### Scenario: Placement validation
- **WHEN** a structure is placed off the map, over terrain, a building or ore, or far from the owner's base
- **THEN** placement is refused with the matching reason, and accepted next to the base

### Requirement: Paths
Ground units SHALL follow 8-way shortest paths over passable cells without cutting corners, end beside a building they are sent to, re-plan when something is built in their way and retry unreachable goals later.

#### Scenario: Paths avoid buildings
- **WHEN** a wall with one gap separates start and goal
- **THEN** the path passes through the gap, and no path exists once the gap is closed

### Requirement: Power and production
A side's power factor SHALL be 1 when it makes at least as much power as it uses and otherwise the ratio made/used (at least 0.25); production SHALL advance at that rate and defences SHALL fire only at full power. Queued items SHALL be paid up front, cancelling refunds the last item, and a finished structure waits for placement before the next one completes.

#### Scenario: Power math
- **WHEN** a base uses 130 power while making 100
- **THEN** production runs at 100/130 speed, defences go offline, and a low-power alert is raised when the balance tips

#### Scenario: Build queue pays up front
- **WHEN** a side queues five power plants with 5000 credits and cancels one
- **THEN** a sixth is refused, the cancelled one is refunded and the next structure waits until the finished one is placed

### Requirement: Harvesting
A harvester SHALL seek the nearest ore, mine a tenth of its capacity every half second until full or the field is dry, return to a refinery of its side and unload into credits, then seek again; without a refinery a loaded harvester waits.

#### Scenario: Harvester cycle
- **WHEN** a harvester works a field of 300 ore next to a refinery
- **THEN** it delivers loads of 100 until all 300 ore are credits, and waits loaded when the refinery is gone

### Requirement: Orders and control groups
Units SHALL obey move, attack, attack-move and stop orders, defend themselves when idle, and leave every control group when they die; ordering a group orders each living member.

#### Scenario: Control groups
- **WHEN** two riflemen in group 1 are ordered to a cell and one of them dies
- **THEN** both walk there, and the next group order reaches only the survivor

### Requirement: Lockstep fingerprint
The kit SHALL fingerprint its state (sides, queues, buildings, units) so that two simulations fed the same commands produce the same hash and any difference changes it.

#### Scenario: Lockstep hash equality
- **WHEN** two worlds run the same script
- **THEN** their hashes match, and a single credit or a millimetre of unit movement makes them differ

### Requirement: Fog of war
Each side SHALL see only the cells within sight of its units and buildings (plus viewers a portal adds), and SHALL remember every cell it has ever seen as explored.

#### Scenario: Fog of war
- **WHEN** a unit walks away from a spot it stood on
- **THEN** the spot is no longer visible but stays explored, and the other side never saw it

### Requirement: Neutral buildings and capture
Buildings of the neutral side SHALL be ignored by automatic targeting, and capturing a building SHALL hand it, its cells and its power to the capturing side.

#### Scenario: Neutral buildings and capture
- **WHEN** a tank idles next to a neutral power plant that is then captured
- **THEN** the plant is not attacked, and after the capture it belongs to and powers the capturing side

### Requirement: Separate production queues
Structures and units SHALL build in two queues that advance at the same time, each unit type training only where a building with its training flag stands (a war factory unless stated otherwise), and a refused purchase for lack of money SHALL raise an event.

#### Scenario: Barracks and factory queues run together
- **WHEN** a side with a barracks queues a trooper and a factory at once
- **THEN** both finish together, a tank is refused without a factory and a purchase without money raises the broke event

### Requirement: AI opponent
The AI opponent SHALL keep its planned base standing (rebuilding lost structures), keep a harvester, train its unit mix up to an army cap, attack intruders near its home with idle units and send attack waves that grow up to a maximum.

#### Scenario: AI opponent
- **WHEN** the AI plays with a plan, a unit mix and a wave timer, loses its factory and sees an intruder at home
- **THEN** it builds the plan, sends waves, rebuilds the factory and destroys the intruder

# Spec Delta

## Purpose
Day 2, Aurora Carpet: an apprentice skyweaver reshapes the Shattered Fjords and wins the starlight mana back from the Frost Sorcerer Vael.

## ADDED Requirements

### Requirement: Mana economy
The world SHALL hold exactly 1000 mana at all times, split between free orbs, balloons, castles, creatures and the wild pool; spent mana SHALL return to the wild pool and be reborn as creatures, and killed creatures and damaged castles SHALL release their mana as free orbs. Mana counts towards an owner once it is banked in its castle or carried by its balloons.

#### Scenario: Mana is conserved
- **WHEN** spells are cast, creatures die and a castle is destroyed
- **THEN** the world still holds 1000 mana

#### Scenario: Claiming transfers ownership
- **WHEN** a claim spell reaches a free orb owned by someone else
- **THEN** the orb belongs to the caster, but counts for them only once a balloon carries it

### Requirement: Balloons and castles
A castle SHALL send up to two plus its level balloons to fetch its owner's free orbs, flying over any land in the way, and SHALL grow to level 2 at 150, level 3 at 350 and level 4 at 600 stored mana; from level 2 its turrets shoot enemies, enemy balloons and creatures in range. Only one castle per side is allowed, on flat dry land at least 50 m from the other side's castle.

#### Scenario: Balloons fly over raised land
- **WHEN** a wall of raised land stands between the castle and a claimed orb
- **THEN** a balloon still fetches the orb and the castle banks its mana

#### Scenario: Castle grows with mana
- **WHEN** the player casts Castle on the home plateau and later stores 400 mana
- **THEN** a level 1 castle appears, a second castle is refused, and the castle reaches level 3

### Requirement: Vael, the rival wizard
Vael SHALL harvest and steal mana, duel the player at close range, retreat to heal when weak, raid the player's castle during the siege, and respawn at his castle while it stands. His castle SHALL be immune to fireballs while he lives and before nightfall; the Volcano scroll (found at night) works at any time.

#### Scenario: Ward protects Vael's castle until nightfall
- **WHEN** the player fireballs Vael's castle before nightfall, then at night after Vael fell
- **THEN** the castle loses no mana the first time and 8 mana the second time

### Requirement: Winning, losing and the Shining Star
The Shining Star SHALL appear at the aurora's apex when the player holds at least 75 % of the world's mana, or rise from Vael's ruins when his castle is destroyed; catching it wins. Vael holding 75 % SHALL lose the portal.

#### Scenario: Star rises at 75 percent
- **WHEN** the player's share reaches 75 %
- **THEN** the star appears, and flying into it collects the star and wins

#### Scenario: Win by mana share or ruins
- **WHEN** the completion bot plays Day 2 from the hall
- **THEN** it wins with the Shining Star

#### Scenario: Vael hoards the light
- **WHEN** Vael's share reaches 75 %
- **THEN** the portal reports a lost result

### Requirement: Snowflakes
The portal SHALL award 1 snowflake for winning, +1 if the castle reached level 2 and never fell below it, and +1 for winning within 15 minutes on default or hard difficulty.

#### Scenario: Snowflakes for a kept castle
- **WHEN** the player wins quickly on default difficulty with a castle kept at level 2 or more
- **THEN** the result carries 3 snowflakes, and only 1 on story difficulty after the castle dropped

### Requirement: Checkpoints
The portal SHALL record checkpoint 1 when the first castle stands and checkpoint 2 at nightfall; resuming SHALL rebuild the castle on the home plateau, and at nightfall bank 300 mana and show the Volcano scroll.

#### Scenario: Resume checkpoints
- **WHEN** the portal starts with checkpoint 1 or 2
- **THEN** the castle stands, the world still holds 1000 mana, and at checkpoint 2 night has fallen with the scroll waiting

#### Scenario: Resume at nightfall
- **WHEN** the rendered game resumes Day 2 at nightfall and the bot plays on
- **THEN** it wins with the Shining Star

### Requirement: Friends' marks
Day 2 SHALL hide three marks: Vael's siege taunt naming friend 1, giant runes of friend 3's name raised on the middle isle (registered from above 50 m), and a dagger-shaped rock bearing friend 5's name (registered within 34 m).

#### Scenario: Friend marks in the fjords
- **WHEN** the player flies high over the runes and then close to the rock
- **THEN** both marks are registered

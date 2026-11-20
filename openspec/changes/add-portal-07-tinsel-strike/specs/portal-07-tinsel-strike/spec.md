# Spec Delta

## Purpose
Day 7, Tinsel Strike: a best-of-9 market brawl between the Humbug Syndicate and the Nutcracker Guard.

## ADDED Requirements

### Requirement: Buy time
Each round SHALL open with 15 seconds of buy time in which players cannot move; the buy menu SHALL sell weapons and equipment the player can afford (the defuse kit only to the Guard, at most two flashbangs and one smoke) and close with Esc without leaving the portal.

#### Scenario: Buy menu and freeze
- **WHEN** the player buys a rifle, grenades and a vest, tries to repurchase and to buy while broke
- **THEN** money drops by the prices, refusals are reported, and the round goes live after buy time

### Requirement: The glitter bomb
An attacker carrying the bomb SHALL plant it by holding use for three seconds inside a site; a defender SHALL defuse it by holding use beside it for ten seconds (five with a kit); a planted bomb SHALL go off after 40 seconds; a fallen carrier SHALL drop it for another attacker to pick up.

#### Scenario: Plant and defuse
- **WHEN** the carrier tries outside a site, lets go midway, plants at A, and a defender with a kit defuses
- **THEN** only the full hold in the site plants, and the Guard wins by defusing while the attackers still earn the plant bonus

### Requirement: Rounds and the match
A round SHALL end when one side is all tagged (attackers only before a plant), the bomb goes off or is defused, or time runs out without a plant. The sides SHALL switch after round 4 with money reset and a checkpoint kept; the first team to 5 wins. A won match SHALL open a climb up the giant tree to the Shining Star. Snowflakes: 1 for the match, +1 for winning both halves, +1 for a 1v3 clutch or a pistol-round ace.

#### Scenario: Rounds halves and the star
- **WHEN** a round times out, one is won by elimination, a 1v5 clutch takes the half and the fifth win comes
- **THEN** the score, money and sides follow the rules, the tree opens and the star wins the portal with 3 snowflakes

#### Scenario: Resume at half time
- **WHEN** the portal resumes at the half-time checkpoint with rendering on
- **THEN** round 5 starts as the Guard with the score kept and money reset, and the bot plays on

#### Scenario: The match from the hall
- **WHEN** the completion bot enters portal 7 on 7 December
- **THEN** it wins the match and climbs to the star

### Requirement: Bots and balance
Bots SHALL play both sides with roles (entry, support, lurker, anchors, mid), rotations when a site is attacked, callouts, grenades, footsteps and smoke awareness; with even skill on both sides, the defenders SHALL win between 35 and 65 percent of rounds.

#### Scenario: Bot vs bot balance
- **WHEN** sixteen seeded bot-vs-bot matches are played with even skill
- **THEN** the Guard's share of round wins lies between 35 and 65 percent

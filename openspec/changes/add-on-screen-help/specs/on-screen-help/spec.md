# Spec Delta

## Purpose
A good-looking on-screen help for the hall and every portal that players can open at any time.

## ADDED Requirements

### Requirement: Help page per world
The hall and every shipped portal SHALL have a help page with a title, the goal, the controls (with keyboard/mouse and gamepad names, including how to close the help) and up to four tips; portals that have not shipped SHALL show the practice range's page.

#### Scenario: Every world has a help page
- **WHEN** the help is looked up for the hall and each of the 24 days
- **THEN** each page has a goal, at most 13 controls including the help key, and every shipped day has its own page

### Requirement: Toggle and pause
F1 or H, the gamepad's Back button and the "?" touch button SHALL toggle the help; while it is open the world SHALL not advance, and closing it (also with Esc) SHALL NOT pass that key press on to the game.

#### Scenario: Help pauses the world
- **WHEN** the player opens the help in a portal, holds forward, then closes it with Esc
- **THEN** the player does not move while the help is open and is still in the portal afterwards

### Requirement: Readable layout
The help SHALL fit the screen: the controls column sizes to its content, tips appear only when they fit completely, and on small screens the remaining controls and the tips move to a second page flipped with the weapon-switch buttons.

#### Scenario: Two pages on small screens
- **WHEN** Day 3's help is drawn on a 320 x 180 screen
- **THEN** it reports two pages and the second page shows different content

### Requirement: Reminder
Entering the hall or a portal SHALL show a small reminder naming the help key for the device in use for a few seconds.

#### Scenario: Reminder on entry
- **WHEN** a portal starts
- **THEN** the help reminder is showing

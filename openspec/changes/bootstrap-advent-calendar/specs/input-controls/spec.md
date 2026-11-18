# Spec Delta

## Purpose
Device-independent controls for keyboard and mouse, gamepads, touch screens and XR controllers, including automatic touch mode and accessibility.

## ADDED Requirements

### Requirement: Action-based input
Gameplay SHALL respond only to abstract actions, and every action SHALL be remappable per player and per device.

#### Scenario: Remap jump
- **WHEN** the player binds jump to the right mouse button
- **THEN** pressing the right mouse button makes the character jump in every portal that uses jump

### Requirement: Automatic touch mode
The web build SHALL show on-screen touch controls when the primary pointer is coarse or a touch event is received, and SHALL hide them when mouse, keyboard or gamepad input is detected afterwards.

#### Scenario: Phone opens the game
- **WHEN** the web build is opened in a mobile browser emulation with touch
- **THEN** a virtual movement stick, look area and context buttons are visible

#### Scenario: Back to keyboard
- **WHEN** touch controls are visible and a key is pressed on a physical keyboard
- **THEN** the touch controls are hidden

### Requirement: Per-portal touch layouts
Each portal SHALL define a touch layout covering every action needed to complete it.

#### Scenario: Completion with touch only
- **WHEN** a portal's completion bot is run through the touch input path
- **THEN** the portal can be completed

### Requirement: Gamepad support
All content SHALL be playable with a standard gamepad, with optional aim assistance.

#### Scenario: Hot-plug gamepad
- **WHEN** a gamepad is connected during play
- **THEN** it becomes usable without restart

### Requirement: XR controls and comfort
In WebXR sessions the system SHALL provide controller-based movement with snap turn, optional teleport, and an acceleration vignette, all configurable.

#### Scenario: Snap turn
- **WHEN** the player flicks the right stick in an XR session with snap turn enabled
- **THEN** the view rotates by the configured angle (default 30°) in a single step

### Requirement: Accessibility options
The system SHALL offer subtitles for all speech, adjustable field of view, screen-shake toggle, hold/toggle options, and colour-blind-safe key and team colours.

#### Scenario: Subtitles
- **WHEN** a voiced line plays with subtitles enabled
- **THEN** its text is shown for the duration of the line

# Spec Delta

## Purpose
The visual contract shared by the hub and all portals: low-resolution retro 3D with dynamic lighting, shadows, meshes, sprites and voxels.

## ADDED Requirements

### Requirement: Low internal resolution
The renderer SHALL render each view at a low internal resolution (selectable from 320×180 to 640×360, per-portal default) and upscale it to the output with nearest-neighbour filtering, preserving the aspect ratio.

#### Scenario: Pixel-crisp upscale
- **WHEN** a 400×225 frame is presented in a 1600×900 window
- **THEN** every internal pixel appears as a uniform 4×4 block

### Requirement: Dynamic lighting and shadows
The renderer SHALL support at least 32 dynamic lights per view, of which at least 4 point or spot lights and 1 directional light cast real-time shadows.

#### Scenario: Moving light casts shadow
- **WHEN** a shadow-casting light moves past an occluding pillar in a golden-frame test
- **THEN** the pillar's shadow moves accordingly in consecutive captured frames

### Requirement: Mixed geometry types
The renderer SHALL draw textured 3D meshes, camera-facing directional sprites (1, 5 or 8 directions) and voxel models in the same scene, and sprites and voxels SHALL receive dynamic lighting and cast shadows.

#### Scenario: Sprite lit by a muzzle flash
- **WHEN** a dynamic light appears next to a sprite enemy
- **THEN** the sprite's lit side brightens in the captured frame

### Requirement: Era emulation options
The renderer SHALL provide per-portal options for 256-colour palette quantisation, ordered dithering, stepped light bands, distance darkness/fog, flat shading, vertex lighting, affine texture mapping and vertex snapping.

#### Scenario: Palette quantisation
- **WHEN** palette quantisation is enabled with a 256-colour palette
- **THEN** every output pixel colour is a member of that palette

### Requirement: Multiple views
The renderer SHALL render up to 4 independent views per frame for split screen and 2 eye views for stereo XR.

#### Scenario: Four-way split screen
- **WHEN** four local players are active
- **THEN** four views are rendered in a 2×2 layout, each from its player's camera

### Requirement: Performance budget
The hub and every portal SHALL sustain 60 frames per second on the reference mid-range mobile device in the web build at default settings.

#### Scenario: Perf smoke test
- **WHEN** the scripted camera path of a portal is rendered headless
- **THEN** draw calls stay at or below 150 and triangles at or below 120 000 per view

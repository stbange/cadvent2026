# Spec Delta

## Purpose
Destructible building fronts for the FPS kit.

## ADDED Requirements

### Requirement: Voxel facades
A facade SHALL be a grid of solid chunks with hit points; a blast SHALL damage chunks in its radius (less at the edge), remove those it breaks along with their solids, bump the facade's version, and drop chunks left with nothing below or beside them.

#### Scenario: Voxel destruction chunking
- **WHEN** a weak blast, then a strong one hit a wall, and a narrow column loses its bottom
- **THEN** the weak one only chips, the strong one opens a hole the eye sees through, and the hanging chunks fall

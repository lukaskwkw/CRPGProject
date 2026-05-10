# Tactical Movement Overview

## Main flow

These files form one tactical movement pipeline:

- `Source/CRPGProject/Framework/Controllers/CRPGProjectPlayerController.*`
  - owns input bindings, tactical tuning `UPROPERTY` values, hover-preview shaping helpers and HUD refresh entry points
- `Source/CRPGProject/Tactical/Components/TacticalMovementControllerComponent.*`
  - owns click handling, hover preview generation, traversal state, budget clamping and temporary collision/avoidance changes during movement
- `Source/CRPGProject/Tactical/Components/TacticalUnitComponent.*`
  - owns unit tactical state: initiative, AP, movement budget and logical occupancy footprint
- `Source/CRPGProject/Framework/Characters/CRPGBaseCharacter.*`
  - hosts the tactical unit component and the hidden navigation blocker that mirrors occupancy into the navmesh

## Detailed responsibilities

### `CRPGProjectPlayerController`

This is the orchestration layer.

- Creates the tactical camera, movement, turn sync and preview components.
- Binds local input for tactical camera, rotation and click-to-move.
- Switches mouse cursor/input mode when camera mode changes.
- Exposes the tuning values used by the preview and traversal algorithms.
- Converts raw nav path points into dense samples for preview readability and builds a sparser traversal control path used by the movement component.

### `TacticalMovementControllerComponent`

This is the runtime movement state machine.

- On hover in turn mode, performs a nav query and builds `PendingMovePreview`.
- Splits the preview into:
  - full path for visualization
  - affordable path for actual commit
- On confirm click, commits only the affordable part.
- During traversal:
  - disables RVO avoidance
  - changes pawn-vs-pawn collision to `Ignore`
  - suppresses the mover's logical occupancy so it does not block itself
  - moves manually with `AddMovementInput` and look-ahead targeting
- On finish or interruption, restores collision/avoidance and consumes movement budget.

The movement component does not manually rebuild nav octree state on hover. Preview queries rely on the current dynamic nav modifier state already mirrored from unit occupancy.

### `TacticalUnitComponent`

This is the tactical data source.

- Stores initiative and turn state.
- Stores remaining movement and AP values consumed by turn mode.
- Defines logical occupancy independent from the character capsule.
- Exposes gameplay occupancy radius/height and separate nav blocker size accessors.
- Supports per-unit nav blocker tuning through `NavigationBlockerRadiusOffsetCm` and `NavigationBlockerHalfHeightOffsetCm`.
- Notifies the owning character when occupancy is suppressed or the unit dies.

Occupancy and blocker sizing are intentionally related but not identical:

- `OccupancyRadiusCm` and `OccupancyHalfHeightCm` describe the gameplay footprint.
- `NavigationBlockerRadiusOffsetCm` and `NavigationBlockerHalfHeightOffsetCm` let designers shrink or expand only the nav blocker capsule.
- Final blocker size is computed as occupancy size plus blocker offset, then clamped to a small positive minimum.

### `CRPGBaseCharacter`

This is the bridge between unit data and world navigation.

- Creates `UTacticalUnitComponent`.
- Creates a hidden `UCapsuleComponent` configured as a nav-only obstacle footprint.
- Registers/unregisters itself with `UTacticalTurnSubsystem` on begin/end play.
- Keeps the blocker size and area synchronized with tactical occupancy settings.
- Pushes blocker shape and area changes into the nav system with `FNavigationSystem::UpdateComponentData(...)` when needed.

## Files that consume these classes

### `Source/CRPGProject/Tactical/Components/TacticalTurnSyncComponent.cpp`

- listens to EventBus turn events
- changes possession to the current active tactical unit
- enables or disables movement depending on whether the active unit is player-controlled
- clears pending preview / active traversal on possession changes

### `Source/CRPGProject/Tactical/Subsystems/TacticalTurnSubsystem.cpp`

- registers `ACRPGBaseCharacter` units
- reads `UTacticalUnitComponent` to build initiative order
- resets per-round movement/AP values
- exposes `GetActiveUnit()` consumed by the controller, HUD and movement component
- does not use `GlobalTimeDilation`; turn flow is state-driven rather than time-slowed

### `Source/CRPGProject/UI/HUD/TacticalCombatHUDWidget.cpp`

- reads active unit movement/AP from `UTacticalUnitComponent`
- reads pending preview state and planning budget from `ACRPGProjectPlayerController`
- forwards confirm/cancel/toggle actions back to the player controller

### `Source/CRPGProject/Tactical/Components/TacticalPathPreviewComponent.cpp`

- renders the preview path each tick using cached world-space points
- colors the affordable segment differently from the over-budget segment

### `Source/CRPGProject/CRPGProjectCharacter.h`

- derives from `ACRPGBaseCharacter`, so it inherits tactical occupancy and unit data automatically

### `Source/CRPGProject/Camera/Components/CameraControllerComponent.cpp`

- reads the active tactical unit to keep the tactical camera centered on the currently controlled unit

## Recommended test settings

These values are useful when testing responsiveness in the editor. They are not final balance recommendations.

### On `ACRPGProjectPlayerController`

Path feel / responsiveness:

- `TacticalAcceptanceRadius`: `35-55`
- `TacticalFinalAcceptanceRadius`: `8-15`
- `TacticalPathRotationInterpSpeed`: `8-14`
- `TraversalLookAheadDistance`: `70-110`
- `TraversalControlPointSpacing`: `80-120`

Preview quality:

- `DenseSampleSpacing`: `30-45`
- `TacticalHoverPreviewRefreshTolerance`: `20-35`
- `TacticalHoverPreviewRefreshInterval`: `0.10-0.20`
- `PreviewSplineSubdivisions`: `1-3`

Occupancy avoidance tuning:

- `TacticalAvoidMargin`: `10-25`
- `TacticalAvoidTestOffsetSmall`: `35-50`
- `TacticalAvoidTestOffsetMedium`: `60-85`
- `TacticalAvoidTestOffsetLarge`: `90-130`

Commit safety:

- `TacticalMinimumCommittedMoveDistance`: `5-15`

### On `UTacticalUnitComponent`

Medium humanoid baseline:

- `bBlocksTacticalMovement`: `true`
- `OccupancyRadiusCm`: `45-60`
- `OccupancyHalfHeightCm`: `90-120`
- `NavigationBlockerRadiusOffsetCm`: `-20` to `0`
- `NavigationBlockerHalfHeightOffsetCm`: `0` to `20`
- `MaxMovementRangeCm`: `700-1000`

Large creature baseline:

- `OccupancyRadiusCm`: `90-140`
- `OccupancyHalfHeightCm`: `140-220`

Very large blocker test:

- `OccupancyRadiusCm`: `160-240`
- `OccupancyHalfHeightCm`: `180-300`

### On `ACRPGBaseCharacter`

There are no primary test tuning values here beyond inherited movement/capsule settings, but verify:

- the actor owns `TacticalUnitComponent`
- the hidden `TacticalOccupancyNavigationBlocker` exists
- runtime navmesh generation is enabled in `Config/DefaultEngine.ini`

## Future Improvements

These are not part of the current implementation, but they are worth revisiting if capsule blockers still produce paths that look too angular or too conservative.

### Navmesh generation experiments

- Try denser navmesh generation settings, especially smaller cells, if corner cutting and obstacle silhouettes still feel too blocky.
- Test alternative Recast generation and tessellation tradeoffs so obstacle edges read more naturally around rounded blockers.
- Revisit how dynamic modifiers interact with tile resolution, because narrow passages can still produce shortcut-looking previews when the generated mesh is too coarse.

### Path preview smoothing

- Keep the current dense resampling as the baseline, but be open to replacing it if a better preview-smoothing method becomes necessary.
- If preview steps still look too segmented, test a different way of distributing intermediate samples along the nav path.
- If needed later, separate movement-authoritative path points from purely visual preview smoothing so readability can improve without changing traversal behavior.

## Suggested test scenarios

1. Two medium units stand close together and the active unit previews a path between them.
   Expected result: preview should route around occupied nav blockers instead of cutting directly through them. NOTE Now sometimes when navmodifier is to narrow the pawn does shortcuts

2. Active unit starts moving through another pawn's physical space.
   Expected result: no capsule blocking during traversal, but movement still ends on the committed affordable path only.

3. A large monster has `OccupancyRadiusCm` doubled.
   Expected result: other units preview a noticeably wider detour and the capsule blocker footprint grows accordingly.

4. A humanoid unit keeps `OccupancyRadiusCm` unchanged but sets `NavigationBlockerRadiusOffsetCm` to `-15`.
   Expected result: the unit still behaves like the same gameplay occupant, but the nav blocker capsule is slightly tighter and produces less conservative routing around the edges.

5. A unit dies during turn mode.
   Expected result: its occupancy blocker disappears and future previews can route through that space.

6. Budget is smaller than the hovered path distance.
   Expected result: preview shows a valid affordable segment and an invalid over-budget tail; commit only moves along the affordable part.

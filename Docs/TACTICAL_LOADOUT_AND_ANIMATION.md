# Tactical Loadout And Animation

## Purpose

This document captures the current runtime contract for tactical character loadouts, weapon attachment, stance-driven animation selection, and AnimBP integration.

The system is intentionally split so C++ owns gameplay/runtime state while Blueprint AnimBP remains a presentation layer that reads explicit exported values.

## Current Runtime Ownership

### Character Side

`ACRPGBaseCharacter` is the runtime host for:

- `UCombatLoadoutComponent`
- per-slot visual mesh components for equipment display
- loadout-aware attack montage resolution fallback
- Blueprint-facing helpers such as:
  - `GetCurrentIdleProfile()`
  - `GetCurrentLocomotionProfile()`
  - `GetCombatStyleCategory()`
  - `GetCombatStanceContext()`
  - `UsesShieldStyle()`
  - `UsesFightIdleProfile()`
  - `UsesFightLocomotionProfile()`

### Component Side

`UCombatLoadoutComponent` is the source of truth for:

- current style category
- current stance context
- resolved idle profile
- resolved locomotion profile
- resolved attack montage set
- equipped visual variants per slot

The component is also responsible for pushing visual refreshes back to the owning character when the loadout changes.

## Style, Stance, And Profiles

### Style Category

Current style categories:

- `Unarmed`
- `Bow`
- `TwoHanded`
- `Shield`
- `TwoWeapons`

### Stance Context

Current stance contexts:

- `Exploration`
- `CombatReady`

### Animation Profiles

Idle and locomotion are resolved separately and both support exploration/fight variants.

Examples:

- `ShieldNoFight`
- `ShieldFight`
- `BowNoFight`
- `BowFight`
- `TwoHandedNoFight`
- `TwoHandedFight`

This means style alone is not enough to choose the final pose. The same style may use different authored idles and locomotion depending on stance.

## Turn Mode Integration

`UTacticalTurnSubsystem` now synchronizes stance context with tactical turn mode for registered `ACRPGBaseCharacter` units.

Rules:

- entering turn mode sets stance to `CombatReady`
- leaving turn mode sets stance to `Exploration`
- units registered while turn mode is already active inherit the current stance immediately

If stance-driven animation does not visually update, the first check should be AnimBP wiring, not the turn subsystem, because runtime logs confirmed stance propagation works for registered units.

## Weapon Attachment Rules

### Slots

Runtime equipment visuals are organized by slot:

- `MainHand`
- `OffHand`
- `TwoHanded`
- `Ranged`

### Default Socket Mapping

Current default mapping on `ACRPGBaseCharacter`:

- `MainHand -> hand_r_socket`
- `OffHand -> hand_l_socket`
- `TwoHanded -> hand_r_socket`
- `Ranged -> spine_socket`

### Shield Style

`Shield` is a style category, not a single mesh.

Typical setup:

- one-handed weapon in `MainHand`
- shield in `OffHand`

Example:

- short sword -> `MainHand` -> `hand_r_socket`
- rounded shield -> `OffHand` -> `hand_l_socket`

### Two-Handed Style

A two-handed weapon usually attaches to one runtime socket, typically `hand_r_socket`, while the authored animation/IK makes the off-hand meet the weapon correctly.

Do not assume two-handed requires two separate runtime attachments.

## Important Socket Lesson

For shared runtime character equipment, prefer **normal skeleton sockets** over **mesh sockets**.

Reason:

- a socket preview can look correct on one mesh asset while runtime attachment fails or snaps near the pawn origin if the actual in-game mesh does not carry the same mesh socket
- skeleton sockets are shared across all skeletal meshes using the same skeleton and are the safer default for equipment attachment

Use mesh sockets only when you intentionally need mesh-specific attachment overrides.

## Variant Authoring Rules

Each `FCombatEquippedVariant` may specify:

- target slot
- variant id
- static mesh or skeletal mesh
- equipped socket name
- holstered socket name
- relative transform

Guidance:

- if preview attachment is already correct on the intended runtime skeleton socket, keep `RelativeTransform` at identity
- only use `RelativeTransform` to compensate for asset-specific pivot/orientation differences
- do not start by compensating bad socket setup with transform offsets

## Loadout Application API

Useful helper APIs on `UCombatLoadoutComponent`:

- `ApplyComposedLoadout(...)`
- `ApplyUnarmedLoadout(...)`
- `ApplyBowLoadout(...)`
- `ApplyTwoHandedLoadout(...)`
- `ApplyShieldLoadout(...)`
- `ApplyTwoWeaponsLoadout(...)`

This allows gameplay code or BP glue to apply a full style package without separate calls for style, variants, stance, and animation profile.

## AnimBP Integration

### Event Graph

On AnimBP update, cache and refresh values from `ACRPGBaseCharacter`:

- `CurrentIdleProfile`
- `CurrentLocomotionProfile`
- optional booleans like `UsesShieldStyle`, `UsesFightIdleProfile`, `UsesFightLocomotionProfile`

### AnimGraph

Recommended structure:

- `Idle` state uses one `Blend Poses by Enum (ECombatIdleProfile)`
- `Move/Locomotion` state uses one `Blend Poses by Enum (ECombatLocomotionProfile)` or equivalent blendspace routing
- attack montage playback continues through the montage slot and does not need style switching inside AnimGraph

Do not use `Switch on Enum` execution nodes inside AnimGraph pose chains.

## Current Known Limitations

- hit reactions remain shared across styles for now
- loadout data is still configured directly in C++/Blueprint rather than loaded from DSL
- no native custom `UAnimInstance` was introduced yet; the current bridge is explicit runtime variables read by AnimBP

## Recommended Next Improvements

- move recurring character loadout presets into data assets or DSL-friendly definitions
- add optional holster/back display state switching for exploration vs combat-ready stance
- add per-style logging toggles or debug widget for faster animation iteration

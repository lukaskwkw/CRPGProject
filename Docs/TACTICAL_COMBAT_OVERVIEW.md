# Tactical Combat Overview

This document describes the current prototype combat slice: how attack targeting is orchestrated, where combat rules live, how movement-to-range works, and which systems are responsible for combat feedback.

## Scope

Current prototype combat supports:

- melee attack intent from the tactical HUD
- ranged attack intent from the tactical HUD
- hover-based target acquisition on enemy units
- move-into-range preview for melee and ranged attacks
- melee move-then-attack follow-up after traversal
- d20 hit resolution with attack bonuses and armor class
- critical hits on natural `20`
- HP loss, death transition, hover highlight, and floating combat text

This is intentionally still a prototype layer.

- combat math is native C++
- combat content is still data-light and component-driven
- no DSL authoring is required yet for this slice
- GAS is still reserved for future combat-system expansion, not used as the current attack resolver

## High-Level Ownership

### `UTacticalCombatHUDWidget`

The HUD is the intent source.

- it exposes attack buttons
- it publishes named EventBus messages such as `melee_attack_requested` and `ranged_attack_requested`
- it renders controller/subsystem state, but does not resolve combat rules itself

### `ACRPGProjectPlayerController`

The player controller is the orchestration layer.

- listens to EventBus HUD intents
- enters and clears combat targeting mode
- tracks the currently hovered enemy target
- requests approach-to-range preview from the movement stack
- validates whether the selected unit may attack now
- rotates the attacker toward the defender before resolving an in-range attack
- defers melee attacks across locomotion when range must be closed first

The controller decides `when` and `against whom` an attack should be attempted.
It does not own hit math or HP math.

### `UTacticalMovementControllerComponent`

The movement component owns locomotion and preview execution.

- asks the controller whether combat targeting wants a preview destination
- reuses the same nav/path preview system used by tactical movement
- commits the movement when an out-of-range attack click should close distance
- calls back into the controller when traversal finishes, so deferred melee attacks can resume through the normal validation path

### `UCombatResolverSubsystem`

The combat resolver owns rule resolution.

- consumes the attacker's action point
- rolls the attack d20
- adds melee or ranged attack bonus
- checks against defender armor class
- rolls damage when the hit lands
- applies critical-hit bonus damage on natural `20`
- applies damage to the target tactical unit
- publishes `attack_resolved`, `unit_damaged`, and `unit_killed`
- triggers target-side combat feedback presentation

The resolver decides `whether the attack hits` and `how much damage it deals`.

### `UTacticalUnitComponent`

The tactical unit component owns per-unit tactical and prototype combat state.

- HP
- armor class
- melee and ranged bonuses
- melee and ranged damage ranges
- melee and ranged attack ranges
- action-point and movement budgets
- logical alive/dead state

### `ACRPGBaseCharacter`

The base character owns world-side combat presentation and death presentation.

- hidden nav blocker for occupied tactical space
- ragdoll/death transition
- custom-depth hover highlight hook
- overhead combat text such as `MISS`, damage amount, and `CRIT X`

## Orchestration Flow

### 1. HUD intent

The player presses `Melee Attack` or `Ranged Attack`.

- `UTacticalCombatHUDWidget` publishes the matching EventBus event
- `ACRPGProjectPlayerController::HandleGameEvent` receives it
- the controller enters combat targeting mode for the selected action

### 2. Enter targeting mode

When targeting mode starts:

- the controller verifies that the possessed pawn is also the active tactical unit
- normal click-to-move is temporarily gated off
- the last hovered target is cleared
- the movement component is allowed to build combat approach previews instead of normal free movement previews

### 3. Hover target acquisition

While targeting mode is active:

- the controller ticks and resolves the unit under the cursor
- pawn hits are preferred over floor hits
- only alive enemy units are accepted as valid targets
- the hovered enemy receives highlight state through the base character
- the HUD refreshes to show current hover target data

### 4. Preview to range

If the hovered target is outside usable range:

- the movement component asks the controller for a preview destination
- the controller computes a point that places the attacker inside legal range, not exactly on the threshold
- occupied radii and a tolerance buffer are considered so the unit does not stop visually in range but logically a few centimeters short
- the normal tactical path preview stack renders that route

### 5. Click to attack

When the player clicks a hovered enemy:

- the controller validates turn ownership, action points, target legality, and current range
- if already in range, the attacker turns toward the target and the resolver is called immediately
- if melee is out of range, the previewed path is committed and the melee action is deferred until traversal completes
- if ranged is out of range, the unit moves into range and then stops, requiring a second explicit attack click by design

### 6. Deferred melee completion

For melee move-then-attack:

- the controller stores the deferred target and action before movement starts
- when traversal completes, the movement component calls back into the controller
- the controller temporarily restores the pending action and re-runs the normal attack validation path
- if range is now valid, the same in-range resolver call executes

This keeps melee follow-up using the same validation logic as a direct in-range click instead of introducing a separate hidden code path.

## Combat Rule Details

### Hit roll

- roll `1d20`
- add melee or ranged attack bonus
- compare against defender `ArmorClass`

### Critical hit

Current crit rule:

- a natural `20` sets `bCriticalHit = true`
- the attack still resolves through the same hit path
- damage is rolled once normally
- then the same damage packet is rolled a second time and added on top

This effectively behaves like rolling the weapon/prototype damage twice on crit.

### Damage application

- `UTacticalUnitComponent::ApplyDamage` reduces HP
- if HP reaches `0`, the unit transitions into logical death
- the owning `ACRPGBaseCharacter` enters tactical death presentation
- death-related events are published for the wider game state

## EventBus Messages

Current attack-related named events include:

- `melee_attack_requested`
- `ranged_attack_requested`
- `attack_resolved`
- `unit_damaged`
- `unit_killed`
- `tactical_unit_died`

The EventBus remains the decoupling seam between input/UI intent, combat results, and future reactive systems.

## Why the Controller Still Looks Busy

`ACRPGProjectPlayerController` still owns a lot because it is currently the seam where these systems meet:

- tactical camera/input mode
- hover cursor world queries
- tactical movement preview gating
- possession and active-turn rules
- HUD refresh triggers

That is valid orchestration responsibility, but it should stay modular.

Current refactor direction:

- keep combat math in `UCombatResolverSubsystem`
- keep locomotion in `UTacticalMovementControllerComponent`
- keep unit state in `UTacticalUnitComponent`
- keep presentation on `ACRPGBaseCharacter` and HUD widgets
- reduce player-controller file size by splitting combat-targeting implementation into a dedicated translation unit or controller-owned helper component

## Recommended Next Refactors

The next safe refactors are:

1. extract EventBus payload parsing/building into small helpers or typed structs
2. split additional tactical UI/selection code out of `ACRPGProjectPlayerController.cpp`
3. formalize combat rules into data assets or DSL-backed authoring once prototype tuning stabilizes
4. replace the temporary text-render combat feedback with a proper world-space widget or Niagara-driven presentation layer

# CRPGProject AI Handoff

Use this file as compact current-state context for future coding chats. It is intentionally shorter and stricter than `Docs/Overview.md`.

## Current Implemented Baseline

- UE5 C++ DnD-inspired CRPG prototype
- Real-time exploration with direct WSAD movement
- Seamless camera switching, including isometric/top-down presentation
- Separate optional turn mode that is distinct from camera mode
- Turn mode can be entered by user demand now; combat-driven entry is planned later
- `ACRPGProjectCharacter` inherits from `ACRPGBaseCharacter`
- `ACRPGBaseCharacter` owns GAS-ready gameplay foundations, including `AbilitySystemComponent`, `AttributeSet`, and `UTacticalUnitComponent`
- `ACRPGProjectPlayerController` is the tactical orchestration center
- NavigationSystem is used for path calculation only
- Actual tactical locomotion is manual per-frame `AddMovementInput`
- `UTacticalTurnSubsystem` manages turn-state flow independently of camera mode
- `UTacticalCombatHUDWidget` exists as a temporary native prototype HUD

## Important Current Boundaries

- The project is not purely turn-based; current baseline is hybrid exploration plus separate turn mode
- Tactical planning as a broader dedicated gameplay mode is not yet a locked current feature
- Tactical camera does not equal turn combat
- Do not replace manual locomotion with `SimpleMoveToLocation`, AI possession, or path-following controllers
- Keep EventBus-driven decoupling between systems
- Use GAS for combat runtime concerns only, not quest/dialogue/world simulation

## Must Not Regress

- Exploration WSAD movement
- Exploration camera look
- Blueprint compatibility for `BP_ThirdPersonCharacter`
- Tactical camera transitions
- Tactical free movement outside turn mode
- Manual `AddMovementInput` path traversal
- Movement budget consumption on arrival
- `UTacticalTurnSubsystem` round progression
- `UTacticalCombatHUDWidget` spawn and update flow
- `ACRPGBaseCharacter` inheritance chain

## Current Tactical Movement State

- Hover preview plus single-click commit is the current movement UX
- Preview tracks path points, path distance, affordability, and hovered destination
- Minimum movement threshold prevents micro-move commits and budget waste
- Path preview rendering is tunable for iteration
- Movement budget consumption is synchronized with actual traversal completion

## Current Tactical HUD State

- `UTacticalCombatHUDWidget` now rebuilds encounter UI data on every refresh from `UTacticalTurnSubsystem` and `UTacticalUnitComponent`
- The top initiative bar is populated from subsystem initiative order and each entry carries portrait, display name, alive state, active state, and initiative value
- The player party panel is always available, is built from registered player-controlled units, and is not limited to turn mode
- Party entry clicks outside turn mode can repossess a player-controlled living unit without forcing a camera mode reset
- Temporary action buttons currently exposed are `Melee Attack`, `Ranged Attack`, and `End Turn`
- `Melee Attack` and `Ranged Attack` publish EventBus intents; `End Turn` advances through `UTacticalTurnSubsystem::EndCurrentUnitTurn()`
- Entry widgets are split between native C++ data delivery and Blueprint presentation: C++ creates/rebuilds children, Blueprint entry widgets render the visuals in `OnViewDataChanged`
- Keep button click ownership in one place only; current intended setup is Blueprint button `OnClicked` wiring that calls the native widget methods, without extra native `OnClicked` auto-binding
- Preferred future direction is to keep Blueprint limited to layout/presentation glue where practical; gameplay rules, ordering, state transitions, and action dispatch should stay in C++ or a future formalized UI state layer
- A later WebUI direction should not materially change that split; only the presentation technology should change, not where gameplay authority lives

## Architecture Rules To Preserve

- Keep gameplay systems modular and event-driven
- Prefer subsystems for global systems
- Prefer composition over inheritance
- Keep gameplay logic in domain modules, not dumped into actors
- Organize new gameplay code by domain: `Core`, `Framework`, `Camera`, `Tactical`, `Combat`, `UI`, `Events`, `World`, `Data`, `Utilities`
- Controllers belong in `Framework/Controllers`
- Base gameplay characters belong in `Framework/Characters`
- Combat GAS classes belong in `Combat/*`
- Future systems should remain compatible with later DSL-driven content authoring
- UI-facing systems should expose clean state because long-term UI direction is Web UI + React, even though native UMG is still used for prototyping

## Recommended Near-Term Next Step

- Tactical Action System: basic melee attack prototype with preview, commit, and action point consumption

## Suggested Prompt Starter

```txt
We are continuing CRPGProject UE5 C++ CRPG architecture.

Current implemented baseline:
- CRPGBaseCharacter + CRPGProjectCharacter unified
- real-time exploration with WSAD works
- camera switching including isometric/top-down view works
- TacticalTurnSubsystem working
- TacticalUnitComponent working
- TacticalCombatHUDWidget working
- tactical movement hover preview + single-click commit working
- manual AddMovementInput path traversal working

Important constraints:
- turn mode is separate from camera mode
- tactical planning as a broader mode is still future-facing, not current baseline
- preserve existing working movement/camera/HUD flow without regressions

Need to continue from this exact stable state.
```

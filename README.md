# CRPGProject

DnD-inspired CRPG prototype in Unreal Engine 5 with a hybrid gameplay foundation:

- third-person real-time exploration
- tactical top-down camera mode
- optional paused turn-based encounter flow
- C++ gameplay systems with GAS-ready character foundations

For the long-form project state and architecture summary, see [Docs/Overview.md](Docs/Overview.md).

## Current Focus

- hybrid exploration and tactical gameplay loop
- initiative-driven tactical encounter management
- manual tactical movement preview and click-to-move traversal
- prototype tactical combat targeting, attack resolution, and combat feedback
- event-driven subsystem architecture

## Project Structure

Main source domains live under `Source/CRPGProject/`:

```txt
Camera/
Combat/
Core/
Data/
Events/
Framework/
Tactical/
UI/
Utilities/
Variant_Combat/
Variant_Platforming/
Variant_SideScrolling/
World/
```

## Build

Recommended Unreal Editor build command:

```powershell
H:/UE_5.7/Engine/Build/BatchFiles/Build.bat CRPGProjectEditor Win64 Development -Project=D:/Ue5_Projects/CRPGProject/CRPGProject.uproject -WaitMutex -NoHotReloadFromIDE
```

If the build fails at link time with `UnrealEditor-CRPGProject.dll` locked, close `UnrealEditor.exe` and rebuild.

## Tactical Notes

- tactical movement uses `NavigationSystem` only for path calculation
- actual locomotion is driven manually through `AddMovementInput`
- tactical camera mode and turn mode are separate systems
- encounter flow is owned by `UTacticalTurnSubsystem`
- combat targeting is owned by `ACRPGProjectPlayerController`, while hit/damage resolution is owned by `UCombatResolverSubsystem`
- the tactical encounter HUD is data-driven from `UTacticalTurnSubsystem` and `UTacticalUnitComponent`
- initiative entries are rebuilt from subsystem order; party entries are rebuilt from registered player-controlled units
- current UMG entry widgets use a hybrid split: C++ supplies data and spawns entry widgets, Blueprint handles visual presentation in `OnViewDataChanged`
- preferred future direction is to keep Blueprint as a thin presentation layer only; gameplay state, ordering, input handling, and actions should stay in native code or a formal UI view-model layer
- this should remain compatible with a future WebUI path, where Blueprint would still mostly act as glue/presentation rather than a place for gameplay logic
- occupied units in turn mode project hidden capsule-shaped nav blockers from `ACRPGBaseCharacter`
- turn mode does not use `GlobalTimeDilation`; tactical pause is controlled by gameplay state, not slowed world time

## Troubleshooting

### Tactical path preview looks globally shifted

If tactical path lines appear offset for all units, do not assume the renderer is wrong first.

Check these in order:

1. Rebuild or recreate the `NavMeshBoundsVolume` and let navmesh regenerate.
2. Verify the issue affects all units, not just a single Blueprint child.
3. Only after that, inspect shared Blueprint parents or inherited mesh overrides.

In this project, a globally shifted tactical preview was resolved by deleting and recreating the `NavMeshBoundsVolume`, which indicates the path preview was drawing bad nav path data rather than applying a wrong visual offset in code.

### Character has no visible mesh

`ACRPGProjectCharacter` provides movement and camera behavior, but the visible skeletal mesh and animation setup are expected to come from Blueprint content derived from the third-person character chain.

## Documentation

- [Docs/Overview.md](Docs/Overview.md)
- [Docs/TACTICAL_COMBAT_OVERVIEW.md](Docs/TACTICAL_COMBAT_OVERVIEW.md)
- [Docs/TACTICAL_OUTLINE_SETUP.md](Docs/TACTICAL_OUTLINE_SETUP.md)
- [Docs/TACTICAL_MOVEMENT_OVERVIEW.md](Docs/TACTICAL_MOVEMENT_OVERVIEW.md)
- [.github/copilot-instructions.md](.github/copilot-instructions.md)

## Future Improvements

- keep future UMG Blueprints focused on pure presentation and layout, with as little gameplay/UI flow logic in Event Graphs as practical
- if the project shifts more heavily toward WebUI later, the recommended split should stay similar: authoritative state and actions in C++ systems, UI technology handling presentation and interaction wiring only

# ✅ CRPGPROJECT — MASTER PROGRESS SUMMARY (CURRENT STATE)

---

## 1. PROJECT DIRECTION (LOCKED IN)

Projekt nie jest zwykłym action RPG.

Current implemented gameplay foundation:

> **Single-character DnD-inspired CRPG with seamless hybrid camera**
>
> - exploration in third person real-time
> - player-controlled camera switching, including isometric/top-down view
> - optional paused turn-based combat
> - systemic DnD combat rules
>
> Current baseline is hybrid exploration plus a separate turn mode.
> Tactical planning as a broader dedicated mode remains future-facing / experimental.

Long-term direction:

> - later: companions, abilities, reactions, dialogue, world systems

Inspiracje gameplay loop:

- Baldur's Gate 3
- Divinity: Original Sin 2
- Dragon Age: Origins

Core philosophy locked:

> free exploration + tactical pause + deterministic turn decisions

---

---

# 2. SOURCE ARCHITECTURE (REFACTORED)

Source folder is no longer flat template mess.

Current domain architecture:

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

Meaning:

- bounded gameplay domains established
- future scalability prepared
- systems no longer dumped into root Source folder

This folder architecture is now baseline and should be preserved.

---

---

# 3. CHARACTER ARCHITECTURE (IMPORTANT)

Originally UE template used plain `ACharacter`.

Now architecture is unified:

## `ACRPGBaseCharacter`

production gameplay base character:

contains:

- GAS AbilitySystemComponent
- AttributeSet
- TacticalUnitComponent
- tactical subsystem registration/unregistration hooks

Acts as:

> all gameplay-capable controllable or combat units base.

---

## `ACRPGProjectCharacter : public ACRPGBaseCharacter`

Still preserves:

- third person template camera boom
- follow camera
- WSAD movement
- mouse look
- jump
- Enhanced Input compatibility
- BP_ThirdPersonCharacter compatibility

Meaning:

> existing UE template functionality preserved, but now inside gameplay architecture.

This was a critical successful refactor.

---

---

# 4. PLAYER CONTROLLER ARCHITECTURE

`ACRPGProjectPlayerController` is now the tactical orchestration center.

It currently owns:

### camera mode coordination

### tactical click handling

### manual tactical path traversal

### HUD spawning

### event bus listening

### tactical preview/confirm movement orchestration

Important:

AI-driven `SimpleMoveToLocation` was completely removed.

NavigationSystem is now used only for:

> path calculation.

Actual locomotion is:

> manual per-frame `AddMovementInput`.

This means:

- locomotion triggers normal CharacterMovement velocity
- AnimBP ground speed works
- no AI possession weirdness
- full future control over tactical movement interruption

This was major milestone.

---

---

# 5. CAMERA MODE SYSTEM (WORKING)

There is a functioning hybrid camera mode subsystem:

### Exploration Mode

- standard third person movement
- WSAD works
- camera orbit works

### Tactical Mode

- top-down tactical camera
- point and click movement available
- tactical roam available
- tactical input suppression on pawn when needed

Camera mode transitions are event driven and stable.

This system is considered working foundation.

---

---

# 6. TACTICAL TURN SUBSYSTEM (WORKING FOUNDATION)

A real turn-state manager now exists:

## `UTacticalTurnSubsystem`

handles:

- whether turn mode is active
- current round number
- registered tactical units
- active unit tracking
- end turn progression
- reset of per-turn movement/action budgets

Meaning:

> combat can now enter deterministic paused turn mode independent of just camera mode.

Important distinction locked:

### tactical camera ≠ turn combat

Turn mode is separate simulation state.

This is architecturally correct and should stay.

---

---

# 7. TACTICAL UNIT COMPONENT (WORKING)

Each gameplay character now owns:

## `UTacticalUnitComponent`

stores:

- max movement range per turn
- remaining movement range
- action points
- bonus action points
- budget consume/reset logic

Currently movement range is already consumed after successful tactical traversal completion.

Meaning:

> we already have real DnD-style resource budgeting started.

---

---

# 8 TACTICAL MOVEMENT V2 FINAL UX REFINEMENT (UPDATED CURRENT STATE)

Tactical Movement V2 received a further UX refinement and no longer uses double-click style confirmation.

Final behavior is now:

## Hover Preview + Single Click Commit

Current flow:

### mouse hover over navigable terrain:

- planned path preview is rendered continuously
- path distance is recalculated continuously
- affordability is recalculated continuously
- HUD displays planned movement cost in real time

### single left click:

- commits the currently hovered preview path

---

# 9. TACTICAL ENCOUNTER HUD (CURRENT PROTOTYPE STATE)

The temporary tactical HUD has been extended into a dynamic encounter HUD driven from runtime tactical state rather than hardcoded unit references.

Current native UI flow:

- `UTacticalCombatHUDWidget` is still the main HUD shell owned by `ACRPGProjectPlayerController`
- every `RefreshFromSubsystem()` rebuilds encounter data from `UTacticalTurnSubsystem` and `UTacticalUnitComponent`
- the top initiative bar is generated from subsystem initiative order
- the player party panel is generated from registered player-controlled units and remains available outside turn mode
- temporary action buttons are exposed for melee, ranged, and end turn

Current HUD data model per initiative entry:

- portrait texture
- display name
- initiative value
- alive/dead state
- active-unit highlight state

Current HUD data model per party entry:

- referenced player unit
- portrait texture
- display name
- alive/dead state
- active-unit highlight state

Important implementation split:

- C++ owns data gathering, ordering, child widget creation, button action methods, and EventBus publishing
- Blueprint owns the final visual layout and per-entry rendering logic in `OnViewDataChanged`

This split is deliberate for the current prototype phase:

- gameplay state stays authoritative in C++
- UMG iteration on styling, layout, and per-entry visual polish stays fast
- the eventual long-term UI direction can still move to a more formalized web/view-model stack later

Current interaction notes:

- clicking a party entry outside turn mode can repossess that unit if it is player-controlled and alive
- party selection preserves the current camera mode instead of forcing exploration mode
- button click ownership should remain single-sourced to avoid duplicate end-turn triggers; current preferred setup is Blueprint `OnClicked` calling the native widget methods

Future-facing UI guideline:

- when extending native UMG prototypes, prefer keeping Blueprint focused on presentation, styling, and widget composition only
- avoid growing Event Graph logic into gameplay orchestration when the same responsibility can live in C++ systems or a cleaner view-model/state layer
- if the project later leans more heavily into WebUI, the recommended boundary should stay the same: C++ remains authoritative for gameplay state and actions, while the UI technology remains a presentation shell
- unit begins tactical traversal immediately

This creates a much better CRPG planning feel:

> what the player sees is exactly what will execute.

The player now plans movement through cursor positioning,
then confirms with one click.

This is much closer to modern tactical RPG usability.

---

# ADDITION — TACTICAL MOVEMENT SAFETY / MINIMUM DISTANCE THRESHOLD

A minimum tactical movement distance threshold was added.

If the hovered/clicked destination is too close to the current unit position:

- movement is not committed
- movement budget is not consumed
- micro-path noise is ignored

This prevents:

- accidental jitter repositioning
- meaningless 1 cm path commits
- movement budget exploits
- visual clutter from tiny path previews

Movement consumption logic was also normalized so that:

- actual consumed range matches preview range
- minimum threshold is respected
- micro movements do not waste resources

This improves tactical simulation consistency.

---

# ADDITION — TACTICAL PATH DEBUG PREVIEW IS NOW EDITOR-TUNABLE

Path preview rendering parameters are no longer hardcoded.

The tactical preview system now exposes editor-tunable debug settings such as:

- valid path color
- invalid path color
- preview sphere radius
- preview sphere segments
- preview line thickness
- preview persistence timing

This allows:

- gameplay feel tuning without recompilation
- readability adjustments during iteration
- easier later replacement with spline meshes or Niagara visuals

Small feature, but professionally useful.

---

# ADDITION — TACTICAL MOVEMENT V2 IS NOW CONSIDERED USABLE PROTOTYPE COMPLETE

Tactical movement is no longer just a technical proof of concept.

Current movement implementation now includes:

- hover planning preview
- single click commit
- affordability validation
- movement budget synchronization
- minimum movement threshold
- editor tunable preview rendering
- traversal completion consumption

Meaning:

> tactical locomotion has entered practical playable prototype territory.

The movement system is now stable enough to serve as the foundation for:

- attack actions
- spell targeting
- range overlays
- reaction zones
- difficult terrain modifiers

## without requiring another major redesign.

---

# 9. TACTICAL PATH PREVIEW RENDERING

Dedicated preview state exists.

Controller now stores pending move preview data:

- path points
- path distance
- destination
- affordability
- preview active flag

Dedicated `UTacticalPathPreviewComponent` handles rendering.

Meaning:

visualization responsibility is separated from click logic.

Very important for future:

- attack preview
- spell preview
- AoE preview
- jump preview

---

---

# 10. HUD FOUNDATION EXISTS

There is now a spawned:

## `UTacticalCombatHUDWidget`

HUD currently communicates with:

- TacticalTurnSubsystem
- PlayerController pending move preview

Current UI supports:

- turn mode toggle
- end turn
- round number
- remaining movement
- action points
- bonus action points
- planned move distance
- remaining after planned move
- confirm/cancel move

This is temporary UE widget implementation.

Long term plan:

> migrate HUD to React via WebUI plugin later.

But current UE widget is sufficient for prototyping systems.

---

---

# 11. EVENT BUS INTEGRATION

`UEventBusSubsystem` is active.

PlayerController listens for tactical events such as:

- tactical_turn_started
- tactical_turn_ended
- tactical_round_started
- tactical_unit_movement_consumed

HUD refreshes from these events.

Meaning:

systems are beginning to communicate in event-driven style instead of hard references only.

Good scalable direction.

---

---

# 12. WHAT IS ALREADY CONSIDERED STABLE / DO NOT BREAK

These systems are now baseline and should be treated as protected:

### must not regress:

- exploration WSAD movement
- exploration camera look
- BP_ThirdPersonCharacter compatibility
- tactical camera transitions
- tactical free movement outside turn mode
- tactical preview/confirm movement inside turn mode
- manual AddMovementInput path traversal
- movement budget consumption on arrival
- TacticalTurnSubsystem round progression
- TacticalCombatHUDWidget spawn/update
- CRPGBaseCharacter inheritance chain

Any future prompt should explicitly preserve these.

---

---

# 13. WHAT THE PROJECT HAS BECOME (IMPORTANT MENTALLY)

This is no longer:

> "making movement in UE"

This is now:

> building a reusable CRPG tactical simulation framework.

Huge difference.

Because now every next gameplay feature plugs into existing simulation states.

---

---

# 14. NEXT BEST LOGICAL MILESTONES (IN ORDER)

We are exactly at the point where the following features make sense:

## Option A — Tactical Action System (recommended next)

attack command preview + attack commit + action point consume

This creates:
first real combat interaction.

---

## Option B — Initiative Queue / Turn Order

who acts next, enemy placeholder turns, round sequence UI.

---

## Option C — Selectable enemy dummy units on map

for attack target testing.

---

## Option D — Range visualization overlays

movement radius / attack radius circles.

---

# ⭐ MY STRONG RECOMMENDATION FOR NEXT CHAT

Start with:

> Tactical Action System (basic melee attack prototype)

because then the game loop becomes:

- move
- attack
- end turn

which is the first complete CRPG combat loop.

---

---

# ADDITION — LONG TERM DATA ARCHITECTURE: CUSTOM DSL SYSTEM (PLANNED FOUNDATION)

One of the major long-term pillars of the project is the planned migration of a large portion of gameplay and worldbuilding data into a:

> custom CRPG DSL (domain specific language)

This means a proprietary text-based format that will define large parts of game content outside of hardcoded C++.

The DSL is planned to eventually describe:

- class definitions
- races
- spells
- abilities
- items
- buffs/debuffs
- quests
- dialogue trees
- faction relations
- scripted world events
- NPC behaviors
- encounter definitions
- interactable objects
- loot tables
- rule expressions

Primary goal:

> separate content authoring from engine/runtime implementation.

Meaning:

C++ builds runtime systems, interpreters, validators, and execution pipelines, while:
**content itself becomes data-driven through DSL.**

Core philosophy:

> C++ defines systems
> GAS defines combat mechanics
> DSL defines the world/content/rules
> EventBus connects everything

This is a major architectural commitment.

Future gameplay systems should therefore always be designed with the question:

> "can this later be described by DSL data?"

instead of being permanently hardcoded.

`Data/DSL/` already exists as the future home for parsers, schemas, and runtime definitions.

---

# ADDITION — LONG TERM UI ARCHITECTURE: WEB UI MIGRATION (PLANNED)

Current HUD implementation in native UE5 UMG is considered only:

> a temporary gameplay prototyping layer.

The long-term plan is to migrate advanced UI to:

> Web UI plugin + React frontend architecture.

Reason:

UE5 UMG is sufficient for simple prototype interfaces such as:

- debug HUD
- combat labels
- quick buttons

but it becomes increasingly inefficient for complex CRPG-heavy interfaces like:

- inventory grid
- class sheet
- character progression
- feats/perks trees
- spellbook
- quest journal
- codex
- dialogue panels
- merchant screens
- party management
- crafting windows

React/WebUI will provide:

- much faster UI iteration
- richer layouts
- reusable component architecture
- easier state management
- better tooltips / filtering / drag-drop
- modern frontend flexibility

Therefore:

> current `UTacticalCombatHUDWidget` should be treated as a native temporary tactical prototype HUD, not the final UI direction.

Important consequence:

gameplay systems should expose clean C++ state and ViewModel-friendly APIs,
so future React UI can simply consume game state without gameplay logic being coupled to UMG widgets.

---

# ✅ COPY THIS TO NEXT CHAT:

simple first line:

```txt
We are continuing CRPGProject UE5 C++ CRPG architecture.

Current state:
- CRPGBaseCharacter + CRPGProjectCharacter unified
- TacticalTurnSubsystem working
- TacticalUnitComponent working
- TacticalCombatHUDWidget working
- Tactical movement V2 preview/confirm working
- manual AddMovementInput path traversal working
- hybrid exploration/tactical camera working

Need to continue from this exact stable state without regressions.
```

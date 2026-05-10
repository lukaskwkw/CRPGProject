# Copilot Instructions — CRPG Unreal DSL Project

## 1. Project Context

This is a **DnD-inspired CRPG built in Unreal Engine 5** with a hybrid gameplay foundation:

- real-time exploration with direct player control
- seamless camera mode switching, including isometric/top-down presentation
- a separate optional turn mode that is distinct from camera mode
- turn mode may be entered by user demand now and by combat flow later

- C++ = core systems + runtime logic
- GAS (Gameplay Ability System) = combat, stats, status effects
- DSL (JSON-based initially) = narrative, quests, AI behavior, world definitions
- EventBus = global decoupled communication layer

This is NOT a Blueprint-driven project. Blueprints are only allowed for glue/UI/prototyping.

---

## 2. Core Architecture Rules

### 2.1 Separation of Concerns

Always respect:

- Engine Layer (Unreal C++)
- Domain Layer (gameplay systems)
- Data Layer (DSL / JSON)
- Presentation Layer (UI / View)

Never mix responsibilities across layers.

### 2.2 Unreal Engine Class Modifications

When modifying Unreal Engine classes:

- Verify overridden engine methods are not final.
- Include concrete headers for any engine component types used directly.

### 2.3 Character Architecture Refactoring

Character architecture refactors should be minimal and surgical. Unify the playable pawn by inheriting `ACRPGProjectCharacter` from `ACRPGBaseCharacter` while preserving existing camera, movement, jump, Enhanced Input, and blueprint compatibility. Avoid redesigns or new class splits.

---

## 3. Event-Driven Design (MANDATORY)

All systems communicate via EventBus:

Examples:

- combat_started
- npc_killed
- dialogue_started
- quest_updated
- dsl_reloaded

Rules:

- No direct cross-system dependencies
- No hard-coupling between gameplay systems
- Prefer events over function chaining

---

## 4. GAS Usage Rules

Gameplay Ability System is used ONLY for:

- Attributes (Health, Mana, ActionPoints)
- Status effects (buffs/debuffs)
- Combat abilities (spells, attacks, actions)
- Gameplay tags

DO NOT use GAS for:

- quest logic
- dialogue system
- world simulation
- narrative branching

GAS is a combat runtime engine, not a full game framework.

---

## 5. DSL Rules

DSL (JSON initially) defines:

- NPC behavior rules
- dialogue trees
- quests
- encounter definitions
- world state changes

Rules:

- DSL must be hot-reloadable
- DSL must NOT contain engine logic
- DSL must be deterministic
- DSL is source of truth for content

---

## 6. C++ Coding Guidelines

- Prefer Subsystems for global systems
- Prefer composition over inheritance
- Avoid deep inheritance trees
- Keep gameplay logic in domain modules, not in Actors
- Use UE macros only when required

---

## 7. AI Coding Behavior (Copilot / LLM)

When generating code:

- Follow existing subsystem patterns
- Prefer EventBus communication
- Respect GAS boundaries
- Do not introduce Blueprint-heavy solutions
- Do not create unnecessary abstractions
- Keep systems modular and decoupled

If uncertain, ask for clarification rather than guessing architecture.

---

## 8. Naming Conventions

- Subsystems: `U<Name>Subsystem`
- Characters: `A<Name>Character`
- Attributes: `U<Name>AttributeSet`
- DSL files: snake_case.json
- Events: snake_case strings

---

## 9. Current Core Systems

Already implemented:

- EventBusSubsystem
- GameCoreSubsystem
- CRPGBaseCharacter (GAS-ready)
- CRPGAttributeSet

---

## 10. Future Systems (Planned)

- CombatSubsystem (turn-based controller)
- DSLLoaderSubsystem
- DialogueSubsystem
- QuestSubsystem
- AIBehaviorSubsystem
- CameraModeSubsystem

---

## 11. Hard Constraints

- No hex-grid combat (navmesh only)
- No monolithic gameplay classes
- No tight coupling between systems
- No Blueprint as primary logic layer

---

## 12. Core Philosophy

"C++ defines systems. GAS defines combat simulation. DSL defines the world. EventBus connects everything."

---

## 13. Tactical Movement Rules

For the possessed player character, use the NavigationSystem only for path calculation. Actual locomotion should be driven manually each frame via `CharacterMovement::AddMovementInput`, rather than using AI-driven movement systems like `SimpleMoveToLocation` or `AIController` path following.

Additionally, implement hover-based path preview for tactical movement UX. Use click to commit actions and allow over-budget clicks by clamping traversal to the affordable portion while distinctly previewing over-budget segments.

For occupied tactical space, prefer dynamic nav modifiers owned by `ACRPGBaseCharacter` over bespoke preview avoidance logic. The shared blocker should stay hidden, nav-only, and capsule-shaped so turn-mode occupancy reads closer to the character footprint than a square box.

Do not use `GlobalTimeDilation` to simulate tactical pause. Turn mode should keep normal world time and rely on subsystem state, possession changes, movement gating, and nav blockers instead of slowing the entire world.

---

## 14. Source Folder Architecture Rules

Never place new gameplay classes in Source/CRPGProject root.

Always organize new classes by bounded gameplay domain:

- Core/\*
- Framework/\*
- Camera/\*
- Tactical/\*
- Combat/\*
- UI/\*
- Events/\*
- World/\*
- Data/\*
- Utilities/\*

Subsystems must live inside their owning domain's Subsystems folder. Components must live inside their owning domain's Components folder. Controllers must live inside Framework/Controllers. Base gameplay characters must live inside Framework/Characters. Combat GAS classes must live inside Combat/\*.

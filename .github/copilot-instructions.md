# Copilot Instructions — CRPG Unreal DSL Project

## 1. Project Context

This is a **turn-based DND CRPG built in Unreal Engine 5** with a hybrid architecture:

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

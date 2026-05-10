# ✅ CRPGPROJECT — GAMEPLAY DOMAIN ARCHITECTURE CONSTITUTION v1

---

---

# I. CORE DEVELOPMENT PHILOSOPHY

CRPGPROJECT is not a feature pile.

CRPGPROJECT is:

> **a reusable CRPG tactical simulation framework with game content built on top.**

Meaning:

every new implementation must be evaluated not as:

> "does this feature work?"

but as:

> "does this feature fit the long-term simulation architecture?"

Short-term hacks that violate domain ownership are forbidden unless explicitly marked as temporary prototype code.

---

---

# II. PRIMARY BOUNDED CONTEXTS (DOMAIN OWNERSHIP)

Every gameplay rule in the project must belong to exactly one primary domain.

No rule should float between arbitrary classes.

---

## 1. Character Domain

Folder:

```txt id="gq7w9v"
Core/
```

Owns:

* gameplay-capable character base classes
* GAS ownership
* AttributeSet ownership
* TacticalUnitComponent ownership
* faction metadata
* identity metadata
* actor-level registration hooks

Main classes:

```txt id="b62gzz"
ACRPGBaseCharacter
ACRPGProjectCharacter
UTacticalUnitComponent
UCRPGAttributeSet
```

Character domain represents:

> "what this entity is"

Character domain must NOT own:

* initiative queue logic
* hit resolution formulas
* UI decisions

---

## 2. Tactical Domain

Folder:

```txt id="ukm6z2"
Tactical/
```

Owns:

* turn mode state
* encounter lifecycle
* initiative order
* active unit tracking
* movement/action budget resets
* tactical path preview state
* range affordability checks
* tactical action gating

Main classes:

```txt id="k2zfjh"
UTacticalTurnSubsystem
UTacticalPathPreviewComponent
future: UTacticalRangeOverlayComponent
```

Tactical domain represents:

> "whose turn is it and what actions are currently legal"

Tactical domain must NOT own:

* damage formulas
* HP subtraction
* widget rendering

---

## 3. Combat Domain

Folder:

```txt id="1pdrj3"
Combat/
```

Owns:

* attack requests
* target validation
* dice rolling
* hit/miss resolution
* damage resolution
* death resolution
* status effect application
* combat log transactions

Main classes (planned):

```txt id="1gdu5j"
UCombatResolverSubsystem
FCombatAttackRequest
FCombatAttackResult
```

Combat domain represents:

> "what happens when one unit performs an offensive action"

Combat domain must NOT own:

* turn ownership
* camera transitions
* widget buttons

---

## 4. Camera Domain

Folder:

```txt id="a4c8mi"
Camera/
```

Owns:

* exploration/tactical camera transitions
* focus transitions
* zoom/orbit logic
* active pawn framing

Camera must never own combat rules.

---

## 5. UI Domain

Folder:

```txt id="x3g51k"
UI/
```

Owns:

* HUD widgets
* visual rebuilds
* portrait bars
* labels
* tooltips
* action button surfaces

UI domain represents:

> presentation only.

UI must NEVER decide gameplay legality.

UI may ask:

> "what is active unit?"

but may never decide:

> "who should become active unit?"

---

## 6. Event Domain

Folder:

```txt id="fd3yk7"
Events/
```

Owns:

* decoupled gameplay broadcasts
* cross-domain notifications

Main class:

```txt id="r9tbxj"
UEventBusSubsystem
```

No gameplay domain should hard-reference unrelated domains if an event can be used instead.

---

## 7. Data Domain

Folder:

```txt id="vhl3m8"
Data/
Data/DSL/
```

Owns:

* future externalized gameplay definitions
* parsers
* runtime data assets
* schema validators

Data domain never owns simulation logic.

It feeds simulation logic.

---

---

# III. AGGREGATE ROOTS (SIMULATION SOURCES OF TRUTH)

Each large gameplay state cluster has exactly one authority.

---

## Tactical Encounter Aggregate Root

```txt id="q6s0s0"
UTacticalTurnSubsystem
```

Source of truth for:

* encounter running?
* round number
* initiative order
* active unit
* turn advancement

No other class may privately maintain its own initiative ordering.

---

## Combat Resolution Aggregate Root

```txt id="4tk2hv"
UCombatResolverSubsystem (planned)
```

Source of truth for:

* attack transaction processing
* hit/miss result
* damage result
* kill result

No UI or Character class may directly compute combat result independently.

---

## Unit Combatant Aggregate Root

```txt id="2rn2jv"
UTacticalUnitComponent + AttributeSet + owning ACRPGBaseCharacter
```

Source of truth for:

* team
* alive/dead
* movement budget
* action budget
* current initiative
* HP/AP stats

---

---

# IV. PLAYER CONTROLLER RESTRICTION RULES (VERY IMPORTANT)

`ACRPGProjectPlayerController` is an orchestration shell only.

It may own:

* input interpretation
* click detection
* camera requests
* HUD spawn
* tactical command forwarding

It must NOT become:

* combat calculator
* initiative manager
* permanent gameplay state storage

PlayerController asks domains to do work.

PlayerController must not become the domain itself.

This rule is critical because UE templates naturally push too much into controller.

---

---

# V. HUD / UI RESTRICTION RULES

Widgets are read-only observers plus command emitters.

Allowed:

```txt id="s7w2w5"
Button clicked -> request melee attack
```

Forbidden:

```txt id="9lvt3l"
Button clicked -> widget calculates damage and edits HP directly
```

All gameplay mutation must route through proper domain services/subsystems.

---

---

# VI. CROSS DOMAIN COMMUNICATION LAW

Preferred order:

### 1. direct call only within same domain

### 2. subsystem getter query across domains if read-only

### 3. EventBus broadcast for reactive notifications

Avoid:

* arbitrary actor-to-widget references
* widget-to-character direct mutation chains
* controller-to-everything god references

---

---

# VII. COPILOT SAFETY LAW

Every future Copilot prompt must explicitly state:

> preserve domain ownership and do not place business logic in PlayerController or Widgets if it belongs to Tactical/Combat subsystems.

Without this sentence Copilot will continuously generate bad Unreal-style spaghetti.

This is mandatory.

---

---

# VIII. DSL COMPATIBILITY LAW

Every new gameplay rule should be implemented with future external data in mind.

Ask:

> can hardcoded constants later be supplied by DSL?

Examples:

* attack range
* weapon damage dice
* portrait references
* display names
* armor class
* initiative modifiers

Current hardcoding is acceptable only if variables are exposed for later data migration.

---

---

# IX. TEMPORARY PROTOTYPE EXCEPTION

Temporary prototype code is allowed only if:

* clearly isolated
* easily replaceable
* does not corrupt aggregate ownership

Meaning:

temporary UMG widget = acceptable
temporary hardcoded goblin stats = acceptable

but:

temporary combat logic inside widget = not acceptable.

---

---

# X. GOLDEN RULE OF FEATURE IMPLEMENTATION

Before implementing any feature, ask:

### Which domain owns this?

### Which class is source of truth?

### Who observes it?

### Can this later be data driven?

If these four answers are unclear:

implementation should pause until architecture is clarified.

#include "TacticalCombatHUDWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "UI/HUD/TacticalInitiativeEntryWidget.h"
#include "UI/HUD/TacticalHUDActionEvents.h"
#include "UI/HUD/TacticalPartyEntryWidget.h"

void UTacticalCombatHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (MeleeAttackButton)
    {
        MeleeAttackButton->OnClicked.RemoveDynamic(this, &UTacticalCombatHUDWidget::OnMeleeAttackPressed);
        MeleeAttackButton->OnClicked.AddDynamic(this, &UTacticalCombatHUDWidget::OnMeleeAttackPressed);
    }

    if (RangedAttackButton)
    {
        RangedAttackButton->OnClicked.RemoveDynamic(this, &UTacticalCombatHUDWidget::OnRangedAttackPressed);
        RangedAttackButton->OnClicked.AddDynamic(this, &UTacticalCombatHUDWidget::OnRangedAttackPressed);
    }

    // EndTurn and GoToActive are already wired in the current widget blueprint.
    // Binding them again in native code causes those actions to fire twice.

    // The widget does not cache an initialization snapshot; every construct starts from authoritative gameplay state.
    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        PlayerController->NotifyTacticalUIHoverBegin();
    }

    ClearPendingMovePreviewFromUI();
}

void UTacticalCombatHUDWidget::NativeOnMouseLeave(const FPointerEvent &InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);

    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        PlayerController->NotifyTacticalUIHoverEnd();
    }
}

void UTacticalCombatHUDWidget::OnToggleTurnModePressed()
{
    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        // Turn mode authority stays in the subsystem so the HUD only raises intent here.
        TacticalTurnSubsystem->ToggleTurnMode();
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnEndTurnPressed()
{
    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        const ACRPGBaseCharacter *SelectedUnit = GetOwningCRPGPlayerController() ? Cast<ACRPGBaseCharacter>(GetOwningCRPGPlayerController()->GetPawn()) : nullptr;
        const ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit();

        // End Turn is intentionally tied to the currently selected pawn matching the active unit.
        // This keeps off-turn ally inspection/focus from accidentally advancing initiative.
        if (TacticalTurnSubsystem->IsTurnModeActive() && SelectedUnit != nullptr && SelectedUnit == ActiveUnit)
        {
            TacticalTurnSubsystem->EndCurrentUnitTurn();
        }
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnGoToActivePressed()
{
    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        if (ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit())
        {
            if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
            {
                // Shared controller routing means this can either repossess the active ally or just focus camera,
                // depending on who owns the unit and whether the narrow debug enemy-control rule applies.
                PlayerController->SelectOrFocusTacticalUnit(ActiveUnit);
            }
        }
    }

    RefreshFromSubsystem();
}

FText UTacticalCombatHUDWidget::GetEndTurnActionLabel() const
{
    return EndTurnActionLabel;
}

FText UTacticalCombatHUDWidget::GetGoToActiveActionLabel() const
{
    return GoToActiveActionLabel;
}

bool UTacticalCombatHUDWidget::IsPointerOverBlockingUI() const
{
    const auto IsWidgetHovered = [](const UWidget *Widget)
    {
        return Widget && Widget->IsHovered();
    };

    // Only small actionable HUD elements should suppress world-space hover preview.
    // The root widget spans the screen, so using its hover state would block preview everywhere.
    return IsWidgetHovered(MeleeAttackButton) ||
           IsWidgetHovered(RangedAttackButton) ||
           IsWidgetHovered(EndTurnButton) ||
           IsWidgetHovered(GoToActiveButton) ||
           IsAnyPanelChildHovered(InitiativeBarContainer) ||
           IsAnyPanelChildHovered(PlayerPartyContainer);
}

void UTacticalCombatHUDWidget::OnMeleeAttackPressed()
{
    // Combat execution is still external; the prototype HUD only publishes intent onto the EventBus.
    PublishActionRequestedEvent(TacticalHUDActionEvents::MeleeAttackRequested);
    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnRangedAttackPressed()
{
    // Combat execution is still external; the prototype HUD only publishes intent onto the EventBus.
    PublishActionRequestedEvent(TacticalHUDActionEvents::RangedAttackRequested);
    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::RefreshFromSubsystem()
{
    // Reset to a safe empty state first so partial subsystem/controller availability never leaves stale UI behind.
    bTurnModeEnabled = false;
    RoundNumber = 0;
    RemainingMovementRangeCm = 0.0f;
    PlannedMoveDistanceCm = 0.0f;
    RemainingMovementAfterPlannedMoveCm = 0.0f;
    ActionPoints = 0;
    BonusActionPoints = 0;
    CurrentTurnState = ETacticalTurnState::Disabled;
    TurnModeLabel = TEXT("OFF");
    bHasPendingMovePreview = false;
    bPendingMoveValid = false;
    bMovementEnabled = true;
    PendingCombatAction = ECombatActionType::None;
    CurrentCombatTargetingMode = ECombatTargetingMode::None;
    HoveredTargetDisplayName.Reset();
    HoveredTargetCurrentHP = 0;
    HoveredTargetMaxHP = 0;
    bHoveredTargetInRange = false;
    ResetEncounterWidgetData();

    float AvailableMovementForPlanningCm = 0.0f;
    const UTacticalUnitComponent *ActiveUnitComponent = nullptr;

    // HUD data is stitched together from two sources: turn ownership/state lives in the subsystem,
    // while pending preview and movement-toggle state live on the controller-owned movement stack.
    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        bTurnModeEnabled = TacticalTurnSubsystem->IsTurnModeActive();
        RoundNumber = TacticalTurnSubsystem->GetCurrentRound();
        CurrentTurnState = TacticalTurnSubsystem->GetCurrentState();
        TurnModeLabel = bTurnModeEnabled ? TEXT("ON") : TEXT("OFF");

        if (ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit())
        {
            if (UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit->GetTacticalUnitComponent())
            {
                ActiveUnitComponent = TacticalUnitComponent;
                RemainingMovementRangeCm = TacticalUnitComponent->GetRemainingMovementRange();
                AvailableMovementForPlanningCm = RemainingMovementRangeCm;
                ActionPoints = TacticalUnitComponent->GetActionPoints();
                BonusActionPoints = TacticalUnitComponent->GetBonusActionPoints();
            }
        }

        RebuildEncounterWidgetData(*TacticalTurnSubsystem);

        if (!ActionEntries.Num())
        {
            RebuildActionEntries(TacticalTurnSubsystem, ActiveUnitComponent);
        }
    }
    else
    {
        RebuildActionEntries(nullptr, nullptr);
    }

    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        // The controller exposes pending preview and planning-budget data computed by the movement component.
        const FTacticalMovePreviewData &PendingMovePreview = PlayerController->GetPendingMovePreview();
        AvailableMovementForPlanningCm = PlayerController->GetAvailableMovementBudgetForPlanningRequest();
        bHasPendingMovePreview = PendingMovePreview.bHasPreview;
        bPendingMoveValid = PendingMovePreview.AffordablePathDistanceCm > KINDA_SMALL_NUMBER;
        bMovementEnabled = PlayerController->IsTurnModeMovementEnabled();
        PlannedMoveDistanceCm = PendingMovePreview.PathDistanceCm;
        RemainingMovementAfterPlannedMoveCm = FMath::Max(0.0f, AvailableMovementForPlanningCm - PendingMovePreview.AffordablePathDistanceCm);
        PendingCombatAction = PlayerController->GetPendingCombatAction();
        CurrentCombatTargetingMode = PlayerController->GetCurrentCombatTargetingMode();

        if (UTacticalUnitComponent *HoveredTargetUnit = PlayerController->GetHoveredTargetUnit())
        {
            HoveredTargetDisplayName = HoveredTargetUnit->GetDisplayName();
            HoveredTargetCurrentHP = HoveredTargetUnit->GetCurrentHP();
            HoveredTargetMaxHP = HoveredTargetUnit->GetMaxHP();

            if (ACRPGBaseCharacter *SelectedUnit = Cast<ACRPGBaseCharacter>(PlayerController->GetPawn()))
            {
                if (UTacticalUnitComponent *SelectedUnitComponent = SelectedUnit->GetTacticalUnitComponent())
                {
                    bHoveredTargetInRange = PlayerController->IsTargetInCombatRange(
                        SelectedUnitComponent,
                        HoveredTargetUnit,
                        PendingCombatAction);
                }
            }
        }
    }

    // Blueprint redraw happens first so native child rebuilding and button-state sync both consume the same fresh data.
    OnTacticalCombatDataRefreshed();
    RebuildEncounterWidgetChildren();
    SyncActionButtonStates();
}

void UTacticalCombatHUDWidget::OnConfirmMovePressed()
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        // UI never moves the pawn directly; it forwards intent back to the controller/movement component.
        PlayerController->CommitPendingTacticalMoveRequest();
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnToggleMovementEnabledPressed()
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        // Movement gating is owned by turn-sync/controller code, not by the HUD widget itself.
        PlayerController->ToggleTurnModeMovementEnabled();
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::SetMovementEnabled(bool bEnabled)
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        // This explicit setter exists for UI bindings that want a direct on/off path instead of a toggle.
        PlayerController->SetTurnModeMovementEnabled(bEnabled);
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnCancelMovePressed()
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        // Cancel preview stays on the controller side because only it owns the movement component state.
        PlayerController->ClearPendingTacticalMovePreviewRequest();
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::ClearPendingMovePreviewFromUI() const
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        PlayerController->ClearPendingTacticalMovePreviewRequest();
    }
}

bool UTacticalCombatHUDWidget::IsAnyPanelChildHovered(const UPanelWidget *PanelWidget) const
{
    if (!PanelWidget)
    {
        return false;
    }

    // Some UMG layouts hover the panel itself, others only the spawned child widget, so we check both.
    if (PanelWidget->IsHovered())
    {
        return true;
    }

    for (int32 ChildIndex = 0; ChildIndex < PanelWidget->GetChildrenCount(); ++ChildIndex)
    {
        const UWidget *ChildWidget = PanelWidget->GetChildAt(ChildIndex);
        if (ChildWidget && ChildWidget->IsHovered())
        {
            return true;
        }
    }

    return false;
}

void UTacticalCombatHUDWidget::ResetEncounterWidgetData()
{
    // View-model arrays are rebuilt wholesale each refresh to avoid stale per-entry state surviving turn changes.
    InitiativeEntries.Reset();
    PartyEntries.Reset();
    ActionEntries.Reset();
}

void UTacticalCombatHUDWidget::RebuildEncounterWidgetData(const UTacticalTurnSubsystem &TacticalTurnSubsystem)
{
    const UTacticalUnitComponent *CurrentActiveUnit = TacticalTurnSubsystem.GetCurrentActiveUnit();
    const TArray<UTacticalUnitComponent *> &InitiativeOrder = TacticalTurnSubsystem.GetInitiativeOrder();
    const TArray<TWeakObjectPtr<ACRPGBaseCharacter>> &RegisteredUnits = TacticalTurnSubsystem.GetRegisteredUnits();
    const ACRPGBaseCharacter *SelectedUnit = GetOwningCRPGPlayerController() ? Cast<ACRPGBaseCharacter>(GetOwningCRPGPlayerController()->GetPawn()) : nullptr;

    InitiativeEntries.Reserve(InitiativeOrder.Num());
    PartyEntries.Reserve(RegisteredUnits.Num());

    for (const UTacticalUnitComponent *UnitComponent : InitiativeOrder)
    {
        if (!UnitComponent)
        {
            continue;
        }

        // Initiative entries mirror encounter order exactly, including dead units that remain visible in the tracker.
        const bool bIsActive = UnitComponent == CurrentActiveUnit;
        ACRPGBaseCharacter *Character = Cast<ACRPGBaseCharacter>(UnitComponent->GetOwner());

        FTacticalInitiativeEntryViewData InitiativeEntry;
        InitiativeEntry.Unit = Character;
        InitiativeEntry.Initiative = UnitComponent->GetCurrentInitiative();
        InitiativeEntry.PortraitTexture = UnitComponent->GetPortraitTexture();
        InitiativeEntry.DisplayName = UnitComponent->GetDisplayName();
        InitiativeEntry.CurrentHP = UnitComponent->GetCurrentHP();
        InitiativeEntry.MaxHP = UnitComponent->GetMaxHP();
        InitiativeEntry.HealthFraction = UnitComponent->GetHealthFraction();
        InitiativeEntry.bIsAlive = UnitComponent->IsAlive();
        InitiativeEntry.bIsActive = bIsActive;
        InitiativeEntries.Add(MoveTemp(InitiativeEntry));
    }

    for (const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit : RegisteredUnits)
    {
        ACRPGBaseCharacter *Character = RegisteredUnit.Get();
        UTacticalUnitComponent *UnitComponent = Character ? Character->GetTacticalUnitComponent() : nullptr;
        if (!Character || !UnitComponent || !UnitComponent->IsPlayerControlled())
        {
            continue;
        }

        // The party panel is limited to player-controlled units even if enemies are registered in the encounter.
        FTacticalPartyEntryViewData PartyEntry;
        PartyEntry.Unit = Character;
        PartyEntry.PortraitTexture = UnitComponent->GetPortraitTexture();
        PartyEntry.DisplayName = UnitComponent->GetDisplayName();
        PartyEntry.CurrentHP = UnitComponent->GetCurrentHP();
        PartyEntry.MaxHP = UnitComponent->GetMaxHP();
        PartyEntry.HealthFraction = UnitComponent->GetHealthFraction();
        PartyEntry.bIsAlive = UnitComponent->IsAlive();
        PartyEntry.bIsActive = Character == SelectedUnit;
        PartyEntries.Add(MoveTemp(PartyEntry));
    }

    RebuildActionEntries(&TacticalTurnSubsystem, CurrentActiveUnit);
}

void UTacticalCombatHUDWidget::RebuildActionEntries(const UTacticalTurnSubsystem *TacticalTurnSubsystem, const UTacticalUnitComponent *ActiveUnitComponent)
{
    const ACRPGBaseCharacter *SelectedUnit = GetOwningCRPGPlayerController() ? Cast<ACRPGBaseCharacter>(GetOwningCRPGPlayerController()->GetPawn()) : nullptr;
    const ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
    const bool bSelectedUnitCanAct = TacticalTurnSubsystem &&
                                     TacticalTurnSubsystem->IsTurnModeActive() &&
                                     ActiveUnitComponent != nullptr &&
                                     ActiveUnitComponent->IsAlive() &&
                                     SelectedUnit != nullptr &&
                                     SelectedUnit == ActiveUnit;

    // Action availability is derived from the selected pawn rather than from the highlighted initiative entry,
    // because the user can inspect or select allies off-turn without granting them movement or actions.
    ActionEntries.Reset();
    ActionEntries.Reserve(4);

    FTacticalActionEntryViewData MeleeAction;
    MeleeAction.ActionType = ETacticalHUDActionType::MeleeAttack;
    MeleeAction.Label = FText::FromString(TEXT("Melee Attack"));
    MeleeAction.bIsEnabled = bSelectedUnitCanAct;
    ActionEntries.Add(MoveTemp(MeleeAction));

    FTacticalActionEntryViewData RangedAction;
    RangedAction.ActionType = ETacticalHUDActionType::RangedAttack;
    RangedAction.Label = FText::FromString(TEXT("Ranged Attack"));
    RangedAction.bIsEnabled = bSelectedUnitCanAct;
    ActionEntries.Add(MoveTemp(RangedAction));

    FTacticalActionEntryViewData EndTurnAction;
    EndTurnAction.ActionType = ETacticalHUDActionType::EndTurn;
    EndTurnAction.Label = GetEndTurnActionLabel();
    EndTurnAction.bIsEnabled = bSelectedUnitCanAct;
    ActionEntries.Add(MoveTemp(EndTurnAction));

    FTacticalActionEntryViewData GoToActiveAction;
    GoToActiveAction.ActionType = ETacticalHUDActionType::GoToActive;
    GoToActiveAction.Label = GetGoToActiveActionLabel();
    GoToActiveAction.bIsEnabled = TacticalTurnSubsystem &&
                                  TacticalTurnSubsystem->IsTurnModeActive() &&
                                  ActiveUnitComponent != nullptr &&
                                  ActiveUnitComponent->IsAlive() &&
                                  (SelectedUnit == nullptr || SelectedUnit != ActiveUnit);
    ActionEntries.Add(MoveTemp(GoToActiveAction));
}

void UTacticalCombatHUDWidget::RebuildEncounterWidgetChildren()
{
    // Widget children are recreated from view data each refresh so Blueprint presentation stays dumb and stateless.
    RebuildInitiativeBarChildren();
    RebuildPartyPanelChildren();
}

void UTacticalCombatHUDWidget::RebuildInitiativeBarChildren()
{
    if (!InitiativeBarContainer)
    {
        return;
    }

    InitiativeBarContainer->ClearChildren();

    if (!InitiativeEntryWidgetClass)
    {
        return;
    }

    for (const FTacticalInitiativeEntryViewData &Entry : InitiativeEntries)
    {
        UTacticalInitiativeEntryWidget *EntryWidget = CreateWidget<UTacticalInitiativeEntryWidget>(this, InitiativeEntryWidgetClass);
        if (!EntryWidget)
        {
            continue;
        }

        EntryWidget->SetViewData(Entry);
        InitiativeBarContainer->AddChild(EntryWidget);
    }
}

void UTacticalCombatHUDWidget::RebuildPartyPanelChildren()
{
    if (!PlayerPartyContainer)
    {
        return;
    }

    PlayerPartyContainer->ClearChildren();

    if (!PartyEntryWidgetClass)
    {
        return;
    }

    for (const FTacticalPartyEntryViewData &Entry : PartyEntries)
    {
        UTacticalPartyEntryWidget *EntryWidget = CreateWidget<UTacticalPartyEntryWidget>(this, PartyEntryWidgetClass);
        if (!EntryWidget)
        {
            continue;
        }

        EntryWidget->SetViewData(Entry);
        PlayerPartyContainer->AddChild(EntryWidget);
    }
}

void UTacticalCombatHUDWidget::SyncActionButtonStates()
{
    // Optional direct button bindings are treated as a projection of ActionEntries, not a second source of truth.
    auto FindActionEnabled = [this](ETacticalHUDActionType ActionType)
    {
        if (const FTacticalActionEntryViewData *ActionEntry = ActionEntries.FindByPredicate([ActionType](const FTacticalActionEntryViewData &Entry)
                                                                                            { return Entry.ActionType == ActionType; }))
        {
            return ActionEntry->bIsEnabled;
        }

        return false;
    };

    if (MeleeAttackButton)
    {
        MeleeAttackButton->SetIsEnabled(FindActionEnabled(ETacticalHUDActionType::MeleeAttack));
    }

    if (RangedAttackButton)
    {
        RangedAttackButton->SetIsEnabled(FindActionEnabled(ETacticalHUDActionType::RangedAttack));
    }

    if (EndTurnButton)
    {
        EndTurnButton->SetIsEnabled(FindActionEnabled(ETacticalHUDActionType::EndTurn));
    }

    if (GoToActiveButton)
    {
        GoToActiveButton->SetIsEnabled(FindActionEnabled(ETacticalHUDActionType::GoToActive));
    }
}

void UTacticalCombatHUDWidget::PublishActionRequestedEvent(const FString &EventName) const
{
    UEventBusSubsystem *EventBusSubsystem = GetEventBusSubsystem();
    const UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    const ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
    const UTacticalUnitComponent *ActiveUnitComponent = ActiveUnit ? ActiveUnit->GetTacticalUnitComponent() : nullptr;

    // EventBus payload is intentionally lightweight; gameplay systems can resolve richer actor state from the unit name/id.
    if (!EventBusSubsystem || !TacticalTurnSubsystem || !ActiveUnit || !ActiveUnitComponent || !TacticalTurnSubsystem->IsTurnModeActive())
    {
        return;
    }

    const FString Payload = FString::Printf(
        TEXT("round=%d;active_unit=%s;display_name=%s"),
        TacticalTurnSubsystem->GetCurrentRound(),
        *ActiveUnit->GetName(),
        *ActiveUnitComponent->GetDisplayName());

    EventBusSubsystem->PublishEvent(EventName, Payload);
}

ACRPGProjectPlayerController *UTacticalCombatHUDWidget::GetOwningCRPGPlayerController() const
{
    return GetOwningPlayer<ACRPGProjectPlayerController>();
}

UTacticalTurnSubsystem *UTacticalCombatHUDWidget::GetTacticalTurnSubsystem() const
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        return GameInstance->GetSubsystem<UTacticalTurnSubsystem>();
    }

    return nullptr;
}

UEventBusSubsystem *UTacticalCombatHUDWidget::GetEventBusSubsystem() const
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        return GameInstance->GetSubsystem<UEventBusSubsystem>();
    }

    return nullptr;
}

#include "TacticalTurnSyncComponent.h"

#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "Tactical/Components/TacticalMovementControllerComponent.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

namespace TacticalTurnSyncEvents
{
    static const FString TacticalTurnStarted = TEXT("tactical_turn_started");
    static const FString TacticalTurnEnded = TEXT("tactical_turn_ended");
    static const FString TacticalEncounterStarted = TEXT("tactical_encounter_started");
    static const FString TacticalRoundStarted = TEXT("tactical_round_started");
    static const FString TacticalActiveUnitChanged = TEXT("tactical_active_unit_changed");
    static const FString TacticalUnitMovementConsumed = TEXT("tactical_unit_movement_consumed");
}

UTacticalTurnSyncComponent::UTacticalTurnSyncComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTacticalTurnSyncComponent::BeginPlay()
{
    Super::BeginPlay();
    OwningController = Cast<ACRPGProjectPlayerController>(GetOwner());
}

void UTacticalTurnSyncComponent::HandleGameEvent(const FString &EventName, const FString &Payload)
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller)
    {
        return;
    }

    // Turn sync reacts only to the EventBus events that can invalidate possession, preview state or HUD data.
    if (EventName == TacticalTurnSyncEvents::TacticalTurnStarted || EventName == TacticalTurnSyncEvents::TacticalTurnEnded || EventName == TacticalTurnSyncEvents::TacticalEncounterStarted || EventName == TacticalTurnSyncEvents::TacticalRoundStarted || EventName == TacticalTurnSyncEvents::TacticalActiveUnitChanged || EventName == TacticalTurnSyncEvents::TacticalUnitMovementConsumed)
    {
        if (EventName == TacticalTurnSyncEvents::TacticalTurnStarted)
        {
            if (!ExplorationPawnBeforeTacticalTurn.IsValid())
            {
                ExplorationPawnBeforeTacticalTurn = Controller->GetPawn();
            }

            SyncPossessionToActiveTacticalUnit();
        }

        if (EventName == TacticalTurnSyncEvents::TacticalTurnStarted || EventName == TacticalTurnSyncEvents::TacticalActiveUnitChanged)
        {
            SyncPossessionToActiveTacticalUnit();

            const UTacticalTurnSubsystem *TacticalTurnSubsystem = Controller->GetTacticalTurnSubsystem();
            const ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
            const UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit ? ActiveUnit->GetTacticalUnitComponent() : nullptr;
            bTurnModeMovementEnabled = TacticalUnitComponent && (Controller->bAllowControllingNonPlayerTacticalUnits || TacticalUnitComponent->IsPlayerControlled());
        }

        if (EventName == TacticalTurnSyncEvents::TacticalTurnEnded)
        {
            RestorePossessionAfterTacticalTurn();
            bTurnModeMovementEnabled = true;
            Controller->ClearPendingTacticalMovePreviewRequest();
        }

        Controller->RefreshTacticalCombatHUD();
    }

    (void)Payload;
}

void UTacticalTurnSyncComponent::SyncPossessionToActiveTacticalUnit()
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller)
    {
        return;
    }

    UTacticalTurnSubsystem *TacticalTurnSubsystem = Controller->GetTacticalTurnSubsystem();
    ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
    UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit ? ActiveUnit->GetTacticalUnitComponent() : nullptr;
    if (!IsValid(ActiveUnit) || !TacticalUnitComponent || (!Controller->bAllowControllingNonPlayerTacticalUnits && !TacticalUnitComponent->IsPlayerControlled()) || Controller->GetPawn() == ActiveUnit)
    {
        return;
    }

    // Possession swaps must clear hover state and traversal because they belong to the previously controlled pawn.
    Controller->ClearPendingTacticalMovePreviewRequest();
    if (UTacticalMovementControllerComponent *MovementController = Controller->GetTacticalMovementControllerComponent())
    {
        MovementController->ClearTacticalPathTraversal(true);
    }

    Controller->StopMovement();
    Controller->SetControlledPawnTacticalInputSuppressed(true);
    Controller->Possess(ActiveUnit);
    Controller->SetControlledPawnTacticalInputSuppressed(true);
}

void UTacticalTurnSyncComponent::RestorePossessionAfterTacticalTurn()
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller)
    {
        return;
    }

    APawn *ExplorationPawn = ExplorationPawnBeforeTacticalTurn.Get();
    ExplorationPawnBeforeTacticalTurn.Reset();

    if (!IsValid(ExplorationPawn) || Controller->GetPawn() == ExplorationPawn)
    {
        return;
    }

    Controller->ClearPendingTacticalMovePreviewRequest();
    if (UTacticalMovementControllerComponent *MovementController = Controller->GetTacticalMovementControllerComponent())
    {
        MovementController->ClearTacticalPathTraversal(true);
    }

    Controller->StopMovement();
    Controller->Possess(ExplorationPawn);
    Controller->SetControlledPawnTacticalInputSuppressed(false);
}

bool UTacticalTurnSyncComponent::IsTurnModeMovementEnabled() const
{
    return bTurnModeMovementEnabled;
}

void UTacticalTurnSyncComponent::SetTurnModeMovementEnabled(bool bEnabled)
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller || bTurnModeMovementEnabled == bEnabled)
    {
        return;
    }

    bTurnModeMovementEnabled = bEnabled;

    if (!bTurnModeMovementEnabled)
    {
        Controller->ClearPendingTacticalMovePreviewRequest();
    }
    else
    {
        Controller->RefreshTacticalCombatHUD();
    }
}

void UTacticalTurnSyncComponent::ToggleTurnModeMovementEnabled()
{
    SetTurnModeMovementEnabled(!bTurnModeMovementEnabled);
}

ACRPGProjectPlayerController *UTacticalTurnSyncComponent::GetOwnerController() const
{
    return OwningController.Get();
}
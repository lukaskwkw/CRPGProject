#include "TacticalCombatHUDWidget.h"

#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

void UTacticalCombatHUDWidget::OnToggleTurnModePressed()
{
    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        TacticalTurnSubsystem->ToggleTurnMode();
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnEndTurnPressed()
{
    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        if (TacticalTurnSubsystem->IsTurnModeActive())
        {
            TacticalTurnSubsystem->AdvanceRound();
        }
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::RefreshFromSubsystem()
{
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

    float AvailableMovementForPlanningCm = 0.0f;

    // Pull authoritative state from both the turn subsystem and the player controller every refresh.
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
                RemainingMovementRangeCm = TacticalUnitComponent->GetRemainingMovementRange();
                AvailableMovementForPlanningCm = RemainingMovementRangeCm;
                ActionPoints = TacticalUnitComponent->GetActionPoints();
                BonusActionPoints = TacticalUnitComponent->GetBonusActionPoints();
            }
        }
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
    }

    OnTacticalCombatDataRefreshed();
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
        PlayerController->ToggleTurnModeMovementEnabled();
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::SetMovementEnabled(bool bEnabled)
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        PlayerController->SetTurnModeMovementEnabled(bEnabled);
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnCancelMovePressed()
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningCRPGPlayerController())
    {
        PlayerController->ClearPendingTacticalMovePreviewRequest();
    }

    RefreshFromSubsystem();
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

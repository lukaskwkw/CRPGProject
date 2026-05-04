#include "TacticalCombatHUDWidget.h"

#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

void UTacticalCombatHUDWidget::OnToggleTurnModePressed()
{
    if (UTacticalTurnSubsystem* TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        TacticalTurnSubsystem->ToggleTurnMode();
    }

    RefreshFromSubsystem();
}

void UTacticalCombatHUDWidget::OnEndTurnPressed()
{
    if (UTacticalTurnSubsystem* TacticalTurnSubsystem = GetTacticalTurnSubsystem())
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
    ActionPoints = 0;
    BonusActionPoints = 0;
    CurrentTurnState = ETacticalTurnState::Disabled;
    TurnModeLabel = TEXT("OFF");

    if (UTacticalTurnSubsystem* TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        bTurnModeEnabled = TacticalTurnSubsystem->IsTurnModeActive();
        RoundNumber = TacticalTurnSubsystem->GetCurrentRound();
        CurrentTurnState = TacticalTurnSubsystem->GetCurrentState();
        TurnModeLabel = bTurnModeEnabled ? TEXT("ON") : TEXT("OFF");

        if (ACRPGBaseCharacter* ActiveUnit = TacticalTurnSubsystem->GetActiveUnit())
        {
            if (UTacticalUnitComponent* TacticalUnitComponent = ActiveUnit->GetTacticalUnitComponent())
            {
                RemainingMovementRangeCm = TacticalUnitComponent->GetRemainingMovementRange();
                ActionPoints = TacticalUnitComponent->GetActionPoints();
                BonusActionPoints = TacticalUnitComponent->GetBonusActionPoints();
            }
        }
    }

    OnTacticalCombatDataRefreshed();
}

UTacticalTurnSubsystem* UTacticalCombatHUDWidget::GetTacticalTurnSubsystem() const
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        return GameInstance->GetSubsystem<UTacticalTurnSubsystem>();
    }

    return nullptr;
}

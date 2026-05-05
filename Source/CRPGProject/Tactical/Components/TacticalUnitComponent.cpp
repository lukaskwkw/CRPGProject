#include "TacticalUnitComponent.h"

UTacticalUnitComponent::UTacticalUnitComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTacticalUnitComponent::ResetForNewRound()
{
    ResetEncounterTurnState();
    RemainingMovementRangeCm = MaxMovementRangeCm;
    ActionPoints = 1;
    BonusActionPoints = 1;
    bTurnConsumed = false;
}

bool UTacticalUnitComponent::IsAlive() const
{
    return bIsAlive;
}

bool UTacticalUnitComponent::IsPlayerControlled() const
{
    return bIsPlayerControlled;
}

bool UTacticalUnitComponent::IsEnemyTo(const UTacticalUnitComponent *OtherUnit) const
{
    return OtherUnit != nullptr && OtherUnit != this && TeamId != OtherUnit->TeamId;
}

void UTacticalUnitComponent::MarkDead()
{
    bIsAlive = false;
    bTurnCompleted = true;
}

void UTacticalUnitComponent::SetInitiative(int32 InValue)
{
    CurrentInitiative = InValue;
}

int32 UTacticalUnitComponent::GetCurrentInitiative() const
{
    return CurrentInitiative;
}

void UTacticalUnitComponent::ResetEncounterTurnState()
{
    bTurnCompleted = !bIsAlive;
}

bool UTacticalUnitComponent::HasCompletedEncounterTurn() const
{
    return bTurnCompleted;
}

void UTacticalUnitComponent::SetTurnCompleted(bool bCompleted)
{
    bTurnCompleted = bCompleted || !bIsAlive;
}

void UTacticalUnitComponent::ConsumeMovement(float Amount)
{
    RemainingMovementRangeCm = FMath::Max(0.0f, RemainingMovementRangeCm - FMath::Max(0.0f, Amount));
    bTurnConsumed = RemainingMovementRangeCm <= KINDA_SMALL_NUMBER;
}

bool UTacticalUnitComponent::HasMovementBudget(float RequiredDistance) const
{
    return RemainingMovementRangeCm + KINDA_SMALL_NUMBER >= FMath::Max(0.0f, RequiredDistance);
}

float UTacticalUnitComponent::GetRemainingMovementRange() const
{
    return RemainingMovementRangeCm;
}

int32 UTacticalUnitComponent::GetActionPoints() const
{
    return ActionPoints;
}

int32 UTacticalUnitComponent::GetBonusActionPoints() const
{
    return BonusActionPoints;
}

bool UTacticalUnitComponent::HasTurnConsumed() const
{
    return bTurnConsumed;
}

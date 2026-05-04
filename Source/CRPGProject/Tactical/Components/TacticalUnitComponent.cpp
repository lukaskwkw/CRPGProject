#include "TacticalUnitComponent.h"

UTacticalUnitComponent::UTacticalUnitComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTacticalUnitComponent::ResetForNewRound()
{
    RemainingMovementRangeCm = MaxMovementRangeCm;
    ActionPoints = 1;
    BonusActionPoints = 1;
    bTurnConsumed = false;
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

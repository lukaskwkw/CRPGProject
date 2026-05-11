#include "TacticalUnitComponent.h"

#include "Engine/GameInstance.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

UTacticalUnitComponent::UTacticalUnitComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UTacticalUnitComponent::IsOccupyingTacticalSpace() const
{
    // A unit stops blocking tactical space when dead, explicitly disabled, or temporarily suppressed during traversal.
    return bIsAlive && bBlocksTacticalMovement && !bOccupancySuppressed;
}

FVector UTacticalUnitComponent::GetOccupiedLocation() const
{
    if (const AActor *OwnerActor = GetOwner())
    {
        return OwnerActor->GetActorLocation();
    }

    return FVector::ZeroVector;
}

float UTacticalUnitComponent::GetOccupancyRadiusCm() const
{
    return FMath::Max(0.0f, OccupancyRadiusCm);
}

float UTacticalUnitComponent::GetOccupancyHalfHeightCm() const
{
    return FMath::Max(0.0f, OccupancyHalfHeightCm);
}

float UTacticalUnitComponent::GetNavigationBlockerRadiusCm() const
{
    return FMath::Max(1.0f, GetOccupancyRadiusCm() + NavigationBlockerRadiusOffsetCm);
}

float UTacticalUnitComponent::GetNavigationBlockerHalfHeightCm() const
{
    return FMath::Max(1.0f, GetOccupancyHalfHeightCm() + NavigationBlockerHalfHeightOffsetCm);
}

void UTacticalUnitComponent::SetOccupancySuppressed(bool bSuppressed)
{
    if (bOccupancySuppressed == bSuppressed)
    {
        return;
    }

    bOccupancySuppressed = bSuppressed;

    // The owning character mirrors this logical state into its navigation blocker component.
    if (ACRPGBaseCharacter *OwningCharacter = Cast<ACRPGBaseCharacter>(GetOwner()))
    {
        const UTacticalTurnSubsystem *TacticalTurnSubsystem = OwningCharacter->GetGameInstance()
                                                                  ? OwningCharacter->GetGameInstance()->GetSubsystem<UTacticalTurnSubsystem>()
                                                                  : nullptr;
        OwningCharacter->UpdateTacticalOccupancyNavigationBlocker(TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr);
    }
}

void UTacticalUnitComponent::ResetForNewRound()
{
    // Round reset restores tactical resources but deliberately does not change occupancy sizing.
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

FString UTacticalUnitComponent::GetDisplayName() const
{
    return DisplayName;
}

UTexture2D *UTacticalUnitComponent::GetPortraitTexture() const
{
    return PortraitTexture;
}

bool UTacticalUnitComponent::IsEnemyTo(const UTacticalUnitComponent *OtherUnit) const
{
    return OtherUnit != nullptr && OtherUnit != this && TeamId != OtherUnit->TeamId;
}

void UTacticalUnitComponent::MarkDead()
{
    bIsAlive = false;
    bTurnCompleted = true;

    // Death immediately removes the unit's navigation footprint so future previews can route through it.
    if (ACRPGBaseCharacter *OwningCharacter = Cast<ACRPGBaseCharacter>(GetOwner()))
    {
        const UTacticalTurnSubsystem *TacticalTurnSubsystem = OwningCharacter->GetGameInstance()
                                                                  ? OwningCharacter->GetGameInstance()->GetSubsystem<UTacticalTurnSubsystem>()
                                                                  : nullptr;
        OwningCharacter->UpdateTacticalOccupancyNavigationBlocker(TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr);
    }
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

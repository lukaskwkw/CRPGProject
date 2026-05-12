#include "TacticalUnitComponent.h"

#include "Engine/GameInstance.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

namespace TacticalUnitEvents
{
    static const FString TacticalUnitDied = TEXT("tactical_unit_died");
}

UTacticalUnitComponent::UTacticalUnitComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTacticalUnitComponent::BeginPlay()
{
    Super::BeginPlay();

    MaxHP = FMath::Max(1, MaxHP);
    CurrentHP = bIsAlive
                    ? FMath::Clamp(CurrentHP > 0 ? CurrentHP : MaxHP, 0, MaxHP)
                    : 0;
    bIsAlive = CurrentHP > 0;
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

bool UTacticalUnitComponent::IsDead() const
{
    return !bIsAlive;
}

void UTacticalUnitComponent::ApplyDamage(int32 DamageAmount)
{
    if (!bIsAlive)
    {
        return;
    }

    // Combat resolver already decided whether the hit landed and how much damage it dealt.
    // The component is the owner of HP state and death transition, so it only clamps and reacts here.
    CurrentHP = FMath::Clamp(CurrentHP - FMath::Max(0, DamageAmount), 0, MaxHP);
    if (CurrentHP <= 0)
    {
        MarkDead();
    }
}

int32 UTacticalUnitComponent::GetCurrentHP() const
{
    return CurrentHP;
}

int32 UTacticalUnitComponent::GetMaxHP() const
{
    return MaxHP;
}

float UTacticalUnitComponent::GetHealthFraction() const
{
    return MaxHP > 0 ? static_cast<float>(CurrentHP) / static_cast<float>(MaxHP) : 0.0f;
}

int32 UTacticalUnitComponent::GetArmorClass() const
{
    return ArmorClass;
}

int32 UTacticalUnitComponent::GetMeleeAttackBonus() const
{
    return MeleeAttackBonus;
}

int32 UTacticalUnitComponent::GetRangedAttackBonus() const
{
    return RangedAttackBonus;
}

int32 UTacticalUnitComponent::GetMeleeDamageMin() const
{
    return MeleeDamageMin;
}

int32 UTacticalUnitComponent::GetMeleeDamageMax() const
{
    return MeleeDamageMax;
}

int32 UTacticalUnitComponent::GetRangedDamageMin() const
{
    return RangedDamageMin;
}

int32 UTacticalUnitComponent::GetRangedDamageMax() const
{
    return RangedDamageMax;
}

float UTacticalUnitComponent::GetMeleeRangeCm() const
{
    return FMath::Max(0.0f, MeleeRangeCm);
}

float UTacticalUnitComponent::GetRangedRangeCm() const
{
    return FMath::Max(0.0f, RangedRangeCm);
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
    if (!bIsAlive)
    {
        return;
    }

    bIsAlive = false;
    bTurnCompleted = true;
    bTurnConsumed = true;
    CurrentHP = 0;
    RemainingMovementRangeCm = 0.0f;
    ActionPoints = 0;
    BonusActionPoints = 0;

    // Death immediately removes the unit's navigation footprint so future previews can route through it,
    // while the owning character handles the physical presentation transition such as ragdoll.
    if (ACRPGBaseCharacter *OwningCharacter = Cast<ACRPGBaseCharacter>(GetOwner()))
    {
        OwningCharacter->EnterTacticalDeathState();

        const UTacticalTurnSubsystem *TacticalTurnSubsystem = OwningCharacter->GetGameInstance()
                                                                  ? OwningCharacter->GetGameInstance()->GetSubsystem<UTacticalTurnSubsystem>()
                                                                  : nullptr;
        OwningCharacter->UpdateTacticalOccupancyNavigationBlocker(TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr);
    }

    if (UWorld *World = GetWorld())
    {
        if (UGameInstance *GameInstance = World->GetGameInstance())
        {
            if (UEventBusSubsystem *EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
            {
                EventBusSubsystem->PublishEvent(
                    TacticalUnitEvents::TacticalUnitDied,
                    FString::Printf(TEXT("unit=%s;display_name=%s"), *GetNameSafe(GetOwner()), *GetDisplayName()));
            }
        }
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

bool UTacticalUnitComponent::HasAnyActionPoints() const
{
    return ActionPoints > 0 || BonusActionPoints > 0;
}

bool UTacticalUnitComponent::ConsumeActionPoint()
{
    if (ActionPoints > 0)
    {
        --ActionPoints;
        return true;
    }

    if (BonusActionPoints > 0)
    {
        --BonusActionPoints;
        return true;
    }

    return false;
}

bool UTacticalUnitComponent::HasTurnConsumed() const
{
    return bTurnConsumed;
}

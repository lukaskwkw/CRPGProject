#include "TacticalUnitComponent.h"

#include "Engine/GameInstance.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

namespace TacticalUnitEvents
{
    static const FString TacticalUnitDied = TEXT("tactical_unit_died");
    static const FString TacticalUnitCombatStateChanged = TEXT("tactical_unit_combat_state_changed");
    static const FString TacticalUnitProned = TEXT("tactical_unit_proned");
}

UTacticalUnitComponent::UTacticalUnitComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTacticalUnitComponent::BeginPlay()
{
    Super::BeginPlay();

    MaxHP = FMath::Max(1, MaxHP);
    const bool bStartsDead = CombatState == ECombatUnitState::Dead || !bIsAlive;
    CurrentHP = bStartsDead
                    ? 0
                    : FMath::Clamp(CurrentHP > 0 ? CurrentHP : MaxHP, 0, MaxHP);

    if (bStartsDead || CurrentHP <= 0)
    {
        CombatState = ECombatUnitState::Dead;
        bIsAlive = false;
    }
    else if (CombatState == ECombatUnitState::Prone && !bIsPartyMember)
    {
        CombatState = ECombatUnitState::Alive;
        bIsAlive = true;
    }
    else
    {
        bIsAlive = CombatState != ECombatUnitState::Dead;
    }
}

bool UTacticalUnitComponent::IsOccupyingTacticalSpace() const
{
    // A unit stops blocking tactical space when dead, explicitly disabled, or temporarily suppressed during traversal.
    return !IsDead() && bBlocksTacticalMovement && !bOccupancySuppressed;
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
    RemainingMovementRangeCm = CanMove() ? MaxMovementRangeCm : 0.0f;
    ActionPoints = CanAct() ? 1 : 0;
    BonusActionPoints = CanAct() ? 1 : 0;
    bTurnConsumed = !CanMove();
}

bool UTacticalUnitComponent::IsProne() const
{
    return CombatState == ECombatUnitState::Prone;
}

bool UTacticalUnitComponent::IsDead() const
{
    return CombatState == ECombatUnitState::Dead;
}

ECombatUnitState UTacticalUnitComponent::GetCombatState() const
{
    return CombatState;
}

void UTacticalUnitComponent::SetCombatState(ECombatUnitState NewState)
{
    if (CombatState == NewState)
    {
        return;
    }

    const ECombatUnitState PreviousState = CombatState;
    CombatState = NewState;
    bIsAlive = CombatState != ECombatUnitState::Dead;

    switch (CombatState)
    {
    case ECombatUnitState::Alive:
        if (CurrentHP <= 0)
        {
            CurrentHP = 1;
        }
        break;
    case ECombatUnitState::Prone:
        bIsAlive = true;
        RemainingMovementRangeCm = 0.0f;
        ActionPoints = 0;
        BonusActionPoints = 0;
        bTurnConsumed = true;
        bTurnCompleted = true;
        break;
    case ECombatUnitState::Dead:
        bIsAlive = false;
        CurrentHP = 0;
        RemainingMovementRangeCm = 0.0f;
        ActionPoints = 0;
        BonusActionPoints = 0;
        bTurnConsumed = true;
        bTurnCompleted = true;
        break;
    default:
        break;
    }

    RefreshOwnerStatePresentation(PreviousState, CombatState);
    PublishCombatStateChangedEvent(PreviousState, CombatState);
}

void UTacticalUnitComponent::ApplyDamage(int32 DamageAmount)
{
    if (IsDead())
    {
        return;
    }

    // Combat resolver already decided whether the hit landed and how much damage it dealt.
    // The component is the owner of HP state and death transition, so it only clamps and reacts here.
    CurrentHP = FMath::Clamp(CurrentHP - FMath::Max(0, DamageAmount), 0, MaxHP);
    if (CurrentHP <= 0)
    {
        if (CombatState == ECombatUnitState::Alive && CanEnterProneState())
        {
            SetCombatState(ECombatUnitState::Prone);
        }
        else
        {
            MarkDead();
        }
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
    return CombatState != ECombatUnitState::Dead;
}

bool UTacticalUnitComponent::CanMove() const
{
    return CombatState == ECombatUnitState::Alive;
}

bool UTacticalUnitComponent::CanAct() const
{
    return CombatState == ECombatUnitState::Alive;
}

bool UTacticalUnitComponent::CanEnterProneState() const
{
    return CombatState == ECombatUnitState::Alive && bIsPartyMember;
}

void UTacticalUnitComponent::RecoverFromProne()
{
    if (!IsProne())
    {
        return;
    }

    CurrentHP = FMath::Max(CurrentHP, 1);
    RemainingMovementRangeCm = MaxMovementRangeCm;
    ActionPoints = 1;
    BonusActionPoints = 1;
    bTurnConsumed = false;
    bTurnCompleted = false;
    SetCombatState(ECombatUnitState::Alive);
}

bool UTacticalUnitComponent::IsPlayerControlled() const
{
    return bIsPlayerControlled;
}

bool UTacticalUnitComponent::IsPartyMember() const
{
    return bIsPartyMember;
}

bool UTacticalUnitComponent::IsNeutral() const
{
    return bIsNeutral;
}

int32 UTacticalUnitComponent::GetTeamId() const
{
    return TeamId;
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
    if (IsDead())
    {
        return;
    }

    SetCombatState(ECombatUnitState::Dead);
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
    bTurnCompleted = !CanAct();
}

bool UTacticalUnitComponent::HasCompletedEncounterTurn() const
{
    return bTurnCompleted;
}

void UTacticalUnitComponent::SetTurnCompleted(bool bCompleted)
{
    bTurnCompleted = bCompleted || !CanAct();
}

void UTacticalUnitComponent::ConsumeMovement(float Amount)
{
    if (!CanMove())
    {
        return;
    }

    RemainingMovementRangeCm = FMath::Max(0.0f, RemainingMovementRangeCm - FMath::Max(0.0f, Amount));
    bTurnConsumed = RemainingMovementRangeCm <= KINDA_SMALL_NUMBER;
}

bool UTacticalUnitComponent::HasMovementBudget(float RequiredDistance) const
{
    return CanMove() && RemainingMovementRangeCm + KINDA_SMALL_NUMBER >= FMath::Max(0.0f, RequiredDistance);
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
    return CanAct() && (ActionPoints > 0 || BonusActionPoints > 0);
}

bool UTacticalUnitComponent::ConsumeActionPoint()
{
    if (!CanAct())
    {
        return false;
    }

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

void UTacticalUnitComponent::PublishCombatStateChangedEvent(ECombatUnitState PreviousState, ECombatUnitState NewState) const
{
    UWorld *World = GetWorld();
    UGameInstance *GameInstance = World ? World->GetGameInstance() : nullptr;
    UEventBusSubsystem *EventBusSubsystem = GameInstance ? GameInstance->GetSubsystem<UEventBusSubsystem>() : nullptr;
    if (!EventBusSubsystem)
    {
        return;
    }

    EventBusSubsystem->PublishEvent(
        TacticalUnitEvents::TacticalUnitCombatStateChanged,
        FString::Printf(
            TEXT("unit=%s;display_name=%s;previous_state=%d;new_state=%d"),
            *GetNameSafe(GetOwner()),
            *GetDisplayName(),
            static_cast<int32>(PreviousState),
            static_cast<int32>(NewState)));

    if (NewState == ECombatUnitState::Prone)
    {
        EventBusSubsystem->PublishEvent(
            TacticalUnitEvents::TacticalUnitProned,
            FString::Printf(TEXT("unit=%s;display_name=%s"), *GetNameSafe(GetOwner()), *GetDisplayName()));
    }

    if (NewState == ECombatUnitState::Dead)
    {
        EventBusSubsystem->PublishEvent(
            TacticalUnitEvents::TacticalUnitDied,
            FString::Printf(TEXT("unit=%s;display_name=%s"), *GetNameSafe(GetOwner()), *GetDisplayName()));
    }
}

void UTacticalUnitComponent::RefreshOwnerStatePresentation(ECombatUnitState PreviousState, ECombatUnitState NewState)
{
    ACRPGBaseCharacter *OwningCharacter = Cast<ACRPGBaseCharacter>(GetOwner());
    if (!OwningCharacter)
    {
        return;
    }

    switch (NewState)
    {
    case ECombatUnitState::Alive:
        if (PreviousState == ECombatUnitState::Prone)
        {
            OwningCharacter->RecoverFromPronePresentation();
        }
        break;
    case ECombatUnitState::Prone:
        OwningCharacter->EnterTacticalProneState();
        break;
    case ECombatUnitState::Dead:
        OwningCharacter->EnterTacticalDeathState();
        break;
    default:
        break;
    }

    const UTacticalTurnSubsystem *TacticalTurnSubsystem = OwningCharacter->GetGameInstance()
                                                              ? OwningCharacter->GetGameInstance()->GetSubsystem<UTacticalTurnSubsystem>()
                                                              : nullptr;
    OwningCharacter->UpdateTacticalOccupancyNavigationBlocker(TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr);
}

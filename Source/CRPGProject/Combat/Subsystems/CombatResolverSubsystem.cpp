#include "Combat/Subsystems/CombatResolverSubsystem.h"

#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Components/TacticalUnitComponent.h"

namespace CombatResolverEvents
{
    static const FString AttackResolved = TEXT("attack_resolved");
    static const FString UnitDamaged = TEXT("unit_damaged");
    static const FString UnitKilled = TEXT("unit_killed");
}

namespace
{
    int32 RollAttackDamage(const UTacticalUnitComponent *Attacker, bool bIsMeleeAttack)
    {
        if (!Attacker)
        {
            return 0;
        }

        const int32 DamageMin = bIsMeleeAttack ? Attacker->GetMeleeDamageMin() : Attacker->GetRangedDamageMin();
        const int32 DamageMax = bIsMeleeAttack ? Attacker->GetMeleeDamageMax() : Attacker->GetRangedDamageMax();
        return FMath::RandRange(FMath::Min(DamageMin, DamageMax), FMath::Max(DamageMin, DamageMax));
    }
}

FCombatAttackResult UCombatResolverSubsystem::ResolveMeleeAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender)
{
    return ResolveAttack(Attacker, Defender, ECombatActionType::MeleeAttack, true);
}

FCombatAttackResult UCombatResolverSubsystem::ResolveRangedAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender)
{
    return ResolveAttack(Attacker, Defender, ECombatActionType::RangedAttack, true);
}

FCombatAttackResult UCombatResolverSubsystem::ResolveAttackForExecution(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType)
{
    return ResolveAttack(Attacker, Defender, ActionType, false);
}

void UCombatResolverSubsystem::CommitResolvedAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType, const FCombatAttackResult &Result)
{
    if (!IsValid(Attacker) || !IsValid(Defender) || Attacker == Defender)
    {
        return;
    }

    const bool bIsMeleeAttack = ActionType == ECombatActionType::MeleeAttack;

    if (Result.bHit)
    {
        Defender->ApplyDamage(Result.DamageRoll);

        const bool bKilledTarget = Defender->IsDead();
        PublishEvent(
            CombatResolverEvents::UnitDamaged,
            FString::Printf(
                TEXT("target=%s;damage=%d;current_hp=%d;max_hp=%d"),
                *GetNameSafe(Defender->GetOwner()),
                Result.DamageRoll,
                Defender->GetCurrentHP(),
                Defender->GetMaxHP()));

        if (bKilledTarget)
        {
            PublishEvent(
                CombatResolverEvents::UnitKilled,
                FString::Printf(
                    TEXT("target=%s;attacker=%s;action=%s"),
                    *GetNameSafe(Defender->GetOwner()),
                    *GetNameSafe(Attacker->GetOwner()),
                    bIsMeleeAttack ? TEXT("melee") : TEXT("ranged")));
        }
    }

    if (ACRPGBaseCharacter *DefenderCharacter = Cast<ACRPGBaseCharacter>(Defender->GetOwner()))
    {
        DefenderCharacter->ShowCombatAttackResult(Result);
    }

    PublishEvent(
        CombatResolverEvents::AttackResolved,
        FString::Printf(
            TEXT("attacker=%s;defender=%s;action=%s;hit=%s;critical=%s;attack_roll=%d;natural_roll=%d;damage_roll=%d;killed=%s"),
            *GetNameSafe(Attacker->GetOwner()),
            *GetNameSafe(Defender->GetOwner()),
            bIsMeleeAttack ? TEXT("melee") : TEXT("ranged"),
            Result.bHit ? TEXT("true") : TEXT("false"),
            Result.bCriticalHit ? TEXT("true") : TEXT("false"),
            Result.AttackRoll,
            Result.NaturalRoll,
            Result.DamageRoll,
            Defender->IsDead() ? TEXT("true") : TEXT("false")));
}

FCombatAttackResult UCombatResolverSubsystem::ResolveAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType, bool bCommitImmediately)
{
    FCombatAttackResult Result;

    if (!IsValid(Attacker) || !IsValid(Defender) || Attacker == Defender || !Attacker->IsAlive() || !Defender->IsAlive())
    {
        return Result;
    }

    if (!Attacker->ConsumeActionPoint())
    {
        return Result;
    }

    const int32 RawD20Roll = FMath::RandRange(1, 20);
    const bool bIsMeleeAttack = ActionType == ECombatActionType::MeleeAttack;
    const int32 AttackBonus = bIsMeleeAttack ? Attacker->GetMeleeAttackBonus() : Attacker->GetRangedAttackBonus();
    Result.NaturalRoll = RawD20Roll;
    Result.bCriticalHit = RawD20Roll == 20;
    Result.AttackRoll = RawD20Roll + AttackBonus;
    Result.bHit = Result.AttackRoll >= Defender->GetArmorClass();

    if (Result.bHit)
    {
        Result.DamageRoll = RollAttackDamage(Attacker, bIsMeleeAttack);

        // Prototype crit rule: a natural 20 rolls the same damage packet a second time and adds it on top.
        // This keeps the result swingy and readable without introducing separate crit-only modifiers yet.
        if (Result.bCriticalHit)
        {
            Result.DamageRoll += RollAttackDamage(Attacker, bIsMeleeAttack);
        }
    }

    Result.bKilledTarget = Result.bHit && Result.DamageRoll >= Defender->GetCurrentHP();

    if (bCommitImmediately)
    {
        CommitResolvedAttack(Attacker, Defender, ActionType, Result);
    }

    return Result;
}

void UCombatResolverSubsystem::PublishEvent(const FString &EventName, const FString &Payload) const
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UEventBusSubsystem *EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
        {
            EventBusSubsystem->PublishEvent(EventName, Payload);
        }
    }
}
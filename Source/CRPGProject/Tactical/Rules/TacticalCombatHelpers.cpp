#include "Tactical/Rules/TacticalCombatHelpers.h"

#include "Tactical/Components/TacticalUnitComponent.h"

namespace TacticalCombatHelpers
{
    bool IsValidCombatTarget(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender)
    {
        return Attacker && Defender && Attacker != Defender && Attacker->CanAct() && Defender->IsAlive() && Attacker->IsEnemyTo(Defender);
    }

    float GetCombatGapToTargetCm(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender)
    {
        if (!IsValidCombatTarget(Attacker, Defender))
        {
            return TNumericLimits<float>::Max();
        }

        const float CenterDistanceCm = FVector::Dist2D(Attacker->GetOccupiedLocation(), Defender->GetOccupiedLocation());
        return FMath::Max(0.0f, CenterDistanceCm - Attacker->GetOccupancyRadiusCm() - Defender->GetOccupancyRadiusCm());
    }

    bool IsTargetInCombatRange(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender, ECombatActionType ActionType, float RangeToleranceCm)
    {
        if (!IsValidCombatTarget(Attacker, Defender))
        {
            return false;
        }

        const float RequiredRangeCm = ActionType == ECombatActionType::MeleeAttack ? Attacker->GetMeleeRangeCm() : Attacker->GetRangedRangeCm();
        return GetCombatGapToTargetCm(Attacker, Defender) <= RequiredRangeCm + FMath::Max(0.0f, RangeToleranceCm);
    }

    ECombatActionType GetPreferredCombatActionForStyle(ECombatStyleCategory StyleCategory)
    {
        switch (StyleCategory)
        {
        case ECombatStyleCategory::Bow:
            return ECombatActionType::RangedAttack;
        case ECombatStyleCategory::Unarmed:
        case ECombatStyleCategory::Shield:
        case ECombatStyleCategory::TwoWeapons:
        case ECombatStyleCategory::TwoHanded:
        default:
            return ECombatActionType::MeleeAttack;
        }
    }

    bool TryGetApproachDestinationInRange(
        const UTacticalUnitComponent *Attacker,
        const UTacticalUnitComponent *Defender,
        ECombatActionType ActionType,
        float ApproachBufferCm,
        FVector &OutDestination)
    {
        if (!IsValidCombatTarget(Attacker, Defender))
        {
            return false;
        }

        const FVector AttackerLocation = Attacker->GetOccupiedLocation();
        const FVector TargetLocation = Defender->GetOccupiedLocation();
        FVector ApproachDirection = AttackerLocation - TargetLocation;
        ApproachDirection.Z = 0.0f;
        if (!ApproachDirection.Normalize())
        {
            return false;
        }

        const float AttackRangeCm = ActionType == ECombatActionType::MeleeAttack ? Attacker->GetMeleeRangeCm() : Attacker->GetRangedRangeCm();
        const float DesiredCenterDistanceCm = FMath::Max(
            0.0f,
            AttackRangeCm + Attacker->GetOccupancyRadiusCm() + Defender->GetOccupancyRadiusCm() - FMath::Max(0.0f, ApproachBufferCm));
        OutDestination = TargetLocation + (ApproachDirection * DesiredCenterDistanceCm);
        return true;
    }
}
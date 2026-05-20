#pragma once

#include "CoreMinimal.h"
#include "Combat/Types/CombatTypes.h"
#include "Combat/Types/EquipmentTypes.h"

class UTacticalUnitComponent;

namespace TacticalCombatHelpers
{
    bool IsValidCombatTarget(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender);
    float GetCombatGapToTargetCm(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender);
    bool IsTargetInCombatRange(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender, ECombatActionType ActionType, float RangeToleranceCm = 0.0f);
    ECombatActionType GetPreferredCombatActionForStyle(ECombatStyleCategory StyleCategory);
    bool TryGetApproachDestinationInRange(
        const UTacticalUnitComponent *Attacker,
        const UTacticalUnitComponent *Defender,
        ECombatActionType ActionType,
        float ApproachBufferCm,
        FVector &OutDestination);
}
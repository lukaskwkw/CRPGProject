#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

UENUM(BlueprintType)
enum class ECombatActionType : uint8
{
    None UMETA(DisplayName = "None"),
    MeleeAttack UMETA(DisplayName = "Melee Attack"),
    RangedAttack UMETA(DisplayName = "Ranged Attack")
};

UENUM(BlueprintType)
enum class ECombatTargetingMode : uint8
{
    None UMETA(DisplayName = "None"),
    EnemyUnit UMETA(DisplayName = "Enemy Unit")
};

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FCombatAttackResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bHit = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 AttackRoll = 0;

    // Natural d20 result before bonuses. Useful for UI feedback such as crit banners without recomputing the roll.
    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 NaturalRoll = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bCriticalHit = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    int32 DamageRoll = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bKilledTarget = false;
};
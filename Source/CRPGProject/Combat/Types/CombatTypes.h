#pragma once

#include "CoreMinimal.h"
#include "CombatTypes.generated.h"

class UTacticalUnitComponent;

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

UENUM(BlueprintType)
enum class ECombatUnitState : uint8
{
    Alive UMETA(DisplayName = "Alive"),
    Prone UMETA(DisplayName = "Prone"),
    Dead UMETA(DisplayName = "Dead")
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

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FPendingCombatAttack
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UTacticalUnitComponent> Attacker = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    TObjectPtr<UTacticalUnitComponent> Defender = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    ECombatActionType ActionType = ECombatActionType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    FCombatAttackResult ResolvedResult;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bHasResolvedResult = false;

    UPROPERTY(BlueprintReadOnly, Category = "Combat")
    bool bTriggeredAfterTraversal = false;

    bool IsValid() const
    {
        return Attacker != nullptr && Defender != nullptr && ActionType != ECombatActionType::None;
    }
};
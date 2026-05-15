#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Types/CombatTypes.h"
#include "CombatExecutionSubsystem.generated.h"

class UTacticalUnitComponent;

UCLASS()
class CRPGPROJECT_API UCombatExecutionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool RequestAttackExecution(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType, bool bTriggeredAfterTraversal = false);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool QueuePendingAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType, bool bTriggeredAfterTraversal = false);

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool HasPendingAttackForAttacker(const UTacticalUnitComponent *Attacker) const;

    UFUNCTION(BlueprintPure, Category = "Combat")
    bool TryGetPendingAttackForAttacker(const UTacticalUnitComponent *Attacker, FPendingCombatAttack &OutPendingAttack) const;

    bool MarkPendingAttackResolved(UTacticalUnitComponent *Attacker, const FCombatAttackResult &ResolvedResult);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    bool ExecutePendingAttackAtHitWindow(UTacticalUnitComponent *Attacker);

    bool ConsumePendingAttack(UTacticalUnitComponent *Attacker, FPendingCombatAttack &OutPendingAttack);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    void CancelPendingAttack(UTacticalUnitComponent *Attacker);

private:
    int32 FindPendingAttackIndex(const UTacticalUnitComponent *Attacker) const;
    float PlayAttackMontage(UTacticalUnitComponent *Attacker, ECombatActionType ActionType) const;
    void PublishEvent(const FString &EventName, const FString &Payload) const;

    UPROPERTY(Transient)
    TArray<FPendingCombatAttack> PendingAttacks;
};
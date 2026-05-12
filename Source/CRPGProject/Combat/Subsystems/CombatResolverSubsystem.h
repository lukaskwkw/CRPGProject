#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Types/CombatTypes.h"
#include "CombatResolverSubsystem.generated.h"

class UTacticalUnitComponent;

UCLASS()
class CRPGPROJECT_API UCombatResolverSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // Public entry points stay thin so controller/HUD code can ask for melee or ranged resolution without duplicating
    // AP spend, d20 math, feedback events, or target-side world feedback.
    UFUNCTION(BlueprintCallable, Category = "Combat")
    FCombatAttackResult ResolveMeleeAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender);

    UFUNCTION(BlueprintCallable, Category = "Combat")
    FCombatAttackResult ResolveRangedAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender);

private:
    FCombatAttackResult ResolveAttack(UTacticalUnitComponent *Attacker, UTacticalUnitComponent *Defender, ECombatActionType ActionType);
    // EventBus remains the decoupling seam for logs, future reactions, and UI listeners that should not depend on this subsystem directly.
    void PublishEvent(const FString &EventName, const FString &Payload) const;
};
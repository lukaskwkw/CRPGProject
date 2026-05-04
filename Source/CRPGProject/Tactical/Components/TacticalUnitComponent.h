#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacticalUnitComponent.generated.h"

UCLASS(ClassGroup = (Tactical), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UTacticalUnitComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTacticalUnitComponent();

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void ResetForNewRound();

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void ConsumeMovement(float Amount);

    UFUNCTION(BlueprintPure, Category = "Tactical")
    bool HasMovementBudget(float RequiredDistance) const;

    UFUNCTION(BlueprintPure, Category = "Tactical")
    float GetRemainingMovementRange() const;

    UFUNCTION(BlueprintPure, Category = "Tactical")
    int32 GetActionPoints() const;

    UFUNCTION(BlueprintPure, Category = "Tactical")
    int32 GetBonusActionPoints() const;

    UFUNCTION(BlueprintPure, Category = "Tactical")
    bool HasTurnConsumed() const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical", meta = (ClampMin = "0.0", Units = "cm", AllowPrivateAccess = "true"))
    float MaxMovementRangeCm = 900.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical", meta = (Units = "cm", AllowPrivateAccess = "true"))
    float RemainingMovementRangeCm = 900.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical", meta = (AllowPrivateAccess = "true"))
    int32 ActionPoints = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical", meta = (AllowPrivateAccess = "true"))
    int32 BonusActionPoints = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical", meta = (AllowPrivateAccess = "true"))
    bool bTurnConsumed = false;
};

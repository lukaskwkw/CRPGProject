#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacticalUnitComponent.generated.h"

class UTexture2D;

UCLASS(ClassGroup = (Tactical), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UTacticalUnitComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTacticalUnitComponent();

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void ResetForNewRound();

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    bool IsEnemyTo(const UTacticalUnitComponent *OtherUnit) const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    void MarkDead();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    void SetInitiative(int32 InValue);

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    void ResetEncounterTurnState();

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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Identity", meta = (AllowPrivateAccess = "true"))
    FString DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Identity", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 TeamId = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Identity", meta = (AllowPrivateAccess = "true"))
    bool bIsPlayerControlled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Encounter", meta = (AllowPrivateAccess = "true"))
    bool bIsAlive = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Encounter", meta = (AllowPrivateAccess = "true"))
    int32 CurrentInitiative = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Encounter", meta = (AllowPrivateAccess = "true"))
    bool bTurnCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|UI", meta = (AllowPrivateAccess = "true"))
    UTexture2D *PortraitTexture = nullptr;

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

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

    // Logical occupancy is the gameplay-level source of truth used by preview/pathing and nav blockers.
    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    bool IsOccupyingTacticalSpace() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    FVector GetOccupiedLocation() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    float GetOccupancyRadiusCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    float GetOccupancyHalfHeightCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    float GetNavigationBlockerRadiusCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    float GetNavigationBlockerHalfHeightCm() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Occupancy")
    void SetOccupancySuppressed(bool bSuppressed);

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    void ResetForNewRound();

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    bool IsAlive() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Identity")
    bool IsPlayerControlled() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    bool IsEnemyTo(const UTacticalUnitComponent *OtherUnit) const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    void MarkDead();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    void SetInitiative(int32 InValue);

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    int32 GetCurrentInitiative() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    void ResetEncounterTurnState();

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    bool HasCompletedEncounterTurn() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    void SetTurnCompleted(bool bCompleted);

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

    // If false, the unit still exists in initiative order but does not reserve tactical space.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Occupancy", meta = (AllowPrivateAccess = "true"))
    bool bBlocksTacticalMovement = true;

    // Horizontal footprint used both for preview offsets and for the navigation blocker size.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Occupancy", meta = (ClampMin = "0.0", Units = "cm", AllowPrivateAccess = "true"))
    float OccupancyRadiusCm = 55.0f;

    // Vertical footprint used by the hidden navigation blocker component.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Occupancy", meta = (ClampMin = "0.0", Units = "cm", AllowPrivateAccess = "true"))
    float OccupancyHalfHeightCm = 110.0f;

    // Fine-tunes the nav blocker radius without changing the gameplay occupancy baseline.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Occupancy", meta = (Units = "cm", AllowPrivateAccess = "true"))
    float NavigationBlockerRadiusOffsetCm = -15.0f;

    // Fine-tunes the nav blocker half-height without changing the gameplay occupancy baseline.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Occupancy", meta = (Units = "cm", AllowPrivateAccess = "true"))
    float NavigationBlockerHalfHeightOffsetCm = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Occupancy", meta = (AllowPrivateAccess = "true"))
    bool bOccupancySuppressed = false;

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

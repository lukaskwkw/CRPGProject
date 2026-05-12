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

    virtual void BeginPlay() override;

    // Logical occupancy is the gameplay-level source of truth used by preview/pathing and nav blockers.
    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    /** Returns whether this unit should currently reserve tactical space. */
    bool IsOccupyingTacticalSpace() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    /** Returns the world-space origin used for tactical occupancy and blocker placement. */
    FVector GetOccupiedLocation() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    /** Returns the gameplay occupancy radius clamped to a non-negative value. */
    float GetOccupancyRadiusCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    /** Returns the gameplay occupancy half-height clamped to a non-negative value. */
    float GetOccupancyHalfHeightCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    /** Returns the nav-blocker radius derived from occupancy plus the configured offset. */
    float GetNavigationBlockerRadiusCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Occupancy")
    /** Returns the nav-blocker half-height derived from occupancy plus the configured offset. */
    float GetNavigationBlockerHalfHeightCm() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Occupancy")
    /** Temporarily disables occupancy while movement systems need this unit to stop blocking itself. */
    void SetOccupancySuppressed(bool bSuppressed);

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    /** Restores per-round tactical resources at the start of a new round. */
    void ResetForNewRound();

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns whether the unit has been reduced to zero HP. */
    bool IsDead() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
    /** Applies prototype combat damage and triggers death when HP reaches zero. */
    void ApplyDamage(int32 DamageAmount);

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the current hit point value used by the prototype combat layer. */
    int32 GetCurrentHP() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the maximum hit point value used by the prototype combat layer. */
    int32 GetMaxHP() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the normalized HP fraction for UI bars and tooltips. */
    float GetHealthFraction() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the armor class used by d20 hit resolution. */
    int32 GetArmorClass() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the melee attack bonus added to the d20 roll. */
    int32 GetMeleeAttackBonus() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the ranged attack bonus added to the d20 roll. */
    int32 GetRangedAttackBonus() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the minimum melee damage roll. */
    int32 GetMeleeDamageMin() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the maximum melee damage roll. */
    int32 GetMeleeDamageMax() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the minimum ranged damage roll. */
    int32 GetRangedDamageMin() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the maximum ranged damage roll. */
    int32 GetRangedDamageMax() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat", meta = (Units = "cm"))
    /** Returns the melee attack range used by controller targeting validation. */
    float GetMeleeRangeCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat", meta = (Units = "cm"))
    /** Returns the ranged attack range used by controller targeting validation. */
    float GetRangedRangeCm() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    /** Returns whether this unit can still participate in the current encounter. */
    bool IsAlive() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Identity")
    /** Returns whether the unit belongs to the player's party. */
    bool IsPlayerControlled() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Identity")
    /** Returns the localized display name shown in tactical UI. */
    FString GetDisplayName() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|UI")
    /** Returns the portrait texture used by initiative and party UI entries. */
    UTexture2D *GetPortraitTexture() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    /** Returns whether another tactical unit belongs to an opposing team. */
    bool IsEnemyTo(const UTacticalUnitComponent *OtherUnit) const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    /** Marks the unit as dead and releases its tactical occupancy immediately. */
    void MarkDead();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    /** Stores the initiative value rolled or assigned for encounter ordering. */
    void SetInitiative(int32 InValue);

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    /** Returns the current initiative score for this encounter. */
    int32 GetCurrentInitiative() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    /** Resets per-turn completion state while respecting dead units. */
    void ResetEncounterTurnState();

    UFUNCTION(BlueprintPure, Category = "Tactical|Encounter")
    /** Returns whether the unit has already spent its turn this round. */
    bool HasCompletedEncounterTurn() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Encounter")
    /** Marks the unit turn as complete or incomplete for the current round. */
    void SetTurnCompleted(bool bCompleted);

    UFUNCTION(BlueprintCallable, Category = "Tactical")
    /** Consumes movement range and marks the turn as spent when the budget reaches zero. */
    void ConsumeMovement(float Amount);

    UFUNCTION(BlueprintPure, Category = "Tactical")
    /** Returns whether the unit can afford a move of the requested distance. */
    bool HasMovementBudget(float RequiredDistance) const;

    UFUNCTION(BlueprintPure, Category = "Tactical")
    /** Returns remaining movement range in centimeters. */
    float GetRemainingMovementRange() const;

    UFUNCTION(BlueprintPure, Category = "Tactical")
    /** Returns the current primary action point count. */
    int32 GetActionPoints() const;

    UFUNCTION(BlueprintPure, Category = "Tactical")
    /** Returns the current bonus action point count. */
    int32 GetBonusActionPoints() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns whether the unit can currently spend one prototype combat action point. */
    bool HasAnyActionPoints() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Combat")
    /** Consumes one prototype combat action point, preferring the primary pool. */
    bool ConsumeActionPoint();

    UFUNCTION(BlueprintPure, Category = "Tactical")
    /** Returns whether the unit has exhausted its movement budget this turn. */
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "1", AllowPrivateAccess = "true"))
    // Prototype combat currently lives here rather than in GAS so the first tactical vertical slice stays self-contained.
    int32 MaxHP = 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 CurrentHP = 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "1", AllowPrivateAccess = "true"))
    int32 ArmorClass = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true"))
    int32 MeleeAttackBonus = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (AllowPrivateAccess = "true"))
    int32 RangedAttackBonus = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 MeleeDamageMin = 4;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 MeleeDamageMax = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 RangedDamageMin = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "0", AllowPrivateAccess = "true"))
    int32 RangedDamageMax = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "0.0", Units = "cm", AllowPrivateAccess = "true"))
    // Range is consumed only for targeting validation and approach-preview calculations.
    float MeleeRangeCm = 175.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Combat", meta = (ClampMin = "0.0", Units = "cm", AllowPrivateAccess = "true"))
    float RangedRangeCm = 1200.0f;

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

#pragma once

#include "Combat/Types/CombatTypes.h"
#include "Combat/Types/EquipmentTypes.h"
#include "Components/ActorComponent.h"
#include "CombatLoadoutComponent.generated.h"

class UAnimMontage;

UCLASS(ClassGroup = (Combat), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UCombatLoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatLoadoutComponent();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    FCombatStyleLoadout GetCurrentLoadout() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void SetCurrentLoadout(const FCombatStyleLoadout &NewLoadout);

    // One-shot setup entry point for BP/C++ callers that already know the desired style, stance, variants, and animation set.
    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void ApplyComposedLoadout(
        ECombatStyleCategory NewStyleCategory,
        const FCombatLoadoutAnimationProfile &NewAnimationProfile,
        const TArray<FCombatEquippedVariant> &NewEquippedVariants,
        ECombatStanceContext NewStanceContext = ECombatStanceContext::Exploration);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void ApplyUnarmedLoadout(
        const FCombatLoadoutAnimationProfile &NewAnimationProfile,
        ECombatStanceContext NewStanceContext = ECombatStanceContext::Exploration);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void ApplyBowLoadout(
        const FCombatEquippedVariant &BowVariant,
        const FCombatLoadoutAnimationProfile &NewAnimationProfile,
        ECombatStanceContext NewStanceContext = ECombatStanceContext::Exploration);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void ApplyTwoHandedLoadout(
        const FCombatEquippedVariant &TwoHandedWeaponVariant,
        const FCombatLoadoutAnimationProfile &NewAnimationProfile,
        ECombatStanceContext NewStanceContext = ECombatStanceContext::Exploration);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void ApplyShieldLoadout(
        const FCombatEquippedVariant &MainHandWeaponVariant,
        const FCombatEquippedVariant &ShieldVariant,
        const FCombatLoadoutAnimationProfile &NewAnimationProfile,
        ECombatStanceContext NewStanceContext = ECombatStanceContext::Exploration);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void ApplyTwoWeaponsLoadout(
        const FCombatEquippedVariant &MainHandWeaponVariant,
        const FCombatEquippedVariant &OffHandWeaponVariant,
        const FCombatLoadoutAnimationProfile &NewAnimationProfile,
        ECombatStanceContext NewStanceContext = ECombatStanceContext::Exploration);

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    ECombatStyleCategory GetCombatStyleCategory() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void SetCombatStyleCategory(ECombatStyleCategory NewStyleCategory);

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    ECombatStanceContext GetCombatStanceContext() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void SetCombatStanceContext(ECombatStanceContext NewStanceContext);

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    ECombatIdleProfile GetCurrentIdleProfile() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    ECombatLocomotionProfile GetCurrentLocomotionProfile() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool IsCombatReadyStanceActive() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool IsExplorationStanceActive() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesStyle(ECombatStyleCategory StyleCategory) const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesUnarmedStyle() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesBowStyle() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesTwoHandedStyle() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesShieldStyle() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesTwoWeaponsStyle() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesIdleProfile(ECombatIdleProfile IdleProfile) const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesLocomotionProfile(ECombatLocomotionProfile LocomotionProfile) const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesFightIdleProfile() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesNoFightIdleProfile() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesFightLocomotionProfile() const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool UsesNoFightLocomotionProfile() const;

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void SetLoadoutAnimationProfile(const FCombatLoadoutAnimationProfile &NewAnimationProfile);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void SetEquippedVariantForSlot(ECombatEquipmentSlot Slot, const FCombatEquippedVariant &NewVariant);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void SetEquippedVariants(const TArray<FCombatEquippedVariant> &NewEquippedVariants);

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    void ClearEquippedVariants();

    UFUNCTION(BlueprintCallable, Category = "Combat|Loadout")
    bool RemoveEquippedVariantForSlot(ECombatEquipmentSlot Slot);

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    bool FindEquippedVariant(ECombatEquipmentSlot Slot, FCombatEquippedVariant &OutVariant) const;

    UFUNCTION(BlueprintPure, Category = "Combat|Loadout")
    UAnimMontage *GetResolvedAttackMontage(ECombatActionType ActionType) const;

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout", meta = (AllowPrivateAccess = "true"))
    FCombatStyleLoadout CurrentLoadout;

private:
    void RefreshOwnerLoadoutVisuals() const;
    void PublishLoadoutChangedEvent() const;
    void PublishStanceChangedEvent() const;
};
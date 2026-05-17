#pragma once

#include "CoreMinimal.h"
#include "EquipmentTypes.generated.h"

class UAnimMontage;
class UStaticMesh;
class USkeletalMesh;

UENUM(BlueprintType)
enum class ECombatStyleCategory : uint8
{
    Unarmed UMETA(DisplayName = "Unarmed"),
    Bow UMETA(DisplayName = "Bow"),
    TwoHanded UMETA(DisplayName = "Two-Handed"),
    Shield UMETA(DisplayName = "Shield"),
    TwoWeapons UMETA(DisplayName = "Two Weapons")
};

UENUM(BlueprintType)
enum class ECombatStanceContext : uint8
{
    Exploration UMETA(DisplayName = "Exploration"),
    CombatReady UMETA(DisplayName = "Combat Ready")
};

UENUM(BlueprintType)
enum class ECombatIdleProfile : uint8
{
    Default UMETA(DisplayName = "Default"),
    UnarmedNoFight UMETA(DisplayName = "Unarmed No Fight"),
    UnarmedFight UMETA(DisplayName = "Unarmed Fight"),
    BowNoFight UMETA(DisplayName = "Bow No Fight"),
    BowFight UMETA(DisplayName = "Bow Fight"),
    TwoHandedNoFight UMETA(DisplayName = "Two-Handed No Fight"),
    TwoHandedFight UMETA(DisplayName = "Two-Handed Fight"),
    ShieldNoFight UMETA(DisplayName = "Shield No Fight"),
    ShieldFight UMETA(DisplayName = "Shield Fight"),
    TwoWeaponsNoFight UMETA(DisplayName = "Two Weapons No Fight"),
    TwoWeaponsFight UMETA(DisplayName = "Two Weapons Fight")
};

UENUM(BlueprintType)
enum class ECombatLocomotionProfile : uint8
{
    Default UMETA(DisplayName = "Default"),
    UnarmedNoFight UMETA(DisplayName = "Unarmed No Fight"),
    UnarmedFight UMETA(DisplayName = "Unarmed Fight"),
    BowNoFight UMETA(DisplayName = "Bow No Fight"),
    BowFight UMETA(DisplayName = "Bow Fight"),
    TwoHandedNoFight UMETA(DisplayName = "Two-Handed No Fight"),
    TwoHandedFight UMETA(DisplayName = "Two-Handed Fight"),
    ShieldNoFight UMETA(DisplayName = "Shield No Fight"),
    ShieldFight UMETA(DisplayName = "Shield Fight"),
    TwoWeaponsNoFight UMETA(DisplayName = "Two Weapons No Fight"),
    TwoWeaponsFight UMETA(DisplayName = "Two Weapons Fight")
};

UENUM(BlueprintType)
enum class ECombatEquipmentSlot : uint8
{
    MainHand UMETA(DisplayName = "Main Hand"),
    OffHand UMETA(DisplayName = "Off Hand"),
    TwoHanded UMETA(DisplayName = "Two-Handed"),
    Ranged UMETA(DisplayName = "Ranged")
};

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FCombatEquippedVariant
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    ECombatEquipmentSlot Slot = ECombatEquipmentSlot::MainHand;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    FName VariantId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    TObjectPtr<UStaticMesh> StaticMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    FName EquippedSocketName = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    FName HolsteredSocketName = NAME_None;

    // Small per-variant offset lets different weapon assets share a socket without forcing identical pivots.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    FTransform RelativeTransform = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FCombatLoadoutAnimationProfile
{
    GENERATED_BODY()

    // Exploration and combat-ready profiles stay explicit so ABP can branch differently for the same style.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout|Animation")
    ECombatIdleProfile ExplorationIdleProfile = ECombatIdleProfile::Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout|Animation")
    ECombatIdleProfile CombatIdleProfile = ECombatIdleProfile::Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout|Animation")
    ECombatLocomotionProfile ExplorationLocomotionProfile = ECombatLocomotionProfile::Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout|Animation")
    ECombatLocomotionProfile CombatLocomotionProfile = ECombatLocomotionProfile::Default;

    // Attack montage selection is resolved by the current loadout, while hit reactions can stay shared for now.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout|Animation")
    TObjectPtr<UAnimMontage> MeleeAttackMontage = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout|Animation")
    TObjectPtr<UAnimMontage> RangedAttackMontage = nullptr;
};

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FCombatStyleLoadout
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    ECombatStyleCategory StyleCategory = ECombatStyleCategory::Unarmed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    ECombatStanceContext StanceContext = ECombatStanceContext::Exploration;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout")
    FCombatLoadoutAnimationProfile AnimationProfile;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat|Loadout", meta = (TitleProperty = "Slot"))
    TArray<FCombatEquippedVariant> EquippedVariants;
};
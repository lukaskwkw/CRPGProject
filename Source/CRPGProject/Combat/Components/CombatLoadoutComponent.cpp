#include "Combat/Components/CombatLoadoutComponent.h"

#include "Animation/AnimMontage.h"
#include "Engine/GameInstance.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"

namespace CombatLoadoutEvents
{
    static const FString LoadoutChanged = TEXT("combat_loadout_changed");
    static const FString StanceChanged = TEXT("combat_stance_changed");
}

UCombatLoadoutComponent::UCombatLoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UCombatLoadoutComponent::BeginPlay()
{
    Super::BeginPlay();

    RefreshOwnerLoadoutVisuals();
}

FCombatStyleLoadout UCombatLoadoutComponent::GetCurrentLoadout() const
{
    return CurrentLoadout;
}

void UCombatLoadoutComponent::SetCurrentLoadout(const FCombatStyleLoadout &NewLoadout)
{
    CurrentLoadout = NewLoadout;
    RefreshOwnerLoadoutVisuals();
    PublishLoadoutChangedEvent();
    PublishStanceChangedEvent();
}

void UCombatLoadoutComponent::ApplyComposedLoadout(
    ECombatStyleCategory NewStyleCategory,
    const FCombatLoadoutAnimationProfile &NewAnimationProfile,
    const TArray<FCombatEquippedVariant> &NewEquippedVariants,
    ECombatStanceContext NewStanceContext)
{
    FCombatStyleLoadout NewLoadout;
    NewLoadout.StyleCategory = NewStyleCategory;
    NewLoadout.StanceContext = NewStanceContext;
    NewLoadout.AnimationProfile = NewAnimationProfile;
    NewLoadout.EquippedVariants = NewEquippedVariants;
    SetCurrentLoadout(NewLoadout);
}

void UCombatLoadoutComponent::ApplyUnarmedLoadout(
    const FCombatLoadoutAnimationProfile &NewAnimationProfile,
    ECombatStanceContext NewStanceContext)
{
    ApplyComposedLoadout(ECombatStyleCategory::Unarmed, NewAnimationProfile, {}, NewStanceContext);
}

void UCombatLoadoutComponent::ApplyBowLoadout(
    const FCombatEquippedVariant &BowVariant,
    const FCombatLoadoutAnimationProfile &NewAnimationProfile,
    ECombatStanceContext NewStanceContext)
{
    FCombatEquippedVariant ResolvedBowVariant = BowVariant;
    ResolvedBowVariant.Slot = ECombatEquipmentSlot::Ranged;
    ApplyComposedLoadout(ECombatStyleCategory::Bow, NewAnimationProfile, {ResolvedBowVariant}, NewStanceContext);
}

void UCombatLoadoutComponent::ApplyTwoHandedLoadout(
    const FCombatEquippedVariant &TwoHandedWeaponVariant,
    const FCombatLoadoutAnimationProfile &NewAnimationProfile,
    ECombatStanceContext NewStanceContext)
{
    FCombatEquippedVariant ResolvedTwoHandedVariant = TwoHandedWeaponVariant;
    ResolvedTwoHandedVariant.Slot = ECombatEquipmentSlot::TwoHanded;
    ApplyComposedLoadout(ECombatStyleCategory::TwoHanded, NewAnimationProfile, {ResolvedTwoHandedVariant}, NewStanceContext);
}

void UCombatLoadoutComponent::ApplyShieldLoadout(
    const FCombatEquippedVariant &MainHandWeaponVariant,
    const FCombatEquippedVariant &ShieldVariant,
    const FCombatLoadoutAnimationProfile &NewAnimationProfile,
    ECombatStanceContext NewStanceContext)
{
    FCombatEquippedVariant ResolvedMainHandVariant = MainHandWeaponVariant;
    ResolvedMainHandVariant.Slot = ECombatEquipmentSlot::MainHand;

    FCombatEquippedVariant ResolvedShieldVariant = ShieldVariant;
    ResolvedShieldVariant.Slot = ECombatEquipmentSlot::OffHand;

    ApplyComposedLoadout(ECombatStyleCategory::Shield, NewAnimationProfile, {ResolvedMainHandVariant, ResolvedShieldVariant}, NewStanceContext);
}

void UCombatLoadoutComponent::ApplyTwoWeaponsLoadout(
    const FCombatEquippedVariant &MainHandWeaponVariant,
    const FCombatEquippedVariant &OffHandWeaponVariant,
    const FCombatLoadoutAnimationProfile &NewAnimationProfile,
    ECombatStanceContext NewStanceContext)
{
    FCombatEquippedVariant ResolvedMainHandVariant = MainHandWeaponVariant;
    ResolvedMainHandVariant.Slot = ECombatEquipmentSlot::MainHand;

    FCombatEquippedVariant ResolvedOffHandVariant = OffHandWeaponVariant;
    ResolvedOffHandVariant.Slot = ECombatEquipmentSlot::OffHand;

    ApplyComposedLoadout(ECombatStyleCategory::TwoWeapons, NewAnimationProfile, {ResolvedMainHandVariant, ResolvedOffHandVariant}, NewStanceContext);
}

ECombatStyleCategory UCombatLoadoutComponent::GetCombatStyleCategory() const
{
    return CurrentLoadout.StyleCategory;
}

void UCombatLoadoutComponent::SetCombatStyleCategory(ECombatStyleCategory NewStyleCategory)
{
    if (CurrentLoadout.StyleCategory == NewStyleCategory)
    {
        return;
    }

    CurrentLoadout.StyleCategory = NewStyleCategory;
    RefreshOwnerLoadoutVisuals();
    PublishLoadoutChangedEvent();
}

ECombatStanceContext UCombatLoadoutComponent::GetCombatStanceContext() const
{
    return CurrentLoadout.StanceContext;
}

void UCombatLoadoutComponent::SetCombatStanceContext(ECombatStanceContext NewStanceContext)
{
    if (CurrentLoadout.StanceContext == NewStanceContext)
    {
        UE_LOG(LogTemp, Log, TEXT("[CombatLoadoutComponent] Stance unchanged for owner=%s stance=%d"), *GetNameSafe(GetOwner()), static_cast<int32>(NewStanceContext));
        return;
    }

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[CombatLoadoutComponent] Stance changing for owner=%s from=%d to=%d idle=%d locomotion=%d"),
        *GetNameSafe(GetOwner()),
        static_cast<int32>(CurrentLoadout.StanceContext),
        static_cast<int32>(NewStanceContext),
        static_cast<int32>(GetCurrentIdleProfile()),
        static_cast<int32>(GetCurrentLocomotionProfile()));

    CurrentLoadout.StanceContext = NewStanceContext;
    RefreshOwnerLoadoutVisuals();
    PublishStanceChangedEvent();

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[CombatLoadoutComponent] Stance applied for owner=%s stance=%d idle=%d locomotion=%d"),
        *GetNameSafe(GetOwner()),
        static_cast<int32>(CurrentLoadout.StanceContext),
        static_cast<int32>(GetCurrentIdleProfile()),
        static_cast<int32>(GetCurrentLocomotionProfile()));
}

ECombatIdleProfile UCombatLoadoutComponent::GetCurrentIdleProfile() const
{
    // The same combat style may expose different authored idles for exploration versus combat-ready stance.
    return IsCombatReadyStanceActive() ? CurrentLoadout.AnimationProfile.CombatIdleProfile : CurrentLoadout.AnimationProfile.ExplorationIdleProfile;
}

ECombatLocomotionProfile UCombatLoadoutComponent::GetCurrentLocomotionProfile() const
{
    // Locomotion follows the same stance split as idle so a style can swap both pose and movement feel together.
    return IsCombatReadyStanceActive() ? CurrentLoadout.AnimationProfile.CombatLocomotionProfile : CurrentLoadout.AnimationProfile.ExplorationLocomotionProfile;
}

bool UCombatLoadoutComponent::IsCombatReadyStanceActive() const
{
    return CurrentLoadout.StanceContext == ECombatStanceContext::CombatReady;
}

bool UCombatLoadoutComponent::IsExplorationStanceActive() const
{
    return CurrentLoadout.StanceContext == ECombatStanceContext::Exploration;
}

bool UCombatLoadoutComponent::UsesStyle(ECombatStyleCategory StyleCategory) const
{
    return CurrentLoadout.StyleCategory == StyleCategory;
}

bool UCombatLoadoutComponent::UsesUnarmedStyle() const
{
    return UsesStyle(ECombatStyleCategory::Unarmed);
}

bool UCombatLoadoutComponent::UsesBowStyle() const
{
    return UsesStyle(ECombatStyleCategory::Bow);
}

bool UCombatLoadoutComponent::UsesTwoHandedStyle() const
{
    return UsesStyle(ECombatStyleCategory::TwoHanded);
}

bool UCombatLoadoutComponent::UsesShieldStyle() const
{
    return UsesStyle(ECombatStyleCategory::Shield);
}

bool UCombatLoadoutComponent::UsesTwoWeaponsStyle() const
{
    return UsesStyle(ECombatStyleCategory::TwoWeapons);
}

bool UCombatLoadoutComponent::UsesIdleProfile(ECombatIdleProfile IdleProfile) const
{
    return GetCurrentIdleProfile() == IdleProfile;
}

bool UCombatLoadoutComponent::UsesLocomotionProfile(ECombatLocomotionProfile LocomotionProfile) const
{
    return GetCurrentLocomotionProfile() == LocomotionProfile;
}

bool UCombatLoadoutComponent::UsesFightIdleProfile() const
{
    switch (GetCurrentIdleProfile())
    {
    case ECombatIdleProfile::UnarmedFight:
    case ECombatIdleProfile::BowFight:
    case ECombatIdleProfile::TwoHandedFight:
    case ECombatIdleProfile::ShieldFight:
    case ECombatIdleProfile::TwoWeaponsFight:
        return true;
    default:
        return false;
    }
}

bool UCombatLoadoutComponent::UsesNoFightIdleProfile() const
{
    switch (GetCurrentIdleProfile())
    {
    case ECombatIdleProfile::UnarmedNoFight:
    case ECombatIdleProfile::BowNoFight:
    case ECombatIdleProfile::TwoHandedNoFight:
    case ECombatIdleProfile::ShieldNoFight:
    case ECombatIdleProfile::TwoWeaponsNoFight:
        return true;
    default:
        return false;
    }
}

bool UCombatLoadoutComponent::UsesFightLocomotionProfile() const
{
    switch (GetCurrentLocomotionProfile())
    {
    case ECombatLocomotionProfile::UnarmedFight:
    case ECombatLocomotionProfile::BowFight:
    case ECombatLocomotionProfile::TwoHandedFight:
    case ECombatLocomotionProfile::ShieldFight:
    case ECombatLocomotionProfile::TwoWeaponsFight:
        return true;
    default:
        return false;
    }
}

bool UCombatLoadoutComponent::UsesNoFightLocomotionProfile() const
{
    switch (GetCurrentLocomotionProfile())
    {
    case ECombatLocomotionProfile::UnarmedNoFight:
    case ECombatLocomotionProfile::BowNoFight:
    case ECombatLocomotionProfile::TwoHandedNoFight:
    case ECombatLocomotionProfile::ShieldNoFight:
    case ECombatLocomotionProfile::TwoWeaponsNoFight:
        return true;
    default:
        return false;
    }
}

void UCombatLoadoutComponent::SetLoadoutAnimationProfile(const FCombatLoadoutAnimationProfile &NewAnimationProfile)
{
    CurrentLoadout.AnimationProfile = NewAnimationProfile;
    RefreshOwnerLoadoutVisuals();
    PublishLoadoutChangedEvent();
}

void UCombatLoadoutComponent::SetEquippedVariantForSlot(ECombatEquipmentSlot Slot, const FCombatEquippedVariant &NewVariant)
{
    for (FCombatEquippedVariant &ExistingVariant : CurrentLoadout.EquippedVariants)
    {
        if (ExistingVariant.Slot == Slot)
        {
            ExistingVariant = NewVariant;
            ExistingVariant.Slot = Slot;
            RefreshOwnerLoadoutVisuals();
            PublishLoadoutChangedEvent();
            return;
        }
    }

    FCombatEquippedVariant VariantToAdd = NewVariant;
    VariantToAdd.Slot = Slot;
    CurrentLoadout.EquippedVariants.Add(VariantToAdd);
    RefreshOwnerLoadoutVisuals();
    PublishLoadoutChangedEvent();
}

void UCombatLoadoutComponent::SetEquippedVariants(const TArray<FCombatEquippedVariant> &NewEquippedVariants)
{
    CurrentLoadout.EquippedVariants = NewEquippedVariants;
    RefreshOwnerLoadoutVisuals();
    PublishLoadoutChangedEvent();
}

void UCombatLoadoutComponent::ClearEquippedVariants()
{
    if (CurrentLoadout.EquippedVariants.Num() == 0)
    {
        return;
    }

    CurrentLoadout.EquippedVariants.Reset();
    RefreshOwnerLoadoutVisuals();
    PublishLoadoutChangedEvent();
}

bool UCombatLoadoutComponent::RemoveEquippedVariantForSlot(ECombatEquipmentSlot Slot)
{
    const int32 RemovedCount = CurrentLoadout.EquippedVariants.RemoveAll([Slot](const FCombatEquippedVariant &Variant)
                                                                         { return Variant.Slot == Slot; });

    if (RemovedCount <= 0)
    {
        return false;
    }

    RefreshOwnerLoadoutVisuals();
    PublishLoadoutChangedEvent();
    return true;
}

bool UCombatLoadoutComponent::FindEquippedVariant(ECombatEquipmentSlot Slot, FCombatEquippedVariant &OutVariant) const
{
    for (const FCombatEquippedVariant &ExistingVariant : CurrentLoadout.EquippedVariants)
    {
        if (ExistingVariant.Slot == Slot)
        {
            OutVariant = ExistingVariant;
            return true;
        }
    }

    return false;
}

UAnimMontage *UCombatLoadoutComponent::GetResolvedAttackMontage(ECombatActionType ActionType) const
{
    switch (ActionType)
    {
    case ECombatActionType::MeleeAttack:
        return CurrentLoadout.AnimationProfile.MeleeAttackMontage;
    case ECombatActionType::RangedAttack:
        return CurrentLoadout.AnimationProfile.RangedAttackMontage;
    default:
        return nullptr;
    }
}

void UCombatLoadoutComponent::RefreshOwnerLoadoutVisuals() const
{
    if (ACRPGBaseCharacter *OwnerCharacter = Cast<ACRPGBaseCharacter>(GetOwner()))
    {
        OwnerCharacter->ApplyCombatLoadoutVisuals();
    }
}

void UCombatLoadoutComponent::PublishLoadoutChangedEvent() const
{
    const UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UEventBusSubsystem *EventBusSubsystem = GameInstance ? GameInstance->GetSubsystem<UEventBusSubsystem>() : nullptr;
    if (EventBusSubsystem)
    {
        EventBusSubsystem->PublishEvent(
            CombatLoadoutEvents::LoadoutChanged,
            FString::Printf(
                TEXT("owner=%s;style=%d;stance=%d;variant_count=%d"),
                *GetNameSafe(GetOwner()),
                static_cast<int32>(CurrentLoadout.StyleCategory),
                static_cast<int32>(CurrentLoadout.StanceContext),
                CurrentLoadout.EquippedVariants.Num()));
    }
}

void UCombatLoadoutComponent::PublishStanceChangedEvent() const
{
    const UGameInstance *GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UEventBusSubsystem *EventBusSubsystem = GameInstance ? GameInstance->GetSubsystem<UEventBusSubsystem>() : nullptr;
    if (EventBusSubsystem)
    {
        EventBusSubsystem->PublishEvent(
            CombatLoadoutEvents::StanceChanged,
            FString::Printf(
                TEXT("owner=%s;style=%d;stance=%d;idle=%d;locomotion=%d"),
                *GetNameSafe(GetOwner()),
                static_cast<int32>(CurrentLoadout.StyleCategory),
                static_cast<int32>(CurrentLoadout.StanceContext),
                static_cast<int32>(GetCurrentIdleProfile()),
                static_cast<int32>(GetCurrentLocomotionProfile())));
    }
}
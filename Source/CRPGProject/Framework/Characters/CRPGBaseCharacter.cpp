#include "CRPGBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Combat/Components/CombatLoadoutComponent.h"
#include "Combat/Types/CombatTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "CRPGAttributeSet.h"
#include "Engine/GameInstance.h"
#include "Engine/CollisionProfile.h"
#include "NavAreas/NavArea_Obstacle.h"
#include "NavigationSystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "World/Navigation/TacticalTurnNavAreas.h"

ACRPGBaseCharacter::ACRPGBaseCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;

    // Base character owns the gameplay data component and the nav-only blocker so all derived pawns share the same rules.
    AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UCRPGAttributeSet>(TEXT("AttributeSet"));
    TacticalUnitComponent = CreateDefaultSubobject<UTacticalUnitComponent>(TEXT("TacticalUnitComponent"));
    CombatLoadoutComponent = CreateDefaultSubobject<UCombatLoadoutComponent>(TEXT("CombatLoadoutComponent"));
    MainHandEquipmentStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainHandEquipmentStaticMesh"));
    MainHandEquipmentSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MainHandEquipmentSkeletalMesh"));
    OffHandEquipmentStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OffHandEquipmentStaticMesh"));
    OffHandEquipmentSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("OffHandEquipmentSkeletalMesh"));
    TwoHandedEquipmentStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TwoHandedEquipmentStaticMesh"));
    TwoHandedEquipmentSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TwoHandedEquipmentSkeletalMesh"));
    RangedEquipmentStaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RangedEquipmentStaticMesh"));
    RangedEquipmentSkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("RangedEquipmentSkeletalMesh"));
    TacticalOccupancyNavigationBlocker = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TacticalOccupancyNavigationBlocker"));
    CombatFeedbackText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CombatFeedbackText"));

    InitializeEquipmentVisualComponent(MainHandEquipmentStaticMesh);
    InitializeEquipmentVisualComponent(MainHandEquipmentSkeletalMesh);
    InitializeEquipmentVisualComponent(OffHandEquipmentStaticMesh);
    InitializeEquipmentVisualComponent(OffHandEquipmentSkeletalMesh);
    InitializeEquipmentVisualComponent(TwoHandedEquipmentStaticMesh);
    InitializeEquipmentVisualComponent(TwoHandedEquipmentSkeletalMesh);
    InitializeEquipmentVisualComponent(RangedEquipmentStaticMesh);
    InitializeEquipmentVisualComponent(RangedEquipmentSkeletalMesh);

    if (TacticalOccupancyNavigationBlocker)
    {
        // This capsule never collides physically. It only projects occupied space into the navmesh as an obstacle area.
        TacticalOccupancyNavigationBlocker->SetupAttachment(GetRootComponent());
        TacticalOccupancyNavigationBlocker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        TacticalOccupancyNavigationBlocker->SetCollisionResponseToAllChannels(ECR_Ignore);
        TacticalOccupancyNavigationBlocker->SetGenerateOverlapEvents(false);
        TacticalOccupancyNavigationBlocker->SetHiddenInGame(true);
        TacticalOccupancyNavigationBlocker->bDynamicObstacle = true;
        TacticalOccupancyNavigationBlocker->SetCanEverAffectNavigation(false);
        TacticalOccupancyNavigationBlocker->SetAreaClassOverride(UNavArea_Obstacle::StaticClass());
    }

    if (CombatFeedbackText)
    {
        CombatFeedbackText->SetupAttachment(GetRootComponent());
        CombatFeedbackText->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
        CombatFeedbackText->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
        CombatFeedbackText->SetWorldSize(42.0f);
        CombatFeedbackText->SetTextRenderColor(FColor::White);
        CombatFeedbackText->SetHiddenInGame(true);
        CombatFeedbackText->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CombatFeedbackText->SetGenerateOverlapEvents(false);
        CombatFeedbackText->SetCastShadow(false);
    }

    if (USkeletalMeshComponent *CharacterMesh = GetMesh())
    {
        MainHandEquipmentStaticMesh->SetupAttachment(CharacterMesh);
        MainHandEquipmentSkeletalMesh->SetupAttachment(CharacterMesh);
        OffHandEquipmentStaticMesh->SetupAttachment(CharacterMesh);
        OffHandEquipmentSkeletalMesh->SetupAttachment(CharacterMesh);
        TwoHandedEquipmentStaticMesh->SetupAttachment(CharacterMesh);
        TwoHandedEquipmentSkeletalMesh->SetupAttachment(CharacterMesh);
        RangedEquipmentStaticMesh->SetupAttachment(CharacterMesh);
        RangedEquipmentSkeletalMesh->SetupAttachment(CharacterMesh);
    }
}

UAbilitySystemComponent *ACRPGBaseCharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ACRPGBaseCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bCombatFeedbackAnimating)
    {
        UpdateCombatFeedbackPresentation(DeltaSeconds);
    }
}

void ACRPGBaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    StandingCapsuleHalfHeightCm = GetCapsuleComponent() ? GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : 0.0f;

    if (CombatFeedbackText)
    {
        const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : 88.0f;
        CombatFeedbackBaseRelativeLocation = FVector(0.0f, 0.0f, CapsuleHalfHeight + CombatFeedbackHeightOffsetCm);
        CombatFeedbackText->SetRelativeLocation(CombatFeedbackBaseRelativeLocation);
    }

    // Push the initial occupancy state before registering the unit so previews see the correct blocker size immediately.
    UpdateTacticalOccupancyNavigationBlocker();
    ApplyCombatLoadoutVisuals();

    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GameInstance->GetSubsystem<UTacticalTurnSubsystem>())
        {
            TacticalTurnSubsystem->RegisterUnit(this);
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[CRPGBaseCharacter] GAS Initialized for %s"), *GetName());
}

void ACRPGBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GameInstance->GetSubsystem<UTacticalTurnSubsystem>())
        {
            TacticalTurnSubsystem->UnregisterUnit(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

float ACRPGBaseCharacter::PlayMeleeAttackMontage()
{
    if (CombatLoadoutComponent)
    {
        if (UAnimMontage *ResolvedMontage = CombatLoadoutComponent->GetResolvedAttackMontage(ECombatActionType::MeleeAttack))
        {
            return PlayConfiguredMontageRandomSection(ResolvedMontage);
        }
    }

    return PlayConfiguredMontageRandomSection(MeleeAttackMontage);
}

float ACRPGBaseCharacter::PlayRangedAttackMontage()
{
    if (CombatLoadoutComponent)
    {
        if (UAnimMontage *ResolvedMontage = CombatLoadoutComponent->GetResolvedAttackMontage(ECombatActionType::RangedAttack))
        {
            return PlayConfiguredMontageRandomSection(ResolvedMontage);
        }
    }

    return PlayConfiguredMontageRandomSection(RangedAttackMontage);
}

float ACRPGBaseCharacter::PlayDodgeMontage()
{
    return PlayConfiguredMontage(DodgeMontage);
}

float ACRPGBaseCharacter::PlayDodgeMontageForDirection(ECombatReactionDirection Direction)
{
    return PlayConfiguredMontageSection(DodgeMontage, Direction);
}

float ACRPGBaseCharacter::PlayHitReactMontage()
{
    return PlayConfiguredMontage(HitReactMontage);
}

float ACRPGBaseCharacter::PlayHitReactMontageForDirection(ECombatReactionDirection Direction)
{
    return PlayConfiguredMontageSection(HitReactMontage, Direction);
}

void ACRPGBaseCharacter::EnterTacticalDeathState()
{
    SetCombatTargetHighlightEnabled(false);
    SetRuntimeOutlineCategory(ECRPGOutlineCategory::None);
    SetOutlineExcluded(true);
    HideCombatFeedbackText();

    if (UCharacterMovementComponent *CharacterMovementComponent = GetCharacterMovement())
    {
        CharacterMovementComponent->DisableMovement();
        CharacterMovementComponent->StopMovementImmediately();
    }

    if (UCapsuleComponent *CharacterCapsule = GetCapsuleComponent())
    {
        if (StandingCapsuleHalfHeightCm > 0.0f)
        {
            CharacterCapsule->SetCapsuleHalfHeight(StandingCapsuleHalfHeightCm, true);
        }

        CharacterCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    if (USkeletalMeshComponent *CharacterMesh = GetMesh())
    {
        CharacterMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        CharacterMesh->SetAllBodiesSimulatePhysics(false);
        CharacterMesh->SetSimulatePhysics(false);
    }

    ECombatReactionDirection DeathDirection = ECombatReactionDirection::Front;
    const bool bHasDirectionalDeath = ConsumePendingCombatReactionDirection(DeathDirection);
    const float MontageDuration = bHasDirectionalDeath ? PlayDeathMontageForDirection(DeathDirection) : PlayDeathMontage();
    if (MontageDuration <= 0.0f)
    {
        EnableCombatRagdoll();
    }
}

float ACRPGBaseCharacter::PlayDeathMontage()
{
    return PlayConfiguredMontage(DeathMontage);
}

float ACRPGBaseCharacter::PlayDeathMontageForDirection(ECombatReactionDirection Direction)
{
    return PlayConfiguredMontageSection(DeathMontage, Direction);
}

void ACRPGBaseCharacter::SetPendingCombatReactionDirection(ECombatReactionDirection Direction)
{
    PendingCombatReactionDirection = Direction;
    bHasPendingCombatReactionDirection = true;
}

bool ACRPGBaseCharacter::ConsumePendingCombatReactionDirection(ECombatReactionDirection &OutDirection)
{
    if (!bHasPendingCombatReactionDirection)
    {
        return false;
    }

    OutDirection = PendingCombatReactionDirection;
    bHasPendingCombatReactionDirection = false;
    return true;
}

void ACRPGBaseCharacter::EnterTacticalProneState()
{
    SetCombatTargetHighlightEnabled(false);
    HideCombatFeedbackText();

    if (UCharacterMovementComponent *CharacterMovementComponent = GetCharacterMovement())
    {
        CharacterMovementComponent->DisableMovement();
        CharacterMovementComponent->StopMovementImmediately();
    }

    if (UCapsuleComponent *CharacterCapsule = GetCapsuleComponent())
    {
        if (StandingCapsuleHalfHeightCm <= 0.0f)
        {
            StandingCapsuleHalfHeightCm = CharacterCapsule->GetUnscaledCapsuleHalfHeight();
        }

        CharacterCapsule->SetCapsuleHalfHeight(FMath::Max(0.0f, ProneCapsuleHalfHeightCm), true);
    }

    PlayProneMontage();
}

float ACRPGBaseCharacter::PlayProneMontage()
{
    return PlayConfiguredMontage(ProneMontage);
}

void ACRPGBaseCharacter::RecoverFromPronePresentation()
{
    if (UCapsuleComponent *CharacterCapsule = GetCapsuleComponent(); CharacterCapsule && StandingCapsuleHalfHeightCm > 0.0f)
    {
        CharacterCapsule->SetCapsuleHalfHeight(StandingCapsuleHalfHeightCm, true);
    }

    if (UCharacterMovementComponent *CharacterMovementComponent = GetCharacterMovement())
    {
        CharacterMovementComponent->SetMovementMode(MOVE_Walking);
        CharacterMovementComponent->StopMovementImmediately();
    }

    PlayStandUpMontage();
}

float ACRPGBaseCharacter::PlayStandUpMontage()
{
    return PlayConfiguredMontage(StandUpMontage);
}

void ACRPGBaseCharacter::EnableCombatRagdoll()
{
    if (USkeletalMeshComponent *CharacterMesh = GetMesh())
    {
        CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        CharacterMesh->SetCollisionProfileName(TEXT("Ragdoll"));
        CharacterMesh->SetAllBodiesSimulatePhysics(true);
        CharacterMesh->WakeAllRigidBodies();
    }
}

void ACRPGBaseCharacter::SetCombatTargetHighlightEnabled(bool bEnabled)
{
    bCombatTargetHighlightEnabled = bEnabled;
    RefreshOutlinePresentation();
}

void ACRPGBaseCharacter::SetOutlineCategory(ECRPGOutlineCategory NewCategory)
{
    if (OutlineCategory == NewCategory)
    {
        return;
    }

    OutlineCategory = NewCategory;
    RefreshOutlinePresentation();
}

void ACRPGBaseCharacter::ClearOutlineCategory()
{
    SetOutlineCategory(ECRPGOutlineCategory::None);
}

void ACRPGBaseCharacter::SetOutlineCategoryToPartyMember()
{
    SetOutlineCategory(ECRPGOutlineCategory::PartyMember);
}

void ACRPGBaseCharacter::SetOutlineCategoryToActivePartyMember()
{
    SetOutlineCategory(ECRPGOutlineCategory::ActivePartyMember);
}

void ACRPGBaseCharacter::SetOutlineCategoryToFriendlyNonParty()
{
    SetOutlineCategory(ECRPGOutlineCategory::FriendlyNonParty);
}

void ACRPGBaseCharacter::SetOutlineCategoryToNeutral()
{
    SetOutlineCategory(ECRPGOutlineCategory::Neutral);
}

void ACRPGBaseCharacter::SetOutlineCategoryToEnemy()
{
    SetOutlineCategory(ECRPGOutlineCategory::Enemy);
}

void ACRPGBaseCharacter::SetOutlineCategoryToInteractable()
{
    SetOutlineCategory(ECRPGOutlineCategory::Interactable);
}

void ACRPGBaseCharacter::SetRuntimeOutlineCategory(ECRPGOutlineCategory NewCategory)
{
    if (RuntimeOutlineCategory == NewCategory)
    {
        return;
    }

    RuntimeOutlineCategory = NewCategory;
    RefreshOutlinePresentation();
}

void ACRPGBaseCharacter::SetOutlineExcluded(bool bExcluded)
{
    if (bOutlineExcluded == bExcluded)
    {
        return;
    }

    bOutlineExcluded = bExcluded;
    RefreshOutlinePresentation();
}

void ACRPGBaseCharacter::RefreshOutlinePresentation()
{
    if (USkeletalMeshComponent *CharacterMesh = GetMesh())
    {
        const ECRPGOutlineCategory IdleCategory = bShowOutlineCategoryWhenIdle ? OutlineCategory : ECRPGOutlineCategory::None;
        const ECRPGOutlineCategory BaseCategory = RuntimeOutlineCategory != ECRPGOutlineCategory::None ? RuntimeOutlineCategory : IdleCategory;
        const ECRPGOutlineCategory EffectiveCategory = bOutlineExcluded
                                                           ? ECRPGOutlineCategory::None
                                                           : (bCombatTargetHighlightEnabled ? ECRPGOutlineCategory::HoveredEnemy : BaseCategory);
        const bool bEnableOutline = EffectiveCategory != ECRPGOutlineCategory::None;

        // Hover combat target keeps the highest local priority, while persistent category overlays use their own stencil values.
        CharacterMesh->SetRenderCustomDepth(bEnableOutline);
        CharacterMesh->SetCustomDepthStencilValue(static_cast<int32>(EffectiveCategory));
    }
}

void ACRPGBaseCharacter::ShowCombatAttackResult(const FCombatAttackResult &AttackResult)
{
    if (!AttackResult.bHit)
    {
        ShowCombatFeedbackText(TEXT("MISS"), FColor(196, 200, 208));
        return;
    }

    if (AttackResult.bCriticalHit)
    {
        ShowCombatFeedbackText(FString::Printf(TEXT("CRIT %d"), AttackResult.DamageRoll), FColor(255, 180, 32));
        return;
    }

    ShowCombatFeedbackText(FString::FromInt(AttackResult.DamageRoll), FColor(255, 96, 96));
}

void ACRPGBaseCharacter::ShowCombatFeedbackText(const FString &Text, const FColor &Color)
{
    if (!CombatFeedbackText)
    {
        return;
    }

    CombatFeedbackText->SetText(FText::FromString(Text));
    CombatFeedbackActiveColor = Color;
    CombatFeedbackText->SetTextRenderColor(CombatFeedbackActiveColor);
    CombatFeedbackElapsedSeconds = 0.0f;
    bCombatFeedbackAnimating = true;
    CombatFeedbackText->SetRelativeLocation(CombatFeedbackBaseRelativeLocation);
    CombatFeedbackText->SetHiddenInGame(false);
    SetActorTickEnabled(true);

    GetWorldTimerManager().ClearTimer(CombatFeedbackHideTimerHandle);
    if (CombatFeedbackDurationSeconds <= 0.0f)
    {
        return;
    }

    GetWorldTimerManager().SetTimer(CombatFeedbackHideTimerHandle, this, &ACRPGBaseCharacter::HideCombatFeedbackText, CombatFeedbackDurationSeconds, false);
}

void ACRPGBaseCharacter::HideCombatFeedbackText()
{
    bCombatFeedbackAnimating = false;

    if (CombatFeedbackText)
    {
        CombatFeedbackText->SetHiddenInGame(true);
        CombatFeedbackText->SetRelativeLocation(CombatFeedbackBaseRelativeLocation);
        CombatFeedbackText->SetTextRenderColor(CombatFeedbackActiveColor);
    }

    SetActorTickEnabled(false);
}

void ACRPGBaseCharacter::UpdateCombatFeedbackPresentation(float DeltaSeconds)
{
    if (!CombatFeedbackText || CombatFeedbackDurationSeconds <= 0.0f)
    {
        return;
    }

    CombatFeedbackElapsedSeconds = FMath::Min(CombatFeedbackElapsedSeconds + DeltaSeconds, CombatFeedbackDurationSeconds);
    const float Alpha = FMath::Clamp(CombatFeedbackElapsedSeconds / CombatFeedbackDurationSeconds, 0.0f, 1.0f);
    const float SmoothedAlpha = FMath::InterpEaseOut(0.0f, 1.0f, Alpha, 2.0f);
    const float HeightOffsetCm = FMath::Lerp(0.0f, CombatFeedbackFloatDistanceCm, SmoothedAlpha);
    CombatFeedbackText->SetRelativeLocation(CombatFeedbackBaseRelativeLocation + FVector(0.0f, 0.0f, HeightOffsetCm));

    if (APlayerCameraManager *CameraManager = GetWorld() ? GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->PlayerCameraManager : nullptr : nullptr)
    {
        FVector FacingDirection = CameraManager->GetCameraLocation() - CombatFeedbackText->GetComponentLocation();
        if (!FacingDirection.IsNearlyZero())
        {
            FRotator LookRotation = FacingDirection.Rotation();
            LookRotation.Roll = 0.0f;
            CombatFeedbackText->SetWorldRotation(LookRotation);
        }
    }

    const float FadeStartFraction = FMath::Clamp(CombatFeedbackFadeStartFraction, 0.0f, 1.0f);
    float FadeAlpha = 1.0f;
    if (Alpha > FadeStartFraction)
    {
        const float FadeDenominator = FMath::Max(KINDA_SMALL_NUMBER, 1.0f - FadeStartFraction);
        FadeAlpha = 1.0f - ((Alpha - FadeStartFraction) / FadeDenominator);
    }

    FColor AnimatedColor = CombatFeedbackActiveColor;
    AnimatedColor.A = static_cast<uint8>(FMath::RoundToInt(255.0f * FMath::Clamp(FadeAlpha, 0.0f, 1.0f)));
    CombatFeedbackText->SetTextRenderColor(AnimatedColor);
}

float ACRPGBaseCharacter::PlayConfiguredMontage(UAnimMontage *Montage)
{
    return Montage ? PlayAnimMontage(Montage) : 0.0f;
}

float ACRPGBaseCharacter::PlayConfiguredMontageRandomSection(UAnimMontage *Montage)
{
    if (!Montage)
    {
        return 0.0f;
    }

    TArray<FName, TInlineAllocator<8>> AvailableSections;
    for (int32 SectionIndex = 0; SectionIndex < Montage->GetNumSections(); ++SectionIndex)
    {
        const FName SectionName = Montage->GetSectionName(SectionIndex);
        if (SectionName != NAME_None)
        {
            AvailableSections.Add(SectionName);
        }
    }

    if (AvailableSections.Num() == 0)
    {
        return PlayAnimMontage(Montage);
    }

    const int32 RandomSectionIndex = FMath::RandRange(0, AvailableSections.Num() - 1);
    UE_LOG(LogTemp, Log, TEXT("[CRPGBaseCharacter] Playing random attack montage=%s section=%s on character=%s"), *GetNameSafe(Montage), *AvailableSections[RandomSectionIndex].ToString(), *GetNameSafe(this));
    return PlayAnimMontage(Montage, 1.0f, AvailableSections[RandomSectionIndex]);
}

float ACRPGBaseCharacter::PlayConfiguredMontageSection(UAnimMontage *Montage, ECombatReactionDirection Direction)
{
    if (!Montage)
    {
        return 0.0f;
    }

    const FName DesiredSection = GetReactionDirectionSectionName(Direction);
    if (DesiredSection != NAME_None && Montage->GetSectionIndex(DesiredSection) != INDEX_NONE)
    {
        UE_LOG(LogTemp, Log, TEXT("[CRPGBaseCharacter] Playing montage=%s section=%s on character=%s"), *GetNameSafe(Montage), *DesiredSection.ToString(), *GetNameSafe(this));
        return PlayAnimMontage(Montage, 1.0f, DesiredSection);
    }

    TArray<FName, TInlineAllocator<4>> AvailableSections;
    for (int32 SectionIndex = 0; SectionIndex < Montage->GetNumSections(); ++SectionIndex)
    {
        const FName SectionName = Montage->GetSectionName(SectionIndex);
        if (SectionName != NAME_None)
        {
            AvailableSections.Add(SectionName);
        }
    }

    if (AvailableSections.Num() > 0)
    {
        const int32 RandomSectionIndex = FMath::RandRange(0, AvailableSections.Num() - 1);
        UE_LOG(LogTemp, Warning, TEXT("[CRPGBaseCharacter] Missing desired section=%s in montage=%s on character=%s, falling back to random section=%s"), *DesiredSection.ToString(), *GetNameSafe(Montage), *GetNameSafe(this), *AvailableSections[RandomSectionIndex].ToString());
        return PlayAnimMontage(Montage, 1.0f, AvailableSections[RandomSectionIndex]);
    }

    UE_LOG(LogTemp, Warning, TEXT("[CRPGBaseCharacter] Montage=%s on character=%s has no named sections, playing full montage for desired section=%s"), *GetNameSafe(Montage), *GetNameSafe(this), *DesiredSection.ToString());
    return PlayAnimMontage(Montage);
}

FName ACRPGBaseCharacter::GetReactionDirectionSectionName(ECombatReactionDirection Direction) const
{
    switch (Direction)
    {
    case ECombatReactionDirection::Front:
        return TEXT("Front");
    case ECombatReactionDirection::Back:
        return TEXT("Back");
    case ECombatReactionDirection::Left:
        return TEXT("Left");
    case ECombatReactionDirection::Right:
        return TEXT("Right");
    default:
        return NAME_None;
    }
}

void ACRPGBaseCharacter::UpdateTacticalOccupancyNavigationBlocker(const ACRPGBaseCharacter *ReferenceCharacter)
{
    if (!TacticalOccupancyNavigationBlocker)
    {
        return;
    }

    // The blocker size is data-driven from the tactical unit so small and large units can reserve different footprints.
    const UTacticalUnitComponent *UnitComponent = GetTacticalUnitComponent();
    const UTacticalTurnSubsystem *TacticalTurnSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTacticalTurnSubsystem>() : nullptr;
    const bool bIsTurnModeActive = TacticalTurnSubsystem && TacticalTurnSubsystem->IsTurnModeActive();
    bool bShouldBlockNavigation = bIsTurnModeActive && UnitComponent && UnitComponent->IsOccupyingTacticalSpace();
    float BlockerRadius = UnitComponent ? UnitComponent->GetNavigationBlockerRadiusCm() : 1.0f;
    float BlockerHalfHeight = UnitComponent ? UnitComponent->GetNavigationBlockerHalfHeightCm() : 1.0f;
    TSubclassOf<UNavArea> AreaClass = UNavArea_Obstacle::StaticClass();

    if (!UnitComponent)
    {
        if (const UCapsuleComponent *CharacterCapsuleComponent = GetCapsuleComponent())
        {
            BlockerRadius = FMath::Max(1.0f, CharacterCapsuleComponent->GetUnscaledCapsuleRadius());
            BlockerHalfHeight = FMath::Max(1.0f, CharacterCapsuleComponent->GetUnscaledCapsuleHalfHeight());
        }
    }

    if (!bIsTurnModeActive)
    {
        bShouldBlockNavigation = false;
    }
    else if (ReferenceCharacter == this)
    {
        bShouldBlockNavigation = false;
    }
    else if (ReferenceCharacter)
    {
        AreaClass = UTacticalTurnNavArea_EnemyOccupied::StaticClass();
    }

    TacticalOccupancyNavigationBlocker->SetCapsuleSize(BlockerRadius, BlockerHalfHeight);
    TacticalOccupancyNavigationBlocker->SetAreaClassOverride(AreaClass);
    TacticalOccupancyNavigationBlocker->SetCanEverAffectNavigation(bShouldBlockNavigation);

    if (bShouldBlockNavigation && TacticalOccupancyNavigationBlocker->IsRegistered())
    {
        // Area class and extent changes do not automatically refresh nav octree data for an already registered shape component.
        FNavigationSystem::UpdateComponentData(*TacticalOccupancyNavigationBlocker);
    }
}

UTacticalUnitComponent *ACRPGBaseCharacter::GetTacticalUnitComponent() const
{
    return TacticalUnitComponent;
}

UCombatLoadoutComponent *ACRPGBaseCharacter::GetCombatLoadoutComponent() const
{
    return CombatLoadoutComponent;
}

ECombatStyleCategory ACRPGBaseCharacter::GetCombatStyleCategory() const
{
    return CombatLoadoutComponent ? CombatLoadoutComponent->GetCombatStyleCategory() : ECombatStyleCategory::Unarmed;
}

ECombatStanceContext ACRPGBaseCharacter::GetCombatStanceContext() const
{
    return CombatLoadoutComponent ? CombatLoadoutComponent->GetCombatStanceContext() : ECombatStanceContext::Exploration;
}

void ACRPGBaseCharacter::SetCombatStanceContext(ECombatStanceContext NewStanceContext)
{
    if (CombatLoadoutComponent)
    {
        CombatLoadoutComponent->SetCombatStanceContext(NewStanceContext);
    }
}

ECombatIdleProfile ACRPGBaseCharacter::GetCurrentIdleProfile() const
{
    return CombatLoadoutComponent ? CombatLoadoutComponent->GetCurrentIdleProfile() : ECombatIdleProfile::Default;
}

ECombatLocomotionProfile ACRPGBaseCharacter::GetCurrentLocomotionProfile() const
{
    return CombatLoadoutComponent ? CombatLoadoutComponent->GetCurrentLocomotionProfile() : ECombatLocomotionProfile::Default;
}

bool ACRPGBaseCharacter::IsCombatReadyStanceActive() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->IsCombatReadyStanceActive();
}

bool ACRPGBaseCharacter::IsExplorationStanceActive() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->IsExplorationStanceActive();
}

bool ACRPGBaseCharacter::UsesUnarmedStyle() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesUnarmedStyle();
}

bool ACRPGBaseCharacter::UsesBowStyle() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesBowStyle();
}

bool ACRPGBaseCharacter::UsesTwoHandedStyle() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesTwoHandedStyle();
}

bool ACRPGBaseCharacter::UsesShieldStyle() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesShieldStyle();
}

bool ACRPGBaseCharacter::UsesTwoWeaponsStyle() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesTwoWeaponsStyle();
}

bool ACRPGBaseCharacter::UsesFightIdleProfile() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesFightIdleProfile();
}

bool ACRPGBaseCharacter::UsesNoFightIdleProfile() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesNoFightIdleProfile();
}

bool ACRPGBaseCharacter::UsesFightLocomotionProfile() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesFightLocomotionProfile();
}

bool ACRPGBaseCharacter::UsesNoFightLocomotionProfile() const
{
    return CombatLoadoutComponent && CombatLoadoutComponent->UsesNoFightLocomotionProfile();
}

void ACRPGBaseCharacter::ApplyCombatLoadoutVisuals()
{
    ClearCombatLoadoutVisuals();

    if (!CombatLoadoutComponent)
    {
        return;
    }

    const FCombatStyleLoadout CurrentLoadout = CombatLoadoutComponent->GetCurrentLoadout();
    for (const FCombatEquippedVariant &Variant : CurrentLoadout.EquippedVariants)
    {
        ApplyEquippedVariantVisual(Variant);
    }
}

void ACRPGBaseCharacter::ClearCombatLoadoutVisuals()
{
    ClearEquipmentSlotVisual(ECombatEquipmentSlot::MainHand);
    ClearEquipmentSlotVisual(ECombatEquipmentSlot::OffHand);
    ClearEquipmentSlotVisual(ECombatEquipmentSlot::TwoHanded);
    ClearEquipmentSlotVisual(ECombatEquipmentSlot::Ranged);
}

void ACRPGBaseCharacter::InitializeEquipmentVisualComponent(UMeshComponent *MeshComponent) const
{
    if (!MeshComponent)
    {
        return;
    }

    MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MeshComponent->SetGenerateOverlapEvents(false);
    MeshComponent->SetCastShadow(false);
    MeshComponent->SetHiddenInGame(true);
}

void ACRPGBaseCharacter::ApplyEquippedVariantVisual(const FCombatEquippedVariant &Variant)
{
    USkeletalMeshComponent *CharacterMesh = GetMesh();
    if (!CharacterMesh)
    {
        return;
    }

    UStaticMeshComponent *StaticVisualComponent = GetEquipmentStaticMeshComponent(Variant.Slot);
    USkeletalMeshComponent *SkeletalVisualComponent = GetEquipmentSkeletalMeshComponent(Variant.Slot);
    const FName SocketName = Variant.EquippedSocketName != NAME_None ? Variant.EquippedSocketName : GetDefaultEquipmentSocketName(Variant.Slot);

    if (StaticVisualComponent)
    {
        StaticVisualComponent->SetStaticMesh(nullptr);
        StaticVisualComponent->SetHiddenInGame(true);
    }

    if (SkeletalVisualComponent)
    {
        SkeletalVisualComponent->SetSkeletalMesh(nullptr);
        SkeletalVisualComponent->SetHiddenInGame(true);
    }

    if (Variant.StaticMesh && StaticVisualComponent)
    {
        StaticVisualComponent->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
        StaticVisualComponent->SetRelativeTransform(Variant.RelativeTransform);
        StaticVisualComponent->SetStaticMesh(Variant.StaticMesh);
        StaticVisualComponent->SetHiddenInGame(false);
        return;
    }

    if (Variant.SkeletalMesh && SkeletalVisualComponent)
    {
        SkeletalVisualComponent->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
        SkeletalVisualComponent->SetRelativeTransform(Variant.RelativeTransform);
        SkeletalVisualComponent->SetSkeletalMesh(Variant.SkeletalMesh);
        SkeletalVisualComponent->SetHiddenInGame(false);
    }
}

void ACRPGBaseCharacter::ClearEquipmentSlotVisual(ECombatEquipmentSlot Slot)
{
    if (UStaticMeshComponent *StaticVisualComponent = GetEquipmentStaticMeshComponent(Slot))
    {
        StaticVisualComponent->SetStaticMesh(nullptr);
        StaticVisualComponent->SetHiddenInGame(true);
    }

    if (USkeletalMeshComponent *SkeletalVisualComponent = GetEquipmentSkeletalMeshComponent(Slot))
    {
        SkeletalVisualComponent->SetSkeletalMesh(nullptr);
        SkeletalVisualComponent->SetHiddenInGame(true);
    }
}

FName ACRPGBaseCharacter::GetDefaultEquipmentSocketName(ECombatEquipmentSlot Slot) const
{
    switch (Slot)
    {
    case ECombatEquipmentSlot::MainHand:
        return DefaultMainHandEquipmentSocketName;
    case ECombatEquipmentSlot::OffHand:
        return DefaultOffHandEquipmentSocketName;
    case ECombatEquipmentSlot::TwoHanded:
        return DefaultTwoHandedEquipmentSocketName;
    case ECombatEquipmentSlot::Ranged:
        return DefaultRangedEquipmentSocketName;
    default:
        return NAME_None;
    }
}

UStaticMeshComponent *ACRPGBaseCharacter::GetEquipmentStaticMeshComponent(ECombatEquipmentSlot Slot) const
{
    switch (Slot)
    {
    case ECombatEquipmentSlot::MainHand:
        return MainHandEquipmentStaticMesh;
    case ECombatEquipmentSlot::OffHand:
        return OffHandEquipmentStaticMesh;
    case ECombatEquipmentSlot::TwoHanded:
        return TwoHandedEquipmentStaticMesh;
    case ECombatEquipmentSlot::Ranged:
        return RangedEquipmentStaticMesh;
    default:
        return nullptr;
    }
}

USkeletalMeshComponent *ACRPGBaseCharacter::GetEquipmentSkeletalMeshComponent(ECombatEquipmentSlot Slot) const
{
    switch (Slot)
    {
    case ECombatEquipmentSlot::MainHand:
        return MainHandEquipmentSkeletalMesh;
    case ECombatEquipmentSlot::OffHand:
        return OffHandEquipmentSkeletalMesh;
    case ECombatEquipmentSlot::TwoHanded:
        return TwoHandedEquipmentSkeletalMesh;
    case ECombatEquipmentSlot::Ranged:
        return RangedEquipmentSkeletalMesh;
    default:
        return nullptr;
    }
}
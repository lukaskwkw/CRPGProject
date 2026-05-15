#include "CRPGBaseCharacter.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Combat/Types/CombatTypes.h"
#include "Components/CapsuleComponent.h"
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
    TacticalOccupancyNavigationBlocker = CreateDefaultSubobject<UCapsuleComponent>(TEXT("TacticalOccupancyNavigationBlocker"));
    CombatFeedbackText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CombatFeedbackText"));

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
    return PlayConfiguredMontage(MeleeAttackMontage);
}

float ACRPGBaseCharacter::PlayRangedAttackMontage()
{
    return PlayConfiguredMontage(RangedAttackMontage);
}

float ACRPGBaseCharacter::PlayDodgeMontage()
{
    return PlayConfiguredMontage(DodgeMontage);
}

float ACRPGBaseCharacter::PlayHitReactMontage()
{
    return PlayConfiguredMontage(HitReactMontage);
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

    if (PlayDeathMontage() <= 0.0f)
    {
        EnableCombatRagdoll();
    }
}

float ACRPGBaseCharacter::PlayDeathMontage()
{
    return PlayConfiguredMontage(DeathMontage);
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
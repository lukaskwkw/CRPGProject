#include "CRPGBaseCharacter.h"
#include "AbilitySystemComponent.h"
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
    PrimaryActorTick.bCanEverTick = false;

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

void ACRPGBaseCharacter::BeginPlay()
{
    Super::BeginPlay();

    if (CombatFeedbackText)
    {
        const float CapsuleHalfHeight = GetCapsuleComponent() ? GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight() : 88.0f;
        CombatFeedbackText->SetRelativeLocation(FVector(0.0f, 0.0f, CapsuleHalfHeight + CombatFeedbackHeightOffsetCm));
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

void ACRPGBaseCharacter::EnterTacticalDeathState()
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
        CharacterCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

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
    if (USkeletalMeshComponent *CharacterMesh = GetMesh())
    {
        // This only marks the mesh for custom-depth based highlighting. The actual outline/fill look still depends
        // on the project's post-process setup, which can now key off this stencil value.
        CharacterMesh->SetRenderCustomDepth(bEnabled);
        CharacterMesh->SetCustomDepthStencilValue(CombatTargetHighlightStencilValue);
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
    CombatFeedbackText->SetTextRenderColor(Color);
    CombatFeedbackText->SetHiddenInGame(false);

    GetWorldTimerManager().ClearTimer(CombatFeedbackHideTimerHandle);
    if (CombatFeedbackDurationSeconds <= 0.0f)
    {
        return;
    }

    GetWorldTimerManager().SetTimer(CombatFeedbackHideTimerHandle, this, &ACRPGBaseCharacter::HideCombatFeedbackText, CombatFeedbackDurationSeconds, false);
}

void ACRPGBaseCharacter::HideCombatFeedbackText()
{
    if (CombatFeedbackText)
    {
        CombatFeedbackText->SetHiddenInGame(true);
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
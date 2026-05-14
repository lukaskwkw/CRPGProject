#include "TacticalOutlineOverlayComponent.h"

#include "Engine/GameInstance.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Interfaces/TacticalOutlineInteractable.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "World/Actors/OutlineInteractableActor.h"
#include "Components/CapsuleComponent.h"
#include "Engine/EngineTypes.h"

namespace TacticalOutlineOverlayEvents
{
    static const FString TacticalTurnEnded = TEXT("tactical_turn_ended");
    static const FString TacticalActiveUnitChanged = TEXT("tactical_active_unit_changed");
}

namespace
{
    ECRPGOutlineCategory ResolveUnitOutlineCategory(const ACRPGBaseCharacter *UnitCharacter, const UTacticalUnitComponent *UnitComponent, const UTacticalUnitComponent *ReferenceUnit, bool bIsActiveUnit)
    {
        if (!UnitComponent || !UnitComponent->IsAlive())
        {
            return ECRPGOutlineCategory::None;
        }

        if (UnitCharacter)
        {
            const ECRPGOutlineCategory AuthoredCategory = UnitCharacter->GetOutlineCategory();
            return AuthoredCategory;
        }

        return ECRPGOutlineCategory::None;
    }
}

UTacticalOutlineOverlayComponent::UTacticalOutlineOverlayComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTacticalOutlineOverlayComponent::BeginPlay()
{
    Super::BeginPlay();
    OwningController = Cast<ACRPGProjectPlayerController>(GetOwner());
}

void UTacticalOutlineOverlayComponent::HandleGameEvent(const FString &EventName, const FString &Payload)
{
    if (EventName == TacticalOutlineOverlayEvents::TacticalTurnEnded || EventName == TacticalOutlineOverlayEvents::TacticalActiveUnitChanged)
    {
        RefreshWorldOutlineOverlay();
    }

    (void)Payload;
}

void UTacticalOutlineOverlayComponent::UpdateHoveredInteractableFromCursor()
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller || !Controller->IsLocalPlayerController() || Controller->IsPointerOverTacticalHUD())
    {
        ClearHoveredInteractable();
        return;
    }

    SetHoveredInteractableActor(ResolveInteractableUnderCursor());
}

void UTacticalOutlineOverlayComponent::ClearHoveredInteractable()
{
    SetHoveredInteractableActor(nullptr);
}

bool UTacticalOutlineOverlayComponent::TryGetHoveredInteractablePreviewDestination(FVector &OutDestination) const
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    const AOutlineInteractableActor *HoveredActor = HoveredInteractableActor.Get();
    const ACRPGBaseCharacter *SelectedCharacter = Controller ? Cast<ACRPGBaseCharacter>(Controller->GetPawn()) : nullptr;
    const UCapsuleComponent *SelectedCapsule = SelectedCharacter ? SelectedCharacter->GetCapsuleComponent() : nullptr;
    if (!Controller || !HoveredActor || !SelectedCharacter)
    {
        return false;
    }

    FVector ApproachDirection = SelectedCharacter->GetActorLocation() - HoveredActor->GetInteractableTargetLocation();
    ApproachDirection.Z = 0.0f;
    if (!ApproachDirection.Normalize())
    {
        return false;
    }

    const float InteractorRadiusCm = SelectedCapsule ? SelectedCapsule->GetUnscaledCapsuleRadius() : 55.0f;
    const float DesiredCenterDistanceCm = FMath::Max(0.0f, InteractorRadiusCm + HoveredActor->GetInteractableBoundsRadiusCm() + HoveredActor->GetInteractableDistanceCm());
    OutDestination = HoveredActor->GetInteractableTargetLocation() + (ApproachDirection * DesiredCenterDistanceCm);
    return true;
}

AOutlineInteractableActor *UTacticalOutlineOverlayComponent::GetHoveredInteractableActor() const
{
    return HoveredInteractableActor.Get();
}

void UTacticalOutlineOverlayComponent::HandleUnitOutlineOverlayPressed()
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller || !Controller->IsLocalPlayerController())
    {
        return;
    }

    bUnitOutlineOverlayHeld = true;
    RefreshWorldOutlineOverlay();
}

void UTacticalOutlineOverlayComponent::HandleUnitOutlineOverlayReleased()
{
    bUnitOutlineOverlayHeld = false;
    RefreshWorldOutlineOverlay();
}

void UTacticalOutlineOverlayComponent::HandleInteractableOutlineOverlayPressed()
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller || !Controller->IsLocalPlayerController())
    {
        return;
    }

    bInteractableOutlineOverlayHeld = true;
    RefreshWorldOutlineOverlay();
}

void UTacticalOutlineOverlayComponent::HandleInteractableOutlineOverlayReleased()
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    bInteractableOutlineOverlayHeld = Controller && (Controller->IsInputKeyDown(EKeys::LeftAlt) || Controller->IsInputKeyDown(EKeys::RightAlt));
    RefreshWorldOutlineOverlay();
}

void UTacticalOutlineOverlayComponent::RefreshWorldOutlineOverlay() const
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    if (!Controller || !TacticalTurnSubsystem)
    {
        return;
    }

    const bool bShowUnitOutlineOverlay = Controller->IsLocalPlayerController() && bUnitOutlineOverlayHeld && !bInteractableOutlineOverlayHeld;
    const ACRPGBaseCharacter *SelectedCharacter = Cast<ACRPGBaseCharacter>(Controller->GetPawn());
    const UTacticalUnitComponent *SelectedUnit = SelectedCharacter ? SelectedCharacter->GetTacticalUnitComponent() : nullptr;
    const bool bIsTurnModeActive = TacticalTurnSubsystem->IsTurnModeActive();
    const ACRPGBaseCharacter *ActiveCharacter = bIsTurnModeActive ? TacticalTurnSubsystem->GetActiveUnit() : SelectedCharacter;
    const UTacticalUnitComponent *ReferenceUnit = bIsTurnModeActive ? TacticalTurnSubsystem->GetCurrentActiveUnit() : SelectedUnit;

    for (const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit : TacticalTurnSubsystem->GetRegisteredUnits())
    {
        ACRPGBaseCharacter *UnitCharacter = RegisteredUnit.Get();
        if (!IsValid(UnitCharacter))
        {
            continue;
        }

        UTacticalUnitComponent *UnitComponent = UnitCharacter->GetTacticalUnitComponent();
        const bool bIsActiveUnit = UnitCharacter == ActiveCharacter;
        const ECRPGOutlineCategory NewCategory = bShowUnitOutlineOverlay
                                                     ? ResolveUnitOutlineCategory(UnitCharacter, UnitComponent, ReferenceUnit, bIsActiveUnit)
                                                     : ECRPGOutlineCategory::None;
        UnitCharacter->SetRuntimeOutlineCategory(NewCategory);
    }

    RefreshInteractableOutlineOverlay(Controller->IsLocalPlayerController() && bInteractableOutlineOverlayHeld);
}

void UTacticalOutlineOverlayComponent::RefreshInteractableOutlineOverlay(bool bEnableInteractableOverlay) const
{
    UWorld *World = GetWorld();
    if (!World)
    {
        return;
    }

    TArray<AActor *> InteractableActors;
    UGameplayStatics::GetAllActorsWithInterface(World, UTacticalOutlineInteractable::StaticClass(), InteractableActors);
    for (AActor *InteractableActor : InteractableActors)
    {
        if (!IsValid(InteractableActor) || !InteractableActor->GetClass()->ImplementsInterface(UTacticalOutlineInteractable::StaticClass()))
        {
            continue;
        }

        if (ITacticalOutlineInteractable::Execute_IsInteractableOutlineExcluded(InteractableActor))
        {
            ITacticalOutlineInteractable::Execute_SetInteractableOutlineEnabled(InteractableActor, false);
            continue;
        }

        ITacticalOutlineInteractable::Execute_SetInteractableOutlineEnabled(InteractableActor, bEnableInteractableOverlay);
    }
}

ACRPGProjectPlayerController *UTacticalOutlineOverlayComponent::GetOwnerController() const
{
    return OwningController.Get();
}

UTacticalTurnSubsystem *UTacticalOutlineOverlayComponent::GetTacticalTurnSubsystem() const
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    UGameInstance *GameInstance = Controller ? Controller->GetGameInstance() : nullptr;
    return GameInstance ? GameInstance->GetSubsystem<UTacticalTurnSubsystem>() : nullptr;
}

AOutlineInteractableActor *UTacticalOutlineOverlayComponent::ResolveInteractableUnderCursor() const
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller)
    {
        return nullptr;
    }

    FHitResult HitResult;
    if (!Controller->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult) || !HitResult.bBlockingHit)
    {
        return nullptr;
    }

    AActor *HitActor = HitResult.GetActor();
    if (!HitActor && HitResult.Component.IsValid())
    {
        HitActor = HitResult.Component->GetOwner();
    }

    return Cast<AOutlineInteractableActor>(HitActor);
}

void UTacticalOutlineOverlayComponent::SetHoveredInteractableActor(AOutlineInteractableActor *NewInteractableActor)
{
    AOutlineInteractableActor *PreviousActor = HoveredInteractableActor.Get();
    if (PreviousActor == NewInteractableActor)
    {
        return;
    }

    if (IsValid(PreviousActor))
    {
        PreviousActor->SetInteractableHoverEnabled(false);
    }

    HoveredInteractableActor = NewInteractableActor;

    if (IsValid(NewInteractableActor) && !NewInteractableActor->IsInteractableOutlineExcluded_Implementation())
    {
        NewInteractableActor->SetInteractableHoverEnabled(true);
    }
}
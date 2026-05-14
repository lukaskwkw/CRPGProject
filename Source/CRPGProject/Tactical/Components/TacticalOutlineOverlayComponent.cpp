#include "TacticalOutlineOverlayComponent.h"

#include "Engine/GameInstance.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Interfaces/TacticalOutlineInteractable.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

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
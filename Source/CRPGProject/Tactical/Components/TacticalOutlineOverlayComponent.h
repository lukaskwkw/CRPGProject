#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacticalOutlineOverlayComponent.generated.h"

class ACRPGProjectPlayerController;
class UTacticalTurnSubsystem;

UCLASS(ClassGroup = (Tactical), meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UTacticalOutlineOverlayComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTacticalOutlineOverlayComponent();

    virtual void BeginPlay() override;

    void HandleGameEvent(const FString &EventName, const FString &Payload);
    void HandleUnitOutlineOverlayPressed();
    void HandleUnitOutlineOverlayReleased();
    void HandleInteractableOutlineOverlayPressed();
    void HandleInteractableOutlineOverlayReleased();
    void RefreshWorldOutlineOverlay() const;

private:
    ACRPGProjectPlayerController *GetOwnerController() const;
    UTacticalTurnSubsystem *GetTacticalTurnSubsystem() const;
    void RefreshInteractableOutlineOverlay(bool bEnableInteractableOverlay) const;

private:
    UPROPERTY(Transient)
    TObjectPtr<ACRPGProjectPlayerController> OwningController;

    UPROPERTY(Transient)
    bool bUnitOutlineOverlayHeld = false;

    UPROPERTY(Transient)
    bool bInteractableOutlineOverlayHeld = false;
};
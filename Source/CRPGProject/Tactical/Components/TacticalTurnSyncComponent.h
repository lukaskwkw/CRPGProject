#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacticalTurnSyncComponent.generated.h"

class ACRPGProjectPlayerController;
class APawn;

UCLASS(ClassGroup = (Tactical), meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UTacticalTurnSyncComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTacticalTurnSyncComponent();

    virtual void BeginPlay() override;

    void HandleGameEvent(const FString &EventName, const FString &Payload);
    void SyncPossessionToActiveTacticalUnit();
    void RestorePossessionAfterTacticalTurn();
    bool IsTurnModeMovementEnabled() const;
    void SetTurnModeMovementEnabled(bool bEnabled);
    void ToggleTurnModeMovementEnabled();

private:
    ACRPGProjectPlayerController *GetOwnerController() const;
    bool CanSelectedUnitUseTurnControls() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<ACRPGProjectPlayerController> OwningController;

    UPROPERTY(Transient)
    bool bUserTurnModeMovementEnabled = true;

    UPROPERTY(Transient)
    TWeakObjectPtr<APawn> ExplorationPawnBeforeTacticalTurn;
};
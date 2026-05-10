#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TacticalTurnSubsystem.generated.h"

class ACRPGBaseCharacter;
class UCameraModeSubsystem;
class UEventBusSubsystem;
class UTacticalUnitComponent;

UENUM(BlueprintType)
enum class ETacticalTurnState : uint8
{
    Disabled UMETA(DisplayName = "Disabled"),
    TacticalPaused UMETA(DisplayName = "Tactical Paused"),
    ResolvingTurn UMETA(DisplayName = "Resolving Turn")
};

UCLASS()
class CRPGPROJECT_API UTacticalTurnSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void ToggleTurnMode();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void StartTurnMode();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void EndTurnMode();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void AdvanceRound();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void EndCurrentUnitTurn();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void RegisterUnit(ACRPGBaseCharacter *Unit);

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void UnregisterUnit(ACRPGBaseCharacter *Unit);

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    ETacticalTurnState GetCurrentState() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    bool IsTurnModeActive() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    ACRPGBaseCharacter *GetActiveUnit() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    const TArray<UTacticalUnitComponent *> &GetInitiativeOrder() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    UTacticalUnitComponent *GetCurrentActiveUnit() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    int32 GetCurrentRound() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    bool IsEncounterRunning() const;

private:
    ACRPGBaseCharacter *ResolveCurrentlyPossessedUnit() const;
    void PublishEvent(const FString &EventName, const FString &Payload) const;
    void RefreshRegisteredUnitNavigationBlockers() const;
    void CacheSubsystemDependencies();
    void SortInitiativeOrder();
    void ResetInitiativeUnitsForNewRound();
    void RefreshActiveUnitFromInitiative();
    void BroadcastActiveUnitChanged() const;
    int32 FindNextAliveInitiativeIndex(int32 StartIndex) const;

private:
    UPROPERTY()
    bool bIsTurnModeActive = false;

    UPROPERTY()
    bool bEncounterRunning = false;

    UPROPERTY()
    ETacticalTurnState CurrentState = ETacticalTurnState::Disabled;

    UPROPERTY()
    int32 CurrentRound = 0;

    UPROPERTY()
    TWeakObjectPtr<ACRPGBaseCharacter> ActiveUnit;

    UPROPERTY()
    TArray<UTacticalUnitComponent *> InitiativeOrder;

    UPROPERTY()
    int32 ActiveInitiativeIndex = INDEX_NONE;

    UPROPERTY()
    TArray<TWeakObjectPtr<ACRPGBaseCharacter>> RegisteredUnits;

    UPROPERTY()
    TObjectPtr<UCameraModeSubsystem> CameraModeSubsystem = nullptr;

    UPROPERTY()
    TObjectPtr<UEventBusSubsystem> EventBusSubsystem = nullptr;
};

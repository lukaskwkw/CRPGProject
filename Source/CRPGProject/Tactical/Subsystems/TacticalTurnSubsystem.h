#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TacticalTurnSubsystem.generated.h"

class ACRPGBaseCharacter;
class UCameraModeSubsystem;
class UEventBusSubsystem;

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
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
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
    void RegisterUnit(ACRPGBaseCharacter* Unit);

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    void UnregisterUnit(ACRPGBaseCharacter* Unit);

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    ETacticalTurnState GetCurrentState() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    bool IsTurnModeActive() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    ACRPGBaseCharacter* GetActiveUnit() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    int32 GetCurrentRound() const;

private:
    ACRPGBaseCharacter* ResolveCurrentlyPossessedUnit() const;
    void PublishEvent(const FString& EventName, const FString& Payload) const;
    void ApplyActiveUnitTimeCompensation();
    void ClearActiveUnitTimeCompensation();
    void CacheSubsystemDependencies();

private:
    UPROPERTY()
    bool bIsTurnModeActive = false;

    UPROPERTY()
    ETacticalTurnState CurrentState = ETacticalTurnState::Disabled;

    UPROPERTY()
    int32 CurrentRound = 0;

    UPROPERTY()
    TWeakObjectPtr<ACRPGBaseCharacter> ActiveUnit;

    UPROPERTY()
    TArray<TWeakObjectPtr<ACRPGBaseCharacter>> RegisteredUnits;

    UPROPERTY()
    TObjectPtr<UCameraModeSubsystem> CameraModeSubsystem = nullptr;

    UPROPERTY()
    TObjectPtr<UEventBusSubsystem> EventBusSubsystem = nullptr;
};

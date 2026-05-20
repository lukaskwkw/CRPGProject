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
    /** Caches dependent subsystems required for tactical turn orchestration. */
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    /** Shuts down tactical turn state and releases cached subsystem references. */
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    /** Toggles tactical turn mode on or off. */
    void ToggleTurnMode();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    /** Starts turn mode, builds initiative order, and picks the first active unit. */
    void StartTurnMode();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    /** Ends turn mode and clears encounter state. */
    void EndTurnMode();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    /** Advances encounter flow to the next unit, wrapping to a new round when needed. */
    void AdvanceRound();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    /** Finishes the active unit turn and resolves the next valid initiative entry. */
    void EndCurrentUnitTurn();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    /** Adds a character to the set of units considered by tactical turn mode. */
    void RegisterUnit(ACRPGBaseCharacter *Unit);

    UFUNCTION(BlueprintCallable, Category = "Tactical|Turn")
    /** Removes a character from tactical tracking and encounter ordering. */
    void UnregisterUnit(ACRPGBaseCharacter *Unit);

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    /** Returns the current tactical turn state enum. */
    ETacticalTurnState GetCurrentState() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    /** Returns whether tactical turn mode is currently active. */
    bool IsTurnModeActive() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    /** Returns the character whose turn is currently active. */
    ACRPGBaseCharacter *GetActiveUnit() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    /** Returns the current initiative order snapshot used by the HUD. */
    const TArray<UTacticalUnitComponent *> &GetInitiativeOrder() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    /** Returns the active unit component for the current initiative entry. */
    UTacticalUnitComponent *GetCurrentActiveUnit() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    /** Returns the one-based round counter for the active encounter. */
    int32 GetCurrentRound() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Turn")
    /** Returns whether an encounter with at least one active unit is running. */
    bool IsEncounterRunning() const;

    /** Returns all characters registered with tactical turn mode, including inactive weak refs until pruned. */
    const TArray<TWeakObjectPtr<ACRPGBaseCharacter>> &GetRegisteredUnits() const;

private:
    /** Uses current possession to seed the initiative list with the player's active character. */
    ACRPGBaseCharacter *ResolveCurrentlyPossessedUnit() const;
    /** Publishes a tactical lifecycle event through the EventBus subsystem. */
    void PublishEvent(const FString &EventName, const FString &Payload) const;
    /** Refreshes per-character navigation blockers to match the latest active-unit state. */
    void RefreshRegisteredUnitNavigationBlockers() const;
    /** Keeps character animation/loadout stance aligned with tactical turn mode activation. */
    void RefreshRegisteredUnitCombatStances() const;
    /** Resolves cached subsystem dependencies lazily after startup or world changes. */
    void CacheSubsystemDependencies();
    /** Sorts initiative entries from highest initiative to lowest. */
    void SortInitiativeOrder();
    /** Restores movement/action resources for all initiative participants at round start. */
    void ResetInitiativeUnitsForNewRound();
    /** Updates ActiveUnit from the current initiative index. */
    void RefreshActiveUnitFromInitiative();
    /** Broadcasts that the active initiative entry changed so UI/controller layers can react. */
    void BroadcastActiveUnitChanged() const;
    /** Finds the next living initiative entry starting at the supplied index. */
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

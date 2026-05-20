#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Combat/Types/CombatTypes.h"
#include "TacticalEnemyTurnExecutorSubsystem.generated.h"

class ACRPGBaseCharacter;
class ACRPGProjectPlayerController;
class UCombatExecutionSubsystem;
class UEventBusSubsystem;
class UTacticalMovementControllerComponent;
class UTacticalTurnSubsystem;
class UTacticalUnitComponent;

UCLASS()
class CRPGPROJECT_API UTacticalEnemyTurnExecutorSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintPure, Category = "Tactical|AI")
    UTacticalUnitComponent *FindNearestPlayerUnit(UTacticalUnitComponent *EnemyUnit) const;

private:
    void HandleGameEvent(const FString &EventName, const FString &Payload);
    void CacheSubsystemDependencies();
    void ResetExecutionState();
    void ScheduleEnemyTurnForActiveUnit();
    void BeginEnemyTurnExecution();
    void ContinueEnemyTurnExecution();
    void HandleTraversalCompleted(ACRPGBaseCharacter *TraversedUnit);
    void PollAttackResolution();
    void CompleteEnemyTurn();
    bool ShouldAutomateUnit(const ACRPGBaseCharacter *Unit) const;
    const UTacticalUnitComponent *GetLocalPlayerReferenceUnit() const;
    bool TryStartMoveTowardTarget(ACRPGBaseCharacter *EnemyCharacter, UTacticalUnitComponent *EnemyUnit, UTacticalUnitComponent *TargetUnit, ECombatActionType ActionType);
    bool TryStartAttack(UTacticalUnitComponent *EnemyUnit, UTacticalUnitComponent *TargetUnit, ECombatActionType ActionType);
    ACRPGBaseCharacter *GetActiveEnemyUnit() const;
    ACRPGProjectPlayerController *GetOwningPlayerController() const;
    UTacticalMovementControllerComponent *GetMovementController() const;

private:
    UPROPERTY(EditAnywhere, Category = "Tactical|AI", meta = (ClampMin = "0.0", Units = "s"))
    float EnemyTurnDelaySeconds = 0.35f;

    UPROPERTY(EditAnywhere, Category = "Tactical|AI", meta = (ClampMin = "0.01", Units = "s"))
    float AttackResolutionPollIntervalSeconds = 0.05f;

    UPROPERTY()
    TObjectPtr<UTacticalTurnSubsystem> TacticalTurnSubsystem = nullptr;

    UPROPERTY()
    TObjectPtr<UEventBusSubsystem> EventBusSubsystem = nullptr;

    UPROPERTY()
    TObjectPtr<UCombatExecutionSubsystem> CombatExecutionSubsystem = nullptr;

    UPROPERTY(Transient)
    TWeakObjectPtr<ACRPGBaseCharacter> CurrentExecutingUnit;

    UPROPERTY(Transient)
    ECombatActionType PendingActionType = ECombatActionType::None;

    UPROPERTY(Transient)
    bool bWaitingForTraversal = false;

    UPROPERTY(Transient)
    bool bWaitingForAttackResolution = false;

    FTimerHandle EnemyTurnDelayTimerHandle;
    FTimerHandle AttackResolutionPollTimerHandle;
};
#include "Tactical/Subsystems/TacticalEnemyTurnExecutorSubsystem.h"

#include "CRPGProject.h"
#include "Combat/Subsystems/CombatExecutionSubsystem.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "Tactical/Components/TacticalMovementControllerComponent.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Rules/TacticalCombatHelpers.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

namespace TacticalEnemyTurnEvents
{
    static const FString TacticalTurnStarted = TEXT("tactical_turn_started");
    static const FString TacticalRoundStarted = TEXT("tactical_round_started");
    static const FString TacticalActiveUnitChanged = TEXT("tactical_active_unit_changed");
    static const FString TacticalTurnEnded = TEXT("tactical_turn_ended");
}

void UTacticalEnemyTurnExecutorSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Collection.InitializeDependency<UEventBusSubsystem>();
    Collection.InitializeDependency<UTacticalTurnSubsystem>();
    Collection.InitializeDependency<UCombatExecutionSubsystem>();

    Super::Initialize(Collection);
    CacheSubsystemDependencies();

    if (EventBusSubsystem)
    {
        EventBusSubsystem->OnNamedGameEvent.AddUObject(this, &UTacticalEnemyTurnExecutorSubsystem::HandleGameEvent);
        UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] Initialized and subscribed to EventBus"));
    }
    else
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] Initialized without EventBus subsystem"));
    }
}

void UTacticalEnemyTurnExecutorSubsystem::Deinitialize()
{
    ResetExecutionState();

    if (EventBusSubsystem)
    {
        EventBusSubsystem->OnNamedGameEvent.RemoveAll(this);
    }

    CombatExecutionSubsystem = nullptr;
    EventBusSubsystem = nullptr;
    TacticalTurnSubsystem = nullptr;
    Super::Deinitialize();
}

UTacticalUnitComponent *UTacticalEnemyTurnExecutorSubsystem::FindNearestPlayerUnit(UTacticalUnitComponent *EnemyUnit) const
{
    if (!EnemyUnit || !TacticalTurnSubsystem)
    {
        return nullptr;
    }

    UTacticalUnitComponent *ClosestUnit = nullptr;
    float ClosestDistanceSq = TNumericLimits<float>::Max();

    for (const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit : TacticalTurnSubsystem->GetRegisteredUnits())
    {
        const ACRPGBaseCharacter *CandidateCharacter = RegisteredUnit.Get();
        const UTacticalUnitComponent *CandidateUnit = CandidateCharacter ? CandidateCharacter->GetTacticalUnitComponent() : nullptr;
        if (!CandidateUnit || CandidateUnit == EnemyUnit || !CandidateUnit->IsAlive() || CandidateUnit->IsNeutral() || !EnemyUnit->IsEnemyTo(CandidateUnit))
        {
            continue;
        }

        const float CandidateDistanceSq = FVector::DistSquared2D(EnemyUnit->GetOccupiedLocation(), CandidateUnit->GetOccupiedLocation());
        if (CandidateDistanceSq < ClosestDistanceSq)
        {
            ClosestDistanceSq = CandidateDistanceSq;
            ClosestUnit = const_cast<UTacticalUnitComponent *>(CandidateUnit);
        }
    }

    return ClosestUnit;
}

void UTacticalEnemyTurnExecutorSubsystem::HandleGameEvent(const FString &EventName, const FString &Payload)
{
    if (EventName == TacticalEnemyTurnEvents::TacticalTurnEnded)
    {
        UE_LOG(LogCRPGProject, Verbose, TEXT("[EnemyTurnAI] Tactical turn ended, resetting state"));
        ResetExecutionState();
        return;
    }

    if (EventName == TacticalEnemyTurnEvents::TacticalTurnStarted || EventName == TacticalEnemyTurnEvents::TacticalRoundStarted || EventName == TacticalEnemyTurnEvents::TacticalActiveUnitChanged)
    {
        UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] Received event %s with payload: %s"), *EventName, *Payload);
        ResetExecutionState();
        ScheduleEnemyTurnForActiveUnit();
    }

    (void)Payload;
}

void UTacticalEnemyTurnExecutorSubsystem::CacheSubsystemDependencies()
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        TacticalTurnSubsystem = GameInstance->GetSubsystem<UTacticalTurnSubsystem>();
        EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>();
        CombatExecutionSubsystem = GameInstance->GetSubsystem<UCombatExecutionSubsystem>();
    }
}

void UTacticalEnemyTurnExecutorSubsystem::ResetExecutionState()
{
    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EnemyTurnDelayTimerHandle);
        World->GetTimerManager().ClearTimer(AttackResolutionPollTimerHandle);
    }

    CurrentExecutingUnit.Reset();
    PendingActionType = ECombatActionType::None;
    bWaitingForTraversal = false;
    bWaitingForAttackResolution = false;
}

void UTacticalEnemyTurnExecutorSubsystem::ScheduleEnemyTurnForActiveUnit()
{
    ACRPGBaseCharacter *ActiveEnemy = GetActiveEnemyUnit();
    if (!ActiveEnemy)
    {
        UE_LOG(LogCRPGProject, Verbose, TEXT("[EnemyTurnAI] Active unit is not an automated enemy"));
        return;
    }

    if (UWorld *World = GetWorld())
    {
        UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] Scheduling enemy turn for %s after %.2fs"), *ActiveEnemy->GetName(), EnemyTurnDelaySeconds);
        World->GetTimerManager().SetTimer(
            EnemyTurnDelayTimerHandle,
            this,
            &UTacticalEnemyTurnExecutorSubsystem::BeginEnemyTurnExecution,
            FMath::Max(0.0f, EnemyTurnDelaySeconds),
            false);
    }
}

void UTacticalEnemyTurnExecutorSubsystem::BeginEnemyTurnExecution()
{
    ACRPGBaseCharacter *ActiveEnemy = GetActiveEnemyUnit();
    if (!ActiveEnemy)
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] BeginEnemyTurnExecution aborted: no active automated enemy"));
        return;
    }

    CurrentExecutingUnit = ActiveEnemy;
    PendingActionType = ECombatActionType::None;
    bWaitingForTraversal = false;
    bWaitingForAttackResolution = false;

    if (ACRPGProjectPlayerController *PlayerController = GetOwningPlayerController())
    {
        PlayerController->FocusCameraOnTacticalUnit(ActiveEnemy);
    }

    UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] Starting execution for %s"), *ActiveEnemy->GetName());

    ContinueEnemyTurnExecution();
}

void UTacticalEnemyTurnExecutorSubsystem::ContinueEnemyTurnExecution()
{
    ACRPGBaseCharacter *EnemyCharacter = GetActiveEnemyUnit();
    UTacticalUnitComponent *EnemyUnit = EnemyCharacter ? EnemyCharacter->GetTacticalUnitComponent() : nullptr;
    if (!EnemyCharacter || !EnemyUnit)
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] Continue aborted: active enemy or unit component missing"));
        return;
    }

    CurrentExecutingUnit = EnemyCharacter;

    if (!EnemyUnit->IsAlive() || !EnemyUnit->CanAct() || !EnemyUnit->HasAnyActionPoints())
    {
        UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] %s cannot act, ending turn"), *EnemyCharacter->GetName());
        CompleteEnemyTurn();
        return;
    }

    UTacticalUnitComponent *TargetUnit = FindNearestPlayerUnit(EnemyUnit);
    if (!TargetUnit)
    {
        UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] %s found no hostile target, ending turn"), *EnemyCharacter->GetName());
        CompleteEnemyTurn();
        return;
    }

    const ECombatActionType PreferredAction = TacticalCombatHelpers::GetPreferredCombatActionForStyle(EnemyCharacter->GetCombatStyleCategory());
    PendingActionType = PreferredAction;
    UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] %s targeting %s with action %d"), *EnemyCharacter->GetName(), *GetNameSafe(TargetUnit->GetOwner()), static_cast<int32>(PreferredAction));

    if (TacticalCombatHelpers::IsTargetInCombatRange(EnemyUnit, TargetUnit, PreferredAction))
    {
        if (!TryStartAttack(EnemyUnit, TargetUnit, PreferredAction))
        {
            CompleteEnemyTurn();
            return;
        }

        if (!bWaitingForAttackResolution)
        {
            CompleteEnemyTurn();
        }

        return;
    }

    if (!TryStartMoveTowardTarget(EnemyCharacter, EnemyUnit, TargetUnit, PreferredAction))
    {
        CompleteEnemyTurn();
    }
}

void UTacticalEnemyTurnExecutorSubsystem::HandleTraversalCompleted(ACRPGBaseCharacter *TraversedUnit)
{
    bWaitingForTraversal = false;

    if (!TraversedUnit || TraversedUnit != CurrentExecutingUnit.Get())
    {
        return;
    }

    ContinueEnemyTurnExecution();
}

void UTacticalEnemyTurnExecutorSubsystem::PollAttackResolution()
{
    ACRPGBaseCharacter *EnemyCharacter = CurrentExecutingUnit.Get();
    UTacticalUnitComponent *EnemyUnit = EnemyCharacter ? EnemyCharacter->GetTacticalUnitComponent() : nullptr;
    if (!EnemyCharacter || !EnemyUnit || !CombatExecutionSubsystem)
    {
        ResetExecutionState();
        return;
    }

    if (CombatExecutionSubsystem->HasPendingAttackForAttacker(EnemyUnit))
    {
        return;
    }

    if (UWorld *World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(AttackResolutionPollTimerHandle);
    }

    bWaitingForAttackResolution = false;
    CompleteEnemyTurn();
}

void UTacticalEnemyTurnExecutorSubsystem::CompleteEnemyTurn()
{
    ACRPGBaseCharacter *EnemyCharacter = CurrentExecutingUnit.Get();
    if (!EnemyCharacter || !TacticalTurnSubsystem || TacticalTurnSubsystem->GetActiveUnit() != EnemyCharacter)
    {
        ResetExecutionState();
        return;
    }

    ResetExecutionState();
    TacticalTurnSubsystem->EndCurrentUnitTurn();
}

bool UTacticalEnemyTurnExecutorSubsystem::ShouldAutomateUnit(const ACRPGBaseCharacter *Unit) const
{
    const UTacticalUnitComponent *UnitComponent = Unit ? Unit->GetTacticalUnitComponent() : nullptr;
    if (!UnitComponent || !UnitComponent->IsAlive() || UnitComponent->IsNeutral())
    {
        return false;
    }

    if (const UTacticalUnitComponent *LocalPlayerUnit = GetLocalPlayerReferenceUnit())
    {
        return UnitComponent != LocalPlayerUnit && UnitComponent->IsEnemyTo(LocalPlayerUnit);
    }

    // Fallback for maps that still author identity primarily through player-controlled flags.
    return !UnitComponent->IsPlayerControlled();
}

const UTacticalUnitComponent *UTacticalEnemyTurnExecutorSubsystem::GetLocalPlayerReferenceUnit() const
{
    if (const ACRPGProjectPlayerController *PlayerController = GetOwningPlayerController())
    {
        if (const ACRPGBaseCharacter *ControlledCharacter = Cast<ACRPGBaseCharacter>(PlayerController->GetPawn()))
        {
            if (const UTacticalUnitComponent *ControlledUnit = ControlledCharacter->GetTacticalUnitComponent())
            {
                return ControlledUnit;
            }
        }
    }

    if (!TacticalTurnSubsystem)
    {
        return nullptr;
    }

    for (const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit : TacticalTurnSubsystem->GetRegisteredUnits())
    {
        const ACRPGBaseCharacter *RegisteredCharacter = RegisteredUnit.Get();
        const UTacticalUnitComponent *RegisteredUnitComponent = RegisteredCharacter ? RegisteredCharacter->GetTacticalUnitComponent() : nullptr;
        if (!RegisteredUnitComponent || !RegisteredUnitComponent->IsAlive() || RegisteredUnitComponent->IsNeutral())
        {
            continue;
        }

        if (RegisteredUnitComponent->IsPlayerControlled())
        {
            return RegisteredUnitComponent;
        }
    }

    return nullptr;
}

bool UTacticalEnemyTurnExecutorSubsystem::TryStartMoveTowardTarget(ACRPGBaseCharacter *EnemyCharacter, UTacticalUnitComponent *EnemyUnit, UTacticalUnitComponent *TargetUnit, ECombatActionType ActionType)
{
    UTacticalMovementControllerComponent *MovementController = GetMovementController();
    if (!EnemyCharacter || !EnemyUnit || !TargetUnit || !MovementController || !EnemyUnit->CanMove() || !EnemyUnit->HasAnyActionPoints())
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] Move start rejected for %s"), *GetNameSafe(EnemyCharacter));
        return false;
    }

    FVector ApproachDestination = FVector::ZeroVector;
    if (!TacticalCombatHelpers::TryGetApproachDestinationInRange(EnemyUnit, TargetUnit, ActionType, 25.0f, ApproachDestination))
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] Could not compute approach destination for %s"), *EnemyCharacter->GetName());
        return false;
    }

    TArray<FVector> TraversalControlPath;
    float CommittedDistanceCm = 0.0f;
    if (!MovementController->TryBuildTraversalToDestinationForUnit(EnemyCharacter, ApproachDestination, TraversalControlPath, CommittedDistanceCm))
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] Could not build traversal for %s toward %s"), *EnemyCharacter->GetName(), *ApproachDestination.ToCompactString());
        return false;
    }

    bWaitingForTraversal = MovementController->StartTraversalForUnit(
        EnemyCharacter,
        TraversalControlPath,
        CommittedDistanceCm,
        FOnTacticalTraversalCompleted::CreateUObject(this, &UTacticalEnemyTurnExecutorSubsystem::HandleTraversalCompleted));
    UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] Traversal %s for %s, path points: %d, committed distance: %.2f"), bWaitingForTraversal ? TEXT("started") : TEXT("failed"), *EnemyCharacter->GetName(), TraversalControlPath.Num(), CommittedDistanceCm);
    return bWaitingForTraversal;
}

bool UTacticalEnemyTurnExecutorSubsystem::TryStartAttack(UTacticalUnitComponent *EnemyUnit, UTacticalUnitComponent *TargetUnit, ECombatActionType ActionType)
{
    if (!EnemyUnit || !TargetUnit || !CombatExecutionSubsystem)
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] Attack start rejected: missing unit or combat subsystem"));
        return false;
    }

    if (ACRPGBaseCharacter *EnemyCharacter = Cast<ACRPGBaseCharacter>(EnemyUnit->GetOwner()))
    {
        FVector LookDirection = TargetUnit->GetOccupiedLocation() - EnemyCharacter->GetActorLocation();
        LookDirection.Z = 0.0f;
        if (!LookDirection.IsNearlyZero())
        {
            EnemyCharacter->SetActorRotation(LookDirection.Rotation());
        }
    }

    if (!CombatExecutionSubsystem->RequestAttackExecution(EnemyUnit, TargetUnit, ActionType, false))
    {
        UE_LOG(LogCRPGProject, Warning, TEXT("[EnemyTurnAI] Attack execution request failed for %s against %s"), *GetNameSafe(EnemyUnit->GetOwner()), *GetNameSafe(TargetUnit->GetOwner()));
        return false;
    }

    UE_LOG(LogCRPGProject, Log, TEXT("[EnemyTurnAI] Attack execution requested for %s against %s"), *GetNameSafe(EnemyUnit->GetOwner()), *GetNameSafe(TargetUnit->GetOwner()));

    if (!CombatExecutionSubsystem->HasPendingAttackForAttacker(EnemyUnit))
    {
        return true;
    }

    if (UWorld *World = GetWorld())
    {
        bWaitingForAttackResolution = true;
        World->GetTimerManager().SetTimer(
            AttackResolutionPollTimerHandle,
            this,
            &UTacticalEnemyTurnExecutorSubsystem::PollAttackResolution,
            FMath::Max(0.01f, AttackResolutionPollIntervalSeconds),
            true);
    }

    return true;
}

ACRPGBaseCharacter *UTacticalEnemyTurnExecutorSubsystem::GetActiveEnemyUnit() const
{
    ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
    return ShouldAutomateUnit(ActiveUnit) ? ActiveUnit : nullptr;
}

ACRPGProjectPlayerController *UTacticalEnemyTurnExecutorSubsystem::GetOwningPlayerController() const
{
    return GetWorld() ? Cast<ACRPGProjectPlayerController>(GetWorld()->GetFirstPlayerController()) : nullptr;
}

UTacticalMovementControllerComponent *UTacticalEnemyTurnExecutorSubsystem::GetMovementController() const
{
    if (ACRPGProjectPlayerController *PlayerController = GetOwningPlayerController())
    {
        return PlayerController->GetTacticalMovementControllerComponent();
    }

    return nullptr;
}
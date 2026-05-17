#include "TacticalTurnSubsystem.h"

#include "Camera/Subsystems/CameraModeSubsystem.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Tactical/Components/TacticalUnitComponent.h"

namespace TacticalTurnEvents
{
    static const FString TacticalTurnStarted = TEXT("tactical_turn_started");
    static const FString TacticalTurnEnded = TEXT("tactical_turn_ended");
    static const FString TacticalEncounterStarted = TEXT("tactical_encounter_started");
    static const FString TacticalRoundStarted = TEXT("tactical_round_started");
    static const FString TacticalActiveUnitChanged = TEXT("tactical_active_unit_changed");
}

void UTacticalTurnSubsystem::Initialize(FSubsystemCollectionBase &Collection)
{
    Super::Initialize(Collection);

    CacheSubsystemDependencies();
}

void UTacticalTurnSubsystem::Deinitialize()
{
    if (bIsTurnModeActive)
    {
        EndTurnMode();
    }

    RegisteredUnits.Reset();
    InitiativeOrder.Reset();
    ActiveUnit.Reset();
    ActiveInitiativeIndex = INDEX_NONE;
    bEncounterRunning = false;
    CameraModeSubsystem = nullptr;
    EventBusSubsystem = nullptr;

    Super::Deinitialize();
}

void UTacticalTurnSubsystem::ToggleTurnMode()
{
    if (bIsTurnModeActive)
    {
        EndTurnMode();
        return;
    }

    StartTurnMode();
}

void UTacticalTurnSubsystem::StartTurnMode()
{
    if (bIsTurnModeActive)
    {
        return;
    }

    CacheSubsystemDependencies();

    bIsTurnModeActive = true;
    CurrentState = ETacticalTurnState::TacticalPaused;
    CurrentRound = 1;
    bEncounterRunning = false;
    ActiveInitiativeIndex = INDEX_NONE;
    InitiativeOrder.Reset();

    if (ACRPGBaseCharacter *ActiveCharacter = ResolveCurrentlyPossessedUnit())
    {
        RegisterUnit(ActiveCharacter);
    }

    // Turn mode can be entered after actors were spawned earlier, so prune weak refs before rebuilding initiative.
    RegisteredUnits.RemoveAll([](const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit)
                              { return !RegisteredUnit.IsValid(); });

    // Initiative order is built from characters, but the tactical data actually lives on their unit components.
    for (const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit : RegisteredUnits)
    {
        if (ACRPGBaseCharacter *Character = RegisteredUnit.Get())
        {
            if (UTacticalUnitComponent *TacticalUnitComponent = Character->GetTacticalUnitComponent())
            {
                if (!TacticalUnitComponent->IsAlive())
                {
                    continue;
                }

                TacticalUnitComponent->SetInitiative(FMath::RandRange(1, 20));
                TacticalUnitComponent->ResetForNewRound();
                InitiativeOrder.AddUnique(TacticalUnitComponent);
            }
        }
    }

    SortInitiativeOrder();
    bEncounterRunning = InitiativeOrder.Num() > 0;
    ActiveInitiativeIndex = bEncounterRunning ? 0 : INDEX_NONE;
    RefreshActiveUnitFromInitiative();
    RefreshRegisteredUnitCombatStances();
    RefreshRegisteredUnitNavigationBlockers();

    PublishEvent(
        TacticalTurnEvents::TacticalTurnStarted,
        FString::Printf(
            TEXT("state=tactical_paused;round=%d;active_unit=%s;registered_units=%d"),
            CurrentRound,
            ActiveUnit.IsValid() ? *ActiveUnit->GetName() : TEXT("none"),
            RegisteredUnits.Num()));

    if (bEncounterRunning)
    {
        PublishEvent(
            TacticalTurnEvents::TacticalEncounterStarted,
            FString::Printf(
                TEXT("round=%d;active_unit=%s;initiative_count=%d"),
                CurrentRound,
                ActiveUnit.IsValid() ? *ActiveUnit->GetName() : TEXT("none"),
                InitiativeOrder.Num()));

        BroadcastActiveUnitChanged();
    }
}

void UTacticalTurnSubsystem::EndTurnMode()
{
    if (!bIsTurnModeActive)
    {
        return;
    }

    const int32 EndingRound = CurrentRound;
    const FString EndingUnitName = ActiveUnit.IsValid() ? ActiveUnit->GetName() : TEXT("none");

    ActiveUnit.Reset();
    InitiativeOrder.Reset();
    ActiveInitiativeIndex = INDEX_NONE;
    CurrentState = ETacticalTurnState::Disabled;
    bIsTurnModeActive = false;
    bEncounterRunning = false;
    CurrentRound = 0;
    RefreshRegisteredUnitCombatStances();
    RefreshRegisteredUnitNavigationBlockers();

    PublishEvent(
        TacticalTurnEvents::TacticalTurnEnded,
        FString::Printf(
            TEXT("round=%d;active_unit=%s;registered_units=%d"),
            EndingRound,
            *EndingUnitName,
            RegisteredUnits.Num()));
}

void UTacticalTurnSubsystem::AdvanceRound()
{
    EndCurrentUnitTurn();
}

void UTacticalTurnSubsystem::EndCurrentUnitTurn()
{
    if (!bIsTurnModeActive)
    {
        return;
    }

    if (!bEncounterRunning)
    {
        RefreshActiveUnitFromInitiative();
        return;
    }

    if (UTacticalUnitComponent *CurrentActiveComponent = GetCurrentActiveUnit())
    {
        CurrentActiveComponent->SetTurnCompleted(true);
    }

    CurrentState = ETacticalTurnState::TacticalPaused;

    // Advance to the next living unit; if none remain, roll into a fresh round and try again from the top.
    int32 NextAliveIndex = FindNextAliveInitiativeIndex(ActiveInitiativeIndex + 1);
    const bool bWrappedRound = NextAliveIndex == INDEX_NONE;

    if (bWrappedRound)
    {
        CurrentRound = FMath::Max(1, CurrentRound + 1);
        ResetInitiativeUnitsForNewRound();
        NextAliveIndex = FindNextAliveInitiativeIndex(0);
    }

    ActiveInitiativeIndex = NextAliveIndex;
    bEncounterRunning = ActiveInitiativeIndex != INDEX_NONE;
    RefreshActiveUnitFromInitiative();
    RefreshRegisteredUnitNavigationBlockers();

    if (bWrappedRound)
    {
        PublishEvent(
            TacticalTurnEvents::TacticalRoundStarted,
            FString::Printf(
                TEXT("round=%d;active_unit=%s"),
                CurrentRound,
                ActiveUnit.IsValid() ? *ActiveUnit->GetName() : TEXT("none")));
    }

    if (bEncounterRunning)
    {
        BroadcastActiveUnitChanged();
    }
}

void UTacticalTurnSubsystem::RegisterUnit(ACRPGBaseCharacter *Unit)
{
    if (!IsValid(Unit))
    {
        return;
    }

    RegisteredUnits.RemoveAll([](const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit)
                              { return !RegisteredUnit.IsValid(); });

    RegisteredUnits.AddUnique(Unit);

    Unit->SetCombatStanceContext(bIsTurnModeActive ? ECombatStanceContext::CombatReady : ECombatStanceContext::Exploration);

    if (!bEncounterRunning)
    {
        return;
    }

    // Late-registered units can join an already running encounter as long as they have a valid tactical component.
    if (UTacticalUnitComponent *TacticalUnitComponent = Unit->GetTacticalUnitComponent())
    {
        if (!TacticalUnitComponent->IsAlive() || InitiativeOrder.Contains(TacticalUnitComponent))
        {
            return;
        }

        TacticalUnitComponent->SetInitiative(FMath::RandRange(1, 20));
        TacticalUnitComponent->ResetForNewRound();
        InitiativeOrder.Add(TacticalUnitComponent);
        SortInitiativeOrder();

        // Newly joined units need their nav blocker state refreshed against the current active unit immediately.
        Unit->UpdateTacticalOccupancyNavigationBlocker(ActiveUnit.Get());

        if (ActiveInitiativeIndex == INDEX_NONE)
        {
            ActiveInitiativeIndex = FindNextAliveInitiativeIndex(0);
            RefreshActiveUnitFromInitiative();
            RefreshRegisteredUnitNavigationBlockers();
            BroadcastActiveUnitChanged();
        }
    }
}

void UTacticalTurnSubsystem::UnregisterUnit(ACRPGBaseCharacter *Unit)
{
    const UTacticalUnitComponent *RemovedComponent = IsValid(Unit) ? Unit->GetTacticalUnitComponent() : nullptr;

    RegisteredUnits.RemoveAll([Unit](const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit)
                              { return !RegisteredUnit.IsValid() || RegisteredUnit.Get() == Unit; });

    if (RemovedComponent)
    {
        // Keep the active initiative index stable relative to the shortened array after removal.
        const int32 RemovedIndex = InitiativeOrder.IndexOfByKey(RemovedComponent);
        if (RemovedIndex != INDEX_NONE)
        {
            InitiativeOrder.RemoveAt(RemovedIndex);

            if (RemovedIndex < ActiveInitiativeIndex)
            {
                --ActiveInitiativeIndex;
            }
            else if (RemovedIndex == ActiveInitiativeIndex)
            {
                ActiveInitiativeIndex = FMath::Clamp(RemovedIndex, 0, InitiativeOrder.Num() - 1);
                if (InitiativeOrder.Num() == 0)
                {
                    ActiveInitiativeIndex = INDEX_NONE;
                }
            }
        }
    }

    if (ActiveUnit.Get() == Unit || ActiveInitiativeIndex == INDEX_NONE || !GetCurrentActiveUnit())
    {
        if (bEncounterRunning)
        {
            // If the active slot disappeared, resolve the next surviving unit from the current cursor position.
            ActiveInitiativeIndex = FindNextAliveInitiativeIndex(FMath::Max(0, ActiveInitiativeIndex));
        }

        bEncounterRunning = InitiativeOrder.Num() > 0 && ActiveInitiativeIndex != INDEX_NONE;
        RefreshActiveUnitFromInitiative();
        RefreshRegisteredUnitNavigationBlockers();

        if (bEncounterRunning)
        {
            BroadcastActiveUnitChanged();
        }
    }
}

ETacticalTurnState UTacticalTurnSubsystem::GetCurrentState() const
{
    return CurrentState;
}

bool UTacticalTurnSubsystem::IsTurnModeActive() const
{
    return bIsTurnModeActive;
}

ACRPGBaseCharacter *UTacticalTurnSubsystem::GetActiveUnit() const
{
    return ActiveUnit.Get();
}

const TArray<UTacticalUnitComponent *> &UTacticalTurnSubsystem::GetInitiativeOrder() const
{
    return InitiativeOrder;
}

UTacticalUnitComponent *UTacticalTurnSubsystem::GetCurrentActiveUnit() const
{
    if (!InitiativeOrder.IsValidIndex(ActiveInitiativeIndex))
    {
        return nullptr;
    }

    UTacticalUnitComponent *TacticalUnitComponent = InitiativeOrder[ActiveInitiativeIndex];
    return IsValid(TacticalUnitComponent) && TacticalUnitComponent->IsAlive() ? TacticalUnitComponent : nullptr;
}

int32 UTacticalTurnSubsystem::GetCurrentRound() const
{
    return CurrentRound;
}

bool UTacticalTurnSubsystem::IsEncounterRunning() const
{
    return bEncounterRunning;
}

const TArray<TWeakObjectPtr<ACRPGBaseCharacter>> &UTacticalTurnSubsystem::GetRegisteredUnits() const
{
    return RegisteredUnits;
}

ACRPGBaseCharacter *UTacticalTurnSubsystem::ResolveCurrentlyPossessedUnit() const
{
    if (APlayerController *PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        return Cast<ACRPGBaseCharacter>(PlayerController->GetPawn());
    }

    return nullptr;
}

void UTacticalTurnSubsystem::PublishEvent(const FString &EventName, const FString &Payload) const
{
    if (EventBusSubsystem)
    {
        EventBusSubsystem->PublishEvent(EventName, Payload);
    }
}

void UTacticalTurnSubsystem::RefreshRegisteredUnitNavigationBlockers() const
{
    const ACRPGBaseCharacter *ReferenceCharacter = bIsTurnModeActive ? ActiveUnit.Get() : nullptr;

    for (const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit : RegisteredUnits)
    {
        if (ACRPGBaseCharacter *Character = RegisteredUnit.Get())
        {
            Character->UpdateTacticalOccupancyNavigationBlocker(ReferenceCharacter);
        }
    }
}

void UTacticalTurnSubsystem::RefreshRegisteredUnitCombatStances() const
{
    const ECombatStanceContext DesiredStance = bIsTurnModeActive ? ECombatStanceContext::CombatReady : ECombatStanceContext::Exploration;

    UE_LOG(
        LogTemp,
        Log,
        TEXT("[TacticalTurnSubsystem] RefreshRegisteredUnitCombatStances desired_stance=%d registered_units=%d initiative_units=%d turn_mode=%s"),
        static_cast<int32>(DesiredStance),
        RegisteredUnits.Num(),
        InitiativeOrder.Num(),
        bIsTurnModeActive ? TEXT("true") : TEXT("false"));

    for (const TWeakObjectPtr<ACRPGBaseCharacter> &RegisteredUnit : RegisteredUnits)
    {
        if (ACRPGBaseCharacter *Character = RegisteredUnit.Get())
        {
            UE_LOG(
                LogTemp,
                Log,
                TEXT("[TacticalTurnSubsystem] Applying stance=%d to character=%s current_stance=%d"),
                static_cast<int32>(DesiredStance),
                *GetNameSafe(Character),
                static_cast<int32>(Character->GetCombatStanceContext()));
            Character->SetCombatStanceContext(DesiredStance);
        }
    }
}

void UTacticalTurnSubsystem::CacheSubsystemDependencies()
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        CameraModeSubsystem = GameInstance->GetSubsystem<UCameraModeSubsystem>();
        EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>();
    }
}

void UTacticalTurnSubsystem::SortInitiativeOrder()
{
    InitiativeOrder.RemoveAll([](UTacticalUnitComponent *TacticalUnitComponent)
                              { return !IsValid(TacticalUnitComponent) || !TacticalUnitComponent->IsAlive(); });

    InitiativeOrder.Sort([](const UTacticalUnitComponent &Left, const UTacticalUnitComponent &Right)
                         {
        if (Left.GetCurrentInitiative() != Right.GetCurrentInitiative())
        {
            return Left.GetCurrentInitiative() > Right.GetCurrentInitiative();
        }

        const AActor* LeftOwner = Left.GetOwner();
        const AActor* RightOwner = Right.GetOwner();
        const FString LeftName = LeftOwner ? LeftOwner->GetName() : TEXT("");
        const FString RightName = RightOwner ? RightOwner->GetName() : TEXT("");
        return LeftName < RightName; });
}

void UTacticalTurnSubsystem::ResetInitiativeUnitsForNewRound()
{
    for (UTacticalUnitComponent *TacticalUnitComponent : InitiativeOrder)
    {
        if (!IsValid(TacticalUnitComponent))
        {
            continue;
        }

        if (TacticalUnitComponent->IsAlive())
        {
            TacticalUnitComponent->ResetForNewRound();
        }
        else
        {
            TacticalUnitComponent->SetTurnCompleted(true);
        }
    }
}

void UTacticalTurnSubsystem::RefreshActiveUnitFromInitiative()
{
    if (UTacticalUnitComponent *TacticalUnitComponent = GetCurrentActiveUnit())
    {
        ActiveUnit = Cast<ACRPGBaseCharacter>(TacticalUnitComponent->GetOwner());
        return;
    }

    ActiveUnit.Reset();
}

void UTacticalTurnSubsystem::BroadcastActiveUnitChanged() const
{
    if (const UTacticalUnitComponent *TacticalUnitComponent = GetCurrentActiveUnit())
    {
        PublishEvent(
            TacticalTurnEvents::TacticalActiveUnitChanged,
            FString::Printf(
                TEXT("round=%d;active_unit=%s;initiative=%d;initiative_index=%d"),
                CurrentRound,
                ActiveUnit.IsValid() ? *ActiveUnit->GetName() : TEXT("none"),
                TacticalUnitComponent->GetCurrentInitiative(),
                ActiveInitiativeIndex));
        return;
    }

    PublishEvent(
        TacticalTurnEvents::TacticalActiveUnitChanged,
        FString::Printf(
            TEXT("round=%d;active_unit=none;initiative=0;initiative_index=%d"),
            CurrentRound,
            ActiveInitiativeIndex));
}

int32 UTacticalTurnSubsystem::FindNextAliveInitiativeIndex(int32 StartIndex) const
{
    if (InitiativeOrder.Num() == 0)
    {
        return INDEX_NONE;
    }

    for (int32 InitiativeIndex = FMath::Max(0, StartIndex); InitiativeIndex < InitiativeOrder.Num(); ++InitiativeIndex)
    {
        if (UTacticalUnitComponent *TacticalUnitComponent = InitiativeOrder[InitiativeIndex])
        {
            if (IsValid(TacticalUnitComponent) && TacticalUnitComponent->IsAlive())
            {
                return InitiativeIndex;
            }
        }
    }

    return INDEX_NONE;
}

#include "TacticalTurnSubsystem.h"

#include "Camera/Subsystems/CameraModeSubsystem.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Tactical/Components/TacticalUnitComponent.h"

namespace TacticalTurnEvents
{
    static const FString TacticalTurnStarted = TEXT("tactical_turn_started");
    static const FString TacticalTurnEnded = TEXT("tactical_turn_ended");
    static const FString TacticalRoundStarted = TEXT("tactical_round_started");
    static constexpr float TacticalGlobalTimeDilation = 0.001f;
}

void UTacticalTurnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
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
    ActiveUnit.Reset();
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
    ClearActiveUnitTimeCompensation();

    bIsTurnModeActive = true;
    CurrentState = ETacticalTurnState::TacticalPaused;
    CurrentRound = 1;
    ActiveUnit = ResolveCurrentlyPossessedUnit();

    if (ACRPGBaseCharacter* ActiveCharacter = ActiveUnit.Get())
    {
        RegisterUnit(ActiveCharacter);

        if (UTacticalUnitComponent* TacticalUnitComponent = ActiveCharacter->GetTacticalUnitComponent())
        {
            TacticalUnitComponent->ResetForNewRound();
        }
    }

    if (UWorld* World = GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(World, TacticalTurnEvents::TacticalGlobalTimeDilation);
    }

    ApplyActiveUnitTimeCompensation();

    if (CameraModeSubsystem)
    {
        CameraModeSubsystem->RequestCameraMode(ECameraMode::Tactical, -1.0f, TacticalTurnEvents::TacticalTurnStarted);
    }

    PublishEvent(
        TacticalTurnEvents::TacticalTurnStarted,
        FString::Printf(
            TEXT("state=tactical_paused;round=%d;active_unit=%s;registered_units=%d"),
            CurrentRound,
            ActiveUnit.IsValid() ? *ActiveUnit->GetName() : TEXT("none"),
            RegisteredUnits.Num()));
}

void UTacticalTurnSubsystem::EndTurnMode()
{
    if (!bIsTurnModeActive)
    {
        return;
    }

    const int32 EndingRound = CurrentRound;
    const FString EndingUnitName = ActiveUnit.IsValid() ? ActiveUnit->GetName() : TEXT("none");

    ClearActiveUnitTimeCompensation();

    if (UWorld* World = GetWorld())
    {
        UGameplayStatics::SetGlobalTimeDilation(World, 1.0f);
    }

    ActiveUnit.Reset();
    CurrentState = ETacticalTurnState::Disabled;
    bIsTurnModeActive = false;
    CurrentRound = 0;

    PublishEvent(
        TacticalTurnEvents::TacticalTurnEnded,
        FString::Printf(
            TEXT("round=%d;active_unit=%s;registered_units=%d"),
            EndingRound,
            *EndingUnitName,
            RegisteredUnits.Num()));

    CacheSubsystemDependencies();
    if (CameraModeSubsystem)
    {
        CameraModeSubsystem->RequestCameraMode(ECameraMode::Exploration, -1.0f, TacticalTurnEvents::TacticalTurnEnded);
    }
}

void UTacticalTurnSubsystem::AdvanceRound()
{
    if (!bIsTurnModeActive)
    {
        return;
    }

    CurrentRound = FMath::Max(1, CurrentRound + 1);
    CurrentState = ETacticalTurnState::TacticalPaused;

    if (!ActiveUnit.IsValid())
    {
        ActiveUnit = ResolveCurrentlyPossessedUnit();
        ApplyActiveUnitTimeCompensation();
    }

    if (ACRPGBaseCharacter* ActiveCharacter = ActiveUnit.Get())
    {
        if (UTacticalUnitComponent* TacticalUnitComponent = ActiveCharacter->GetTacticalUnitComponent())
        {
            TacticalUnitComponent->ResetForNewRound();
        }
    }

    PublishEvent(
        TacticalTurnEvents::TacticalRoundStarted,
        FString::Printf(
            TEXT("round=%d;active_unit=%s"),
            CurrentRound,
            ActiveUnit.IsValid() ? *ActiveUnit->GetName() : TEXT("none")));
}

void UTacticalTurnSubsystem::RegisterUnit(ACRPGBaseCharacter* Unit)
{
    if (!IsValid(Unit))
    {
        return;
    }

    RegisteredUnits.RemoveAll([](const TWeakObjectPtr<ACRPGBaseCharacter>& RegisteredUnit)
    {
        return !RegisteredUnit.IsValid();
    });

    RegisteredUnits.AddUnique(Unit);
}

void UTacticalTurnSubsystem::UnregisterUnit(ACRPGBaseCharacter* Unit)
{
    RegisteredUnits.RemoveAll([Unit](const TWeakObjectPtr<ACRPGBaseCharacter>& RegisteredUnit)
    {
        return !RegisteredUnit.IsValid() || RegisteredUnit.Get() == Unit;
    });

    if (ActiveUnit.Get() == Unit)
    {
        ClearActiveUnitTimeCompensation();
        ActiveUnit = ResolveCurrentlyPossessedUnit();
        ApplyActiveUnitTimeCompensation();
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

ACRPGBaseCharacter* UTacticalTurnSubsystem::GetActiveUnit() const
{
    return ActiveUnit.Get();
}

int32 UTacticalTurnSubsystem::GetCurrentRound() const
{
    return CurrentRound;
}

ACRPGBaseCharacter* UTacticalTurnSubsystem::ResolveCurrentlyPossessedUnit() const
{
    if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        return Cast<ACRPGBaseCharacter>(PlayerController->GetPawn());
    }

    return nullptr;
}

void UTacticalTurnSubsystem::PublishEvent(const FString& EventName, const FString& Payload) const
{
    if (EventBusSubsystem)
    {
        EventBusSubsystem->PublishEvent(EventName, Payload);
    }
}

void UTacticalTurnSubsystem::ApplyActiveUnitTimeCompensation()
{
   if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        PlayerController->CustomTimeDilation = 1.0f / TacticalTurnEvents::TacticalGlobalTimeDilation;
    }

    if (ActiveUnit.IsValid())
    {
        ActiveUnit->CustomTimeDilation = 1.0f / TacticalTurnEvents::TacticalGlobalTimeDilation;
    }
}

void UTacticalTurnSubsystem::ClearActiveUnitTimeCompensation()
{
   if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
    {
        PlayerController->CustomTimeDilation = 1.0f;
    }

    if (ActiveUnit.IsValid())
    {
        ActiveUnit->CustomTimeDilation = 1.0f;
    }
}

void UTacticalTurnSubsystem::CacheSubsystemDependencies()
{
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        CameraModeSubsystem = GameInstance->GetSubsystem<UCameraModeSubsystem>();
        EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>();
    }
}

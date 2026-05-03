#include "GameCoreSubsystem.h"
#include "EventBusSubsystem.h"

void UGameCoreSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Warning, TEXT("[GameCore] Initialized"));

    EventBus = GetGameInstance()->GetSubsystem<UEventBusSubsystem>();

    if (EventBus)
    {
        EventBus->OnGameEvent.AddDynamic(this, &UGameCoreSubsystem::HandleGameEvent);

        EventBus->PublishEvent(TEXT("game_bootstrap_started"), TEXT("GameCore online"));
    }
}

void UGameCoreSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Warning, TEXT("[GameCore] Deinitialized"));

    Super::Deinitialize();
}

void UGameCoreSubsystem::HandleGameEvent(const FString& Payload)
{
    UE_LOG(LogTemp, Warning, TEXT("[GameCore] Received Event Payload -> %s"), *Payload);
}
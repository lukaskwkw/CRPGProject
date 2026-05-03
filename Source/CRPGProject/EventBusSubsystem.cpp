#include "EventBusSubsystem.h"

void UEventBusSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    UE_LOG(LogTemp, Warning, TEXT("[EventBus] Initialized"));
}

void UEventBusSubsystem::Deinitialize()
{
    UE_LOG(LogTemp, Warning, TEXT("[EventBus] Deinitialized"));

    Super::Deinitialize();
}

void UEventBusSubsystem::PublishEvent(const FString& EventName, const FString& Payload)
{
    FGameEvent NewEvent(EventName, Payload);
    EventHistory.Add(NewEvent);

    UE_LOG(LogTemp, Warning, TEXT("[EventBus] Published Event -> %s | Payload -> %s"), *EventName, *Payload);

    OnGameEvent.Broadcast(Payload);
}
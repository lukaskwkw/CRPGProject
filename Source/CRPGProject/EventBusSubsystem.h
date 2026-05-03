#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBusSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameEventSignature, const FString&, Payload);

USTRUCT(BlueprintType)
struct FGameEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString EventName;

    UPROPERTY(BlueprintReadOnly)
    FString Payload;

    FGameEvent() {}

    FGameEvent(const FString& InEventName, const FString& InPayload)
        : EventName(InEventName), Payload(InPayload) {
    }
};

UCLASS()
class CRPGPROJECT_API UEventBusSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable)
    void PublishEvent(const FString& EventName, const FString& Payload);

    UPROPERTY(BlueprintAssignable)
    FGameEventSignature OnGameEvent;

private:
    TArray<FGameEvent> EventHistory;
};
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EventBusSubsystem.generated.h"

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FGameEvent
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameEventSignature, const FString&, Payload);
DECLARE_MULTICAST_DELEGATE_TwoParams(FNamedGameEventSignature, const FString&, const FString&);

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

    FNamedGameEventSignature OnNamedGameEvent;

private:
    TArray<FGameEvent> EventHistory;
};
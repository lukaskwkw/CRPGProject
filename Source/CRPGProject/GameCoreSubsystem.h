#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameCoreSubsystem.generated.h"

class UEventBusSubsystem;

UCLASS()
class CRPGPROJECT_API UGameCoreSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

private:
    UPROPERTY()
    UEventBusSubsystem* EventBus;

    UFUNCTION()
    void HandleGameEvent(const FString& Payload);
};
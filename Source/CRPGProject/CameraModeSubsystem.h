#pragma once

#include "CoreMinimal.h"
#include "EventBusSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CameraModeSubsystem.generated.h"

UENUM(BlueprintType)
enum class ECameraMode : uint8
{
    Exploration UMETA(DisplayName = "Exploration"),
    Tactical UMETA(DisplayName = "Tactical"),
    Cinematic UMETA(DisplayName = "Cinematic")
};

USTRUCT(BlueprintType)
struct FCameraModeTransition
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ECameraMode PreviousMode = ECameraMode::Exploration;

    UPROPERTY(BlueprintReadOnly)
    ECameraMode NewMode = ECameraMode::Exploration;

    UPROPERTY(BlueprintReadOnly)
    float BlendTime = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    FString SourceEvent;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCameraModeChangedSignature, const FCameraModeTransition&, Transition);

UCLASS()
class CRPGPROJECT_API UCameraModeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UFUNCTION(BlueprintCallable, Category = "Camera")
    bool RequestCameraMode(ECameraMode NewMode, float BlendTime = -1.0f, const FString& SourceEvent = TEXT("manual_request"));

    UFUNCTION(BlueprintPure, Category = "Camera")
    ECameraMode GetCurrentCameraMode() const;

    UFUNCTION(BlueprintPure, Category = "Camera")
    const FCameraModeTransition& GetCurrentTransition() const;

    UPROPERTY(BlueprintAssignable, Category = "Camera")
    FCameraModeChangedSignature OnCameraModeChanged;

private:
    void HandleGameEvent(const FString& EventName, const FString& Payload);

    bool ApplyCameraMode(ECameraMode NewMode, float BlendTime, const FString& SourceEvent);
    float ResolveBlendTime(ECameraMode NewMode, float RequestedBlendTime) const;
    FString GetCameraModeName(ECameraMode Mode) const;

private:
    UPROPERTY()
    UEventBusSubsystem* EventBus = nullptr;

    UPROPERTY()
    ECameraMode CurrentMode = ECameraMode::Exploration;

    UPROPERTY()
    FCameraModeTransition CurrentTransition;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float DefaultBlendTime = 0.75f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float TacticalBlendTime = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Camera")
    float ExplorationBlendTime = 0.85f;
};

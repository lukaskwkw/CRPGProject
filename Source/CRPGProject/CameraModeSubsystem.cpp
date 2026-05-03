#include "CameraModeSubsystem.h"

#include "EventBusSubsystem.h"
#include "Engine/GameInstance.h"

namespace CameraModeEvents
{
    static const FString CombatStarted = TEXT("combat_started");
    static const FString CombatEnded = TEXT("combat_ended");
    static const FString CameraModeChanged = TEXT("camera_mode_changed");
}

void UCameraModeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    CurrentTransition.PreviousMode = CurrentMode;
    CurrentTransition.NewMode = CurrentMode;
    CurrentTransition.BlendTime = 0.0f;
    CurrentTransition.SourceEvent = TEXT("initial_state");

    EventBus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UEventBusSubsystem>() : nullptr;

    if (EventBus)
    {
        EventBus->OnNamedGameEvent.AddUObject(this, &UCameraModeSubsystem::HandleGameEvent);
    }

    UE_LOG(LogTemp, Warning, TEXT("[CameraMode] Initialized with mode %s"), *GetCameraModeName(CurrentMode));
}

void UCameraModeSubsystem::Deinitialize()
{
    if (EventBus)
    {
        EventBus->OnNamedGameEvent.RemoveAll(this);
        EventBus = nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT("[CameraMode] Deinitialized"));

    Super::Deinitialize();
}

bool UCameraModeSubsystem::RequestCameraMode(ECameraMode NewMode, float BlendTime, const FString& SourceEvent)
{
    return ApplyCameraMode(NewMode, BlendTime, SourceEvent);
}

ECameraMode UCameraModeSubsystem::GetCurrentCameraMode() const
{
    return CurrentMode;
}

const FCameraModeTransition& UCameraModeSubsystem::GetCurrentTransition() const
{
    return CurrentTransition;
}

void UCameraModeSubsystem::HandleGameEvent(const FString& EventName, const FString& Payload)
{
    if (EventName == CameraModeEvents::CombatStarted)
    {
        ApplyCameraMode(ECameraMode::Tactical, TacticalBlendTime, EventName);
        return;
    }

    if (EventName == CameraModeEvents::CombatEnded)
    {
        ApplyCameraMode(ECameraMode::Exploration, ExplorationBlendTime, EventName);
    }
}

bool UCameraModeSubsystem::ApplyCameraMode(ECameraMode NewMode, float BlendTime, const FString& SourceEvent)
{
    const float ResolvedBlendTime = ResolveBlendTime(NewMode, BlendTime);

    if (CurrentMode == NewMode && FMath::IsNearlyEqual(CurrentTransition.BlendTime, ResolvedBlendTime))
    {
        return false;
    }

    FCameraModeTransition Transition;
    Transition.PreviousMode = CurrentMode;
    Transition.NewMode = NewMode;
    Transition.BlendTime = ResolvedBlendTime;
    Transition.SourceEvent = SourceEvent;

    CurrentMode = NewMode;
    CurrentTransition = Transition;

    const FString Payload = FString::Printf(
        TEXT("mode=%s;previous=%s;blend_time=%.2f;source=%s"),
        *GetCameraModeName(Transition.NewMode),
        *GetCameraModeName(Transition.PreviousMode),
        Transition.BlendTime,
        *Transition.SourceEvent);

    UE_LOG(LogTemp, Warning, TEXT("[CameraMode] Mode changed -> %s | Blend -> %.2f | Source -> %s"), *GetCameraModeName(NewMode), ResolvedBlendTime, *SourceEvent);

    OnCameraModeChanged.Broadcast(CurrentTransition);

    if (EventBus)
    {
        EventBus->PublishEvent(CameraModeEvents::CameraModeChanged, Payload);
    }

    return true;
}

float UCameraModeSubsystem::ResolveBlendTime(ECameraMode NewMode, float RequestedBlendTime) const
{
    if (RequestedBlendTime >= 0.0f)
    {
        return RequestedBlendTime;
    }

    switch (NewMode)
    {
    case ECameraMode::Tactical:
        return TacticalBlendTime;

    case ECameraMode::Exploration:
        return ExplorationBlendTime;

    case ECameraMode::Cinematic:
        return DefaultBlendTime;

    default:
        return DefaultBlendTime;
    }
}

FString UCameraModeSubsystem::GetCameraModeName(ECameraMode Mode) const
{
    switch (Mode)
    {
    case ECameraMode::Exploration:
        return TEXT("exploration");

    case ECameraMode::Tactical:
        return TEXT("tactical");

    case ECameraMode::Cinematic:
        return TEXT("cinematic");

    default:
        return TEXT("unknown");
    }
}

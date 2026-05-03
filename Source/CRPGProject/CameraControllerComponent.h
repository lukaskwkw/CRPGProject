#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraModeSubsystem.h"
#include "CameraControllerComponent.generated.h"

class ACameraActor;
class APlayerController;
class UCameraComponent;
class USpringArmComponent;

USTRUCT(BlueprintType)
struct FCameraModeViewSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float ArmLength = 400.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector TargetOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FVector SocketOffset = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    FRotator Rotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
    float FieldOfView = 90.0f;
};

UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UCameraControllerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UCameraControllerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Camera|Tactical")
    void AddTacticalRoamInput(const FVector2D& InputAxis);

    UFUNCTION(BlueprintCallable, Category = "Camera|Tactical")
    void ClearTacticalRoamInput();

    UFUNCTION(BlueprintCallable, Category = "Camera|Tactical")
    void RecenterOnActivePawn();

    UFUNCTION(BlueprintCallable, Category = "Camera|Tactical")
    void AdjustTacticalZoom(float ZoomAxisValue);

    UFUNCTION(BlueprintCallable, Category = "Camera|Tactical")
    void AddTacticalYawInput(float YawAxisValue);

    UFUNCTION(BlueprintPure, Category = "Camera")
    ECameraMode GetActiveCameraMode() const;

protected:
    UFUNCTION()
    void HandleCameraModeChanged(const FCameraModeTransition& Transition);

private:
    void InitializeForCurrentMode();
    void BindToCameraModeSubsystem();
    void UnbindFromCameraModeSubsystem();
    void UpdateTacticalPitchFromZoom();
    void EnsureTacticalCamera();
    void HandleControlledPawnChanged();
    void EnterExplorationMode(const FCameraModeTransition& Transition);
    void EnterTacticalMode(const FCameraModeTransition& Transition);
    void EnterCinematicMode(const FCameraModeTransition& Transition);
    void UpdateExplorationCamera(float DeltaTime);
    void UpdateTacticalCamera(float DeltaTime);
    const FCameraModeViewSettings& GetModeViewSettings(ECameraMode CameraMode) const;
    APawn* GetControlledPawn() const;
    USpringArmComponent* FindControlledSpringArm() const;
    UCameraComponent* FindControlledCamera() const;
    FVector GetPawnFocusLocation(APawn* Pawn) const;
    float GetModeInterpolationSpeed() const;
    float GetAnchorInterpolationSpeed() const;

private:
    UPROPERTY(EditAnywhere, Category = "Camera|Modes")
    FCameraModeViewSettings ExplorationSettings;

    UPROPERTY(EditAnywhere, Category = "Camera|Modes")
    FCameraModeViewSettings TacticalSettings;

    UPROPERTY(EditAnywhere, Category = "Camera|Modes")
    FCameraModeViewSettings CinematicSettings;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical", meta = (ClampMin = "0.0", Units = "cm/s"))
    float TacticalRoamSpeed = 1800.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalMinimumArmLength = 500.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalMaximumArmLength = 1500.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalZoomStep = 100.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical", meta = (ClampMin = "0.0"))
    float TacticalYawRotationSpeed = 2.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical")
    float TacticalClosestPitch = -50.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical")
    float TacticalFarthestPitch = -75.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical", meta = (ClampMin = "0.0"))
    float MinimumInterpolationSpeed = 4.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical", meta = (ClampMin = "0.0"))
    float MaximumInterpolationSpeed = 14.0f;

    UPROPERTY(EditAnywhere, Category = "Camera|Tactical")
    FVector PawnFocusOffset = FVector(0.0f, 0.0f, 100.0f);

    UPROPERTY(Transient)
    TObjectPtr<APlayerController> OwningPlayerController;

    UPROPERTY(Transient)
    TObjectPtr<UCameraModeSubsystem> CameraModeSubsystem;

    UPROPERTY(Transient)
    TObjectPtr<ACameraActor> TacticalCameraActor;

    UPROPERTY(Transient)
    TObjectPtr<APawn> CachedPawn;

    UPROPERTY(Transient)
    ECameraMode ActiveCameraMode = ECameraMode::Exploration;

    UPROPERTY(Transient)
    FCameraModeTransition ActiveTransition;

    UPROPERTY(Transient)
    FVector TacticalAnchorLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    FVector TargetTacticalAnchorLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    FVector2D TacticalRoamInput = FVector2D::ZeroVector;

    UPROPERTY(Transient)
    float TacticalYaw = 0.0f;

    UPROPERTY(Transient)
    bool bHasTacticalAnchor = false;
};

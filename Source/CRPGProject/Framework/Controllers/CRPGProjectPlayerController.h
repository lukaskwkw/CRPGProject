// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CameraModeSubsystem.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CRPGProjectPlayerController.generated.h"

class UCameraControllerComponent;
class UInputMappingContext;
class UNavigationPath;
class UTacticalCombatHUDWidget;
class UTacticalTurnSubsystem;
class UUserWidget;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */
UCLASS(abstract)
class ACRPGProjectPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ACRPGProjectPlayerController();
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|HUD", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UTacticalCombatHUDWidget> TacticalCombatHUDWidgetClass;

	UPROPERTY()
	TObjectPtr<UTacticalCombatHUDWidget> TacticalCombatHUDWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	/** Camera transition controller owned by the player controller */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraControllerComponent> CameraControllerComponent;

	/** Gameplay initialization */
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Temporary debug camera mode toggle */
	void ToggleDebugCameraMode();
	void HandleTacticalMoveForwardPressed();
	void HandleTacticalMoveForwardReleased();
	void HandleTacticalMoveBackwardPressed();
	void HandleTacticalMoveBackwardReleased();
	void HandleTacticalMoveRightPressed();
	void HandleTacticalMoveRightReleased();
	void HandleTacticalMoveLeftPressed();
	void HandleTacticalMoveLeftReleased();
	void HandleTacticalZoom(float AxisValue);
	void HandleTacticalMouseX(float AxisValue);
	void HandleTacticalRightMousePressed();
	void HandleTacticalRightMouseReleased();
	void HandleTacticalLeftMousePressed();
	void HandleTacticalLeftClick();
	void BindToCameraModeSubsystem();
	void UnbindFromCameraModeSubsystem();
    void BindToEventBus();
	void UnbindFromEventBus();
	void ApplyCameraModeInputState(ECameraMode CameraMode);
	void StartTacticalPathTraversal(const TArray<FVector>& PathPoints, float PendingDistanceConsumption = 0.0f);
	void UpdateTacticalPathTraversal(float DeltaTime);
	void ClearTacticalPathTraversal(bool bStopPawnMovement);
	void RotatePawnTowardDirection(APawn* ControlledPawn, const FVector& MoveDirection, float DeltaTime);
 float CalculatePathDistance(const TArray<FVector>& PathPoints) const;
	void DrawDebugTacticalPath(const TArray<FVector>& PathPoints, bool bIsValid) const;
	UTacticalTurnSubsystem* GetTacticalTurnSubsystem() const;
	void RefreshTacticalCombatHUD() const;
	void SpawnTacticalCombatHUD();
	void HandleTacticalPathTraversalCompleted();
	UFUNCTION()
	void HandleCameraModeChanged(const FCameraModeTransition& Transition);
 void HandleGameEvent(const FString& EventName, const FString& Payload);
	void UpdateTacticalRoamInput();
    void SetControlledPawnTacticalInputSuppressed(bool bSuppressInput);
	void StopTacticalPrototypeMovement();
	bool IsTacticalModeActive() const;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	float TacticalMoveForwardInput = 0.0f;
	float TacticalMoveRightInput = 0.0f;
  UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "1.0", Units = "cm"))
	float TacticalAcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.0"))
	float TacticalPathRotationInterpSpeed = 10.0f;

	bool bHasActiveTacticalPath = false;
	TArray<FVector> ActiveTacticalPathPoints;
	int32 CurrentPathIndex = INDEX_NONE;
	bool bIsTacticalRotateHeld = false;
	float PendingPathDistanceConsumption = 0.0f;

public:

	FORCEINLINE UCameraControllerComponent* GetCameraControllerComponent() const { return CameraControllerComponent; }

};

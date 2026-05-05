// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CameraModeSubsystem.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Tactical/Movement/TacticalPathTypes.h"
#include "CRPGProjectPlayerController.generated.h"

class UCameraControllerComponent;
class UInputMappingContext;
class UNavigationPath;
class UTacticalCombatHUDWidget;
class UTacticalPathPreviewComponent;
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
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext *> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category = "Input|Input Mappings")
	TArray<UInputMappingContext *> MobileExcludedMappingContexts;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category = "Input|Touch Controls")
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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Movement", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTacticalPathPreviewComponent> TacticalPathPreviewComponent;

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
	void BuildTacticalMovePreview(const FVector &Destination);
	void BuildClampedTacticalPath(const TArray<FVector> &SourcePathPoints, float MaxDistanceCm, TArray<FVector> &OutPathPoints, float &OutDistanceCm) const;
	void CommitPendingTacticalMove();
	void ClearPendingTacticalMovePreview();
	bool IsClickNearPendingDestination(const FVector &ClickedLocation) const;
	bool TryGetTacticalCursorWorldLocation(FVector &OutWorldLocation) const;
	void UpdateTacticalMovePreviewFromHover();
	void StopActiveTacticalMove(bool bConsumeTravelledDistance);
	float GetReservedMovementDistanceCm() const;
	float GetAvailableMovementBudgetForPlanning() const;
	void StartTacticalPathTraversal(const TArray<FVector> &PathPoints, float PendingDistanceConsumption = 0.0f);
	void UpdateTacticalPathTraversal(float DeltaTime);
	void ClearTacticalPathTraversal(bool bStopPawnMovement);
	void RotatePawnTowardDirection(APawn *ControlledPawn, const FVector &MoveDirection, float DeltaTime);
	float CalculatePathDistance(const TArray<FVector> &PathPoints) const;
	UTacticalTurnSubsystem *GetTacticalTurnSubsystem() const;
	void RefreshTacticalCombatHUD() const;
	void SpawnTacticalCombatHUD();
	void HandleTacticalPathTraversalCompleted();
	UFUNCTION()
	void HandleCameraModeChanged(const FCameraModeTransition &Transition);
	void HandleGameEvent(const FString &EventName, const FString &Payload);
	void SyncPossessionToActiveTacticalUnit();
	void RestorePossessionAfterTacticalTurn();
	void UpdateTacticalRoamInput();
	void SetControlledPawnTacticalInputSuppressed(bool bSuppressInput);
	void StopTacticalPrototypeMovement();
	bool IsTacticalModeActive() const;
	virtual void OnPossess(APawn *InPawn) override;
	virtual void OnUnPossess() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

	float TacticalMoveForwardInput = 0.0f;
	float TacticalMoveRightInput = 0.0f;
	UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "1.0", Units = "cm"))
	float TacticalAcceptanceRadius = 50.0f;

	UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "1.0", Units = "cm"))
	float TacticalFinalAcceptanceRadius = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.0"))
	float TacticalPathRotationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.0", Units = "cm"))
	float TacticalHoverPreviewRefreshTolerance = 25.0f;

	UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.01", Units = "s"))
	float TacticalHoverPreviewRefreshInterval = 0.2f;

	UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.0", Units = "cm"))
	float TacticalMinimumCommittedMoveDistance = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Debug")
	bool bAllowControllingNonPlayerTacticalUnits = false;

	bool bHasActiveTacticalPath = false;
	TArray<FVector> ActiveTacticalPathPoints;
	int32 CurrentPathIndex = INDEX_NONE;
	bool bIsTacticalRotateHeld = false;
	float PendingPathDistanceConsumption = 0.0f;
	float ActiveTraversalTravelledDistanceCm = 0.0f;
	float NextTacticalHoverPreviewRefreshTime = 0.0f;
	FVector LastTraversalPawnLocation = FVector::ZeroVector;
	bool bTurnModeMovementEnabled = true;
	TWeakObjectPtr<APawn> ExplorationPawnBeforeTacticalTurn;

	UPROPERTY()
	FTacticalMovePreviewData PendingMovePreview;

public:
	FORCEINLINE UCameraControllerComponent *GetCameraControllerComponent() const { return CameraControllerComponent; }
	FORCEINLINE const FTacticalMovePreviewData &GetPendingMovePreview() const { return PendingMovePreview; }

	UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
	void CommitPendingTacticalMoveRequest();

	UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
	void ClearPendingTacticalMovePreviewRequest();

	UFUNCTION(BlueprintPure, Category = "Tactical|Movement")
	bool IsTurnModeMovementEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
	void SetTurnModeMovementEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
	void ToggleTurnModeMovementEnabled();

	UFUNCTION(BlueprintPure, Category = "Tactical|Movement")
	float GetAvailableMovementBudgetForPlanningRequest() const;
};

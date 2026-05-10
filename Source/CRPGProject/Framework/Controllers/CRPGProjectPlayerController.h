// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CameraModeSubsystem.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Tactical/Movement/TacticalPathTypes.h"
#include "CRPGProjectPlayerController.generated.h"

class UCameraControllerComponent;
class UInputMappingContext;
class ACRPGBaseCharacter;
class UTacticalCombatHUDWidget;
class UTacticalMovementControllerComponent;
class UTacticalPathPreviewComponent;
class UTacticalTurnSyncComponent;
class UTacticalTurnSubsystem;
class UUserWidget;

/**
 * Player controller that owns the tactical camera, hover-preview helpers and movement orchestration.
 *
 * The actual path solving and traversal live in UTacticalMovementControllerComponent, while this class
 * exposes the tuning knobs used by preview smoothing, look-ahead traversal and click-to-move UX.
 */
UCLASS(abstract)
class ACRPGProjectPlayerController : public APlayerController
{
    GENERATED_BODY()

    friend class UTacticalTurnSyncComponent;

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
    TObjectPtr<UTacticalMovementControllerComponent> TacticalMovementControllerComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Turn", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTacticalTurnSyncComponent> TacticalTurnSyncComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Movement", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTacticalPathPreviewComponent> TacticalPathPreviewComponent;

    /** Gameplay initialization */
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
    float GetAvailableMovementBudgetForPlanning() const;
    UTacticalTurnSubsystem *GetTacticalTurnSubsystem() const;
    void SpawnTacticalCombatHUD();
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

    // Radius for intermediate control points during traversal. Larger values cut corners earlier.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "1.0", Units = "cm"))
    float TacticalAcceptanceRadius = 50.0f;

    // Tightened acceptance used only for the final node so the unit stops close to the requested destination.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "1.0", Units = "cm"))
    float TacticalFinalAcceptanceRadius = 10.0f;

    // Turn speed used while the pawn aligns to the preview/traversal path.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.0"))
    float TacticalPathRotationInterpSpeed = 10.0f;

    // Distance to the next look-ahead control point used to keep movement smooth on curved paths.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Traversal", meta = (ClampMin = "1.0", Units = "cm"))
    float TraversalLookAheadDistance = 80.0f;

    // Spacing between authored traversal control points extracted from the visual preview path.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Traversal", meta = (ClampMin = "1.0", Units = "cm"))
    float TraversalControlPointSpacing = 90.0f;

    // Sampling density for the raw nav path before occupancy relaxation and spline smoothing.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Preview", meta = (ClampMin = "1.0", Units = "cm"))
    float DenseSampleSpacing = 40.0f;

    UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalHoverPreviewRefreshTolerance = 25.0f;

    // Extra padding added around occupied units when offsetting the preview path away from them.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Preview", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalAvoidMargin = 20.0f;

    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Preview", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalAvoidTestOffsetSmall = 40.0f;

    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Preview", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalAvoidTestOffsetMedium = 70.0f;

    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Preview", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalAvoidTestOffsetLarge = 100.0f;

    // Visual-only Catmull-Rom subdivisions; higher values smooth the debug path but also increase its length slightly.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Preview", meta = (ClampMin = "0"))
    int32 PreviewSplineSubdivisions = 2;

    UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.01", Units = "s"))
    float TacticalHoverPreviewRefreshInterval = 0.2f;

    // Small moves below this threshold are treated as accidental clicks and ignored.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical", meta = (ClampMin = "0.0", Units = "cm"))
    float TacticalMinimumCommittedMoveDistance = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Debug")
    bool bAllowControllingNonPlayerTacticalUnits = false;

    bool bIsTacticalRotateHeld = false;

    UPROPERTY(Transient)
    TArray<FVector> CommittedTraversalControlPoints;

public:
    FORCEINLINE UCameraControllerComponent *GetCameraControllerComponent() const { return CameraControllerComponent; }
    FORCEINLINE UTacticalMovementControllerComponent *GetTacticalMovementControllerComponent() const { return TacticalMovementControllerComponent; }
    FORCEINLINE UTacticalPathPreviewComponent *GetTacticalPathPreviewComponent() const { return TacticalPathPreviewComponent; }
    FORCEINLINE UTacticalTurnSyncComponent *GetTacticalTurnSyncComponent() const { return TacticalTurnSyncComponent; }
    // Preview path pipeline: nav path -> dense samples -> occupancy offsets -> smoothed visual spline.
    TArray<FVector> BuildDenseResampledPath(const TArray<FVector> &RawNavPoints, float SampleSpacing) const;
    FVector RelaxPreviewPointAgainstUnits(const FVector &SamplePoint, const FVector &PathDirection, const ACRPGBaseCharacter *MovingCharacter) const;
    TArray<FVector> ApplyOccupancyRelaxationToPath(const TArray<FVector> &DensePath, const ACRPGBaseCharacter *MovingCharacter) const;
    TArray<FVector> BuildSmoothedPreviewPath(const TArray<FVector> &RelaxedPath, int32 SubdivisionsPerSegment) const;
    // Traversal path pipeline: visual preview -> sparse control points -> look-ahead target each tick.
    TArray<FVector> BuildTraversalControlPath(const TArray<FVector> &VisualPreviewPath, float ControlPointSpacing) const;
    FVector GetTraversalLookAheadTarget(const FVector &CurrentLocation, int32 CurrentPathIndex) const;
    void SetCommittedTraversalControlPoints(const TArray<FVector> &PathPoints);
    void ClearCommittedTraversalControlPoints();
    const TArray<FVector> &GetCommittedTraversalControlPoints() const;
    const FTacticalMovePreviewData &GetPendingMovePreview() const;
    void RefreshTacticalCombatHUD() const;
    float GetTacticalAcceptanceRadius() const { return TacticalAcceptanceRadius; }
    float GetTacticalFinalAcceptanceRadius() const { return TacticalFinalAcceptanceRadius; }
    float GetTacticalPathRotationInterpSpeed() const { return TacticalPathRotationInterpSpeed; }
    float GetTraversalLookAheadDistance() const { return TraversalLookAheadDistance; }
    float GetTraversalControlPointSpacing() const { return TraversalControlPointSpacing; }
    float GetDenseSampleSpacing() const { return DenseSampleSpacing; }
    float GetTacticalAvoidMargin() const { return TacticalAvoidMargin; }
    float GetTacticalAvoidTestOffsetSmall() const { return TacticalAvoidTestOffsetSmall; }
    float GetTacticalAvoidTestOffsetMedium() const { return TacticalAvoidTestOffsetMedium; }
    float GetTacticalAvoidTestOffsetLarge() const { return TacticalAvoidTestOffsetLarge; }
    int32 GetPreviewSplineSubdivisions() const { return PreviewSplineSubdivisions; }
    float GetTacticalHoverPreviewRefreshTolerance() const { return TacticalHoverPreviewRefreshTolerance; }
    float GetTacticalHoverPreviewRefreshInterval() const { return TacticalHoverPreviewRefreshInterval; }
    float GetTacticalMinimumCommittedMoveDistance() const { return TacticalMinimumCommittedMoveDistance; }

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

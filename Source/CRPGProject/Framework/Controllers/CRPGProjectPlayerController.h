// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CameraModeSubsystem.h"
#include "Combat/Types/CombatTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Tactical/Movement/TacticalPathTypes.h"
#include "CRPGProjectPlayerController.generated.h"

class UCameraControllerComponent;
class UCombatResolverSubsystem;
class UInputMappingContext;
class ACRPGBaseCharacter;
class UTacticalCombatHUDWidget;
class UTacticalMovementControllerComponent;
class UTacticalOutlineOverlayComponent;
class UTacticalPathPreviewComponent;
class UTacticalTurnSyncComponent;
class UTacticalTurnSubsystem;
class UTacticalUnitComponent;
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
    friend class UTacticalMovementControllerComponent;
    friend class UTacticalCombatHUDWidget;

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

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tactical|Outline", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UTacticalOutlineOverlayComponent> TacticalOutlineOverlayComponent;

    /** Gameplay initialization */
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    /** Input mapping context setup */
    virtual void SetupInputComponent() override;

    /** Temporary debug camera mode toggle */
    void ToggleDebugCameraMode();
    /** Applies forward tactical roam input while turn mode is active. */
    void HandleTacticalMoveForwardPressed();
    /** Clears forward tactical roam input when the key is released. */
    void HandleTacticalMoveForwardReleased();
    /** Applies backward tactical roam input while turn mode is active. */
    void HandleTacticalMoveBackwardPressed();
    /** Clears backward tactical roam input when the key is released. */
    void HandleTacticalMoveBackwardReleased();
    /** Applies right-strafe tactical roam input while turn mode is active. */
    void HandleTacticalMoveRightPressed();
    /** Clears right-strafe tactical roam input when the key is released. */
    void HandleTacticalMoveRightReleased();
    /** Applies left-strafe tactical roam input while turn mode is active. */
    void HandleTacticalMoveLeftPressed();
    /** Clears left-strafe tactical roam input when the key is released. */
    void HandleTacticalMoveLeftReleased();
    /** Forwards mouse wheel zoom input to the tactical camera controller. */
    void HandleTacticalZoom(float AxisValue);
    /** Rotates the tactical camera while RMB rotate mode is held. */
    void HandleTacticalMouseX(float AxisValue);
    /** Starts tactical camera rotate mode. */
    void HandleTacticalRightMousePressed();
    /** Ends tactical camera rotate mode. */
    void HandleTacticalRightMouseReleased();
    /** Captures the current cursor state before resolving a tactical click. */
    void HandleTacticalLeftMousePressed();
    /** Commits or clears the pending tactical click action after release. */
    void HandleTacticalLeftClick();
    /** Hooks camera-mode events so controller input stays in sync with presentation mode. */
    void BindToCameraModeSubsystem();
    /** Removes camera-mode bindings during teardown. */
    void UnbindFromCameraModeSubsystem();
    /** Subscribes to gameplay events that should refresh tactical HUD/controller state. */
    void BindToEventBus();
    /** Removes gameplay-event subscriptions during teardown. */
    void UnbindFromEventBus();
    /** Enables or disables controller input affordances for the requested camera mode. */
    void ApplyCameraModeInputState(ECameraMode CameraMode);
    /** Returns the movement budget the UI should use when planning the next move. */
    float GetAvailableMovementBudgetForPlanning() const;
    /** Resolves the tactical turn subsystem from the current game instance. */
    UTacticalTurnSubsystem *GetTacticalTurnSubsystem() const;
    /** Spawns the tactical combat HUD for the local player if a widget class is configured. */
    void SpawnTacticalCombatHUD();
    UFUNCTION()
    /** Reacts to camera mode transitions and keeps tactical control state aligned. */
    void HandleCameraModeChanged(const FCameraModeTransition &Transition);
    /** Handles EventBus messages relevant to tactical possession and HUD refresh. */
    void HandleGameEvent(const FString &EventName, const FString &Payload);
    /** Possesses the active tactical unit when turn mode hands control away from the player pawn. */
    void SyncPossessionToActiveTacticalUnit();
    /** Restores possession to the original player pawn after tactical control ends. */
    void RestorePossessionAfterTacticalTurn();
    /** Pushes the latest roam input axes into the tactical movement component. */
    void UpdateTacticalRoamInput();
    /** Suppresses manual pawn input while tactical systems are driving locomotion. */
    void SetControlledPawnTacticalInputSuppressed(bool bSuppressInput);
    /** Stops prototype tactical traversal and clears transient preview state. */
    void StopTacticalPrototypeMovement();
    /** Returns true while the controller should behave in tactical camera/input mode. */
    bool IsTacticalModeActive() const;
    /** Starts controller-owned combat targeting for the requested action. */
    // This is the entry point from HUD intent into world-space targeting. It arms the requested action,
    // disables regular tactical click-to-move, and lets the movement component switch from location preview
    // into approach-to-range preview while the player hovers enemies.
    void EnterCombatTargetingMode(ECombatActionType ActionType);
    /** Clears controller-owned combat targeting state and restores movement gating. */
    // This is used by RMB cancel, repeated HUD-button presses, turn ownership changes, and possession changes.
    void ClearCombatTargetingMode();
    /** Resolves the tactical unit currently under the cursor, if any. */
    // Combat targeting prefers a pawn hit under the cursor, because a normal visibility trace often resolves to the floor.
    UTacticalUnitComponent *ResolveUnitUnderCursor() const;
    /** Applies hover-target bookkeeping and world highlight state when the cursor moves between enemies. */
    void SetHoveredCombatTarget(UTacticalUnitComponent *NewTargetUnit);
    /** Applies or clears world-space highlight presentation for the supplied tactical unit. */
    void SetCombatTargetHighlight(UTacticalUnitComponent *TargetUnit, bool bEnableHighlight) const;
    /** Returns whether the defender is a legal prototype attack target for the attacker. */
    bool IsValidCombatTarget(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender) const;
    /** Returns the current edge-to-edge gap between attacker and defender occupancies. */
    float GetCombatGapToTargetCm(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender) const;
    /** Returns whether the requested combat action is currently in range. */
    bool IsTargetInCombatRange(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender, ECombatActionType ActionType) const;
    /** Returns whether combat targeting currently wants approach-preview support from the movement component. */
    bool IsCombatTargetingPreviewActive() const;
    /** Calculates a desired world-space destination that would move the attacker into attack range. */
    bool TryGetCombatTargetingPreviewDestination(FVector &OutDestination) const;
    /** Attempts to resolve the currently pending attack against the supplied target. */
    // For melee, this can either attack immediately when already in range or commit the previewed approach path first
    // and defer the actual attack until traversal completes.
    bool TryExecutePendingCombatAction(UTacticalUnitComponent *TargetUnit);
    /** Handles any deferred melee follow-up after tactical traversal completes. */
    void HandleTacticalTraversalCompleted();
    /** Resolves the combat resolver subsystem from the current game instance. */
    UCombatResolverSubsystem *GetCombatResolverSubsystem() const;
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

    // Small forgiveness for combat range checks so nav projection and final traversal acceptance do not leave the unit
    // visually in range but logically a few centimeters short.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Combat", meta = (ClampMin = "0.0", Units = "cm"))
    float CombatRangeToleranceCm = 20.0f;

    // When plotting an approach-to-range preview, target the inside of the legal range rather than the exact threshold
    // so a traversal that stops slightly early can still complete the intended attack.
    UPROPERTY(EditAnywhere, Category = "Input|Tactical|Combat", meta = (ClampMin = "0.0", Units = "cm"))
    float CombatApproachBufferCm = 25.0f;

    /** Debug-only escape hatch that allows repossessing the current active enemy for testing turn flow. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tactical|Debug")
    bool bAllowControllingNonPlayerTacticalUnits = false;

    bool bIsTacticalRotateHeld = false;

    UPROPERTY(Transient)
    TArray<FVector> CommittedTraversalControlPoints;

    UPROPERTY(Transient)
    ECombatActionType PendingCombatAction = ECombatActionType::None;

    UPROPERTY(Transient)
    ECombatTargetingMode CurrentTargetingMode = ECombatTargetingMode::None;

    UPROPERTY(Transient)
    TWeakObjectPtr<UTacticalUnitComponent> HoveredTargetUnit;

    UPROPERTY(Transient)
    bool bMovementEnabledBeforeCombatTargeting = true;

    // Switching melee <-> ranged while already targeting must not overwrite the pre-targeting movement state,
    // otherwise leaving targeting can permanently restore the wrong value.

    UPROPERTY(Transient)
    ECombatActionType DeferredCombatActionAfterTraversal = ECombatActionType::None;

    UPROPERTY(Transient)
    // Stores the move-then-attack follow-up while melee closes distance through the movement component.
    TWeakObjectPtr<UTacticalUnitComponent> DeferredCombatTargetAfterTraversal;

public:
    FORCEINLINE UCameraControllerComponent *GetCameraControllerComponent() const { return CameraControllerComponent; }
    FORCEINLINE UTacticalMovementControllerComponent *GetTacticalMovementControllerComponent() const { return TacticalMovementControllerComponent; }
    FORCEINLINE UTacticalPathPreviewComponent *GetTacticalPathPreviewComponent() const { return TacticalPathPreviewComponent; }
    FORCEINLINE UTacticalTurnSyncComponent *GetTacticalTurnSyncComponent() const { return TacticalTurnSyncComponent; }
    // Preview path pipeline: nav path -> dense samples -> occupancy offsets -> smoothed visual spline.
    /** Resamples the raw navigation path so later smoothing and occupancy checks use even spacing. */
    TArray<FVector> BuildDenseResampledPath(const TArray<FVector> &RawNavPoints, float SampleSpacing) const;
    /** Pushes a preview sample away from occupied units without changing the underlying nav path. */
    FVector RelaxPreviewPointAgainstUnits(const FVector &SamplePoint, const FVector &PathDirection, const ACRPGBaseCharacter *MovingCharacter) const;
    /** Applies occupancy avoidance to every dense sample used by the preview spline. */
    TArray<FVector> ApplyOccupancyRelaxationToPath(const TArray<FVector> &DensePath, const ACRPGBaseCharacter *MovingCharacter) const;
    /** Builds the final visual spline shown for hover-preview feedback. */
    TArray<FVector> BuildSmoothedPreviewPath(const TArray<FVector> &RelaxedPath, int32 SubdivisionsPerSegment) const;
    // Traversal path pipeline: visual preview -> sparse control points -> look-ahead target each tick.
    /** Converts the visual preview into a sparser control path suitable for locomotion. */
    TArray<FVector> BuildTraversalControlPath(const TArray<FVector> &VisualPreviewPath, float ControlPointSpacing) const;
    /** Chooses the next look-ahead target that the moving pawn should steer toward. */
    FVector GetTraversalLookAheadTarget(const FVector &CurrentLocation, int32 CurrentPathIndex) const;
    /** Stores the traversal path that was committed from the current preview. */
    void SetCommittedTraversalControlPoints(const TArray<FVector> &PathPoints);
    /** Clears any committed traversal path after movement completes or is cancelled. */
    void ClearCommittedTraversalControlPoints();
    /** Returns the committed traversal path currently being followed. */
    const TArray<FVector> &GetCommittedTraversalControlPoints() const;
    /** Returns the latest movement preview generated by the tactical movement component. */
    const FTacticalMovePreviewData &GetPendingMovePreview() const;
    /** Requests a fresh HUD refresh when controller-owned tactical data changes. */
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
    /** Commits the current hover-preview path as the next tactical move order. */
    void CommitPendingTacticalMoveRequest();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
    /** Cancels the current hover-preview path without moving the unit. */
    void ClearPendingTacticalMovePreviewRequest();

    UFUNCTION(BlueprintPure, Category = "Tactical|Movement")
    /** Returns whether tactical click-to-move input is currently enabled. */
    bool IsTurnModeMovementEnabled() const;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
    /** Enables or disables tactical click-to-move input explicitly. */
    void SetTurnModeMovementEnabled(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
    /** Flips tactical click-to-move input between enabled and disabled states. */
    void ToggleTurnModeMovementEnabled();

    UFUNCTION(BlueprintPure, Category = "Tactical|Movement")
    /** Exposes the planning budget to Blueprint/UI callers. */
    float GetAvailableMovementBudgetForPlanningRequest() const;

    /** Selects a player-controlled unit from the HUD party panel. */
    bool SelectPlayerPartyUnit(ACRPGBaseCharacter *RequestedUnit);

    /** Moves the current camera focus to the requested tactical unit without changing encounter state. */
    bool FocusCameraOnTacticalUnit(ACRPGBaseCharacter *RequestedUnit);

    /** Selects a player ally when possible, otherwise just focuses the camera on the requested unit. */
    bool SelectOrFocusTacticalUnit(ACRPGBaseCharacter *RequestedUnit);

    /** Returns whether the cursor is currently over tactical HUD controls that should block hover preview. */
    bool IsPointerOverTacticalHUD() const;

    /** Tells the controller that the cursor entered tactical HUD space so hover previews can pause. */
    void NotifyTacticalUIHoverBegin();

    /** Tells the controller that the cursor left tactical HUD space so hover previews can resume. */
    void NotifyTacticalUIHoverEnd();

    /** Returns whether the cursor is currently interacting with tactical HUD widgets. */
    bool IsHoveringTacticalUI() const;

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the currently armed combat action for HUD state and debugging. */
    ECombatActionType GetPendingCombatAction() const { return PendingCombatAction; }

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the current combat targeting mode. */
    ECombatTargetingMode GetCurrentCombatTargetingMode() const { return CurrentTargetingMode; }

    UFUNCTION(BlueprintPure, Category = "Tactical|Combat")
    /** Returns the target unit most recently resolved under the cursor while targeting. */
    UTacticalUnitComponent *GetHoveredTargetUnit() const { return HoveredTargetUnit.Get(); }

private:
    UPROPERTY(Transient)
    int32 TacticalUIHoverDepth = 0;
};

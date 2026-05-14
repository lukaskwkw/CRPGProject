// Copyright Epic Games, Inc. All Rights Reserved.

#include "CRPGProjectPlayerController.h"

#include "Camera/Components/CameraControllerComponent.h"
#include "CameraModeSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Blueprint/UserWidget.h"
#include "CRPGProject.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Components/TacticalMovementControllerComponent.h"
#include "Tactical/Components/TacticalOutlineOverlayComponent.h"
#include "Tactical/Components/TacticalPathPreviewComponent.h"
#include "Tactical/Components/TacticalTurnSyncComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "UI/HUD/TacticalHUDActionEvents.h"
#include "UI/HUD/TacticalCombatHUDWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace TacticalTurnEventNames
{
    static const FString TacticalTurnEnded = TEXT("tactical_turn_ended");
    static const FString TacticalActiveUnitChanged = TEXT("tactical_active_unit_changed");
}

ACRPGProjectPlayerController::ACRPGProjectPlayerController()
{
    // Keep orchestration components on the controller so possession changes do not discard tactical state.
    CameraControllerComponent = CreateDefaultSubobject<UCameraControllerComponent>(TEXT("CameraControllerComponent"));
    TacticalMovementControllerComponent = CreateDefaultSubobject<UTacticalMovementControllerComponent>(TEXT("TacticalMovementControllerComponent"));
    TacticalTurnSyncComponent = CreateDefaultSubobject<UTacticalTurnSyncComponent>(TEXT("TacticalTurnSyncComponent"));
    TacticalPathPreviewComponent = CreateDefaultSubobject<UTacticalPathPreviewComponent>(TEXT("TacticalPathPreviewComponent"));
    TacticalOutlineOverlayComponent = CreateDefaultSubobject<UTacticalOutlineOverlayComponent>(TEXT("TacticalOutlineOverlayComponent"));
    bAutoManageActiveCameraTarget = false;
    PrimaryActorTick.bCanEverTick = true;
}

void ACRPGProjectPlayerController::BeginPlay()
{
    Super::BeginPlay();
    BindToCameraModeSubsystem();
    BindToEventBus();
    SetControlledPawnTacticalInputSuppressed(IsTacticalModeActive());
    ApplyCameraModeInputState(IsTacticalModeActive() ? ECameraMode::Tactical : ECameraMode::Exploration);
    SpawnTacticalCombatHUD();
    if (TacticalOutlineOverlayComponent)
    {
        TacticalOutlineOverlayComponent->RefreshWorldOutlineOverlay();
    }
    RefreshTacticalCombatHUD();

    if (ShouldUseTouchControls() && IsLocalPlayerController())
    {
        MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

        if (MobileControlsWidget)
        {
            MobileControlsWidget->AddToPlayerScreen(0);
        }
        else
        {
            UE_LOG(LogCRPGProject, Error, TEXT("Could not spawn mobile controls widget."));
        }
    }
}

void ACRPGProjectPlayerController::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // Use the HUD's live pointer query here instead of the nested-widget hover depth counter.
    // The counter is still useful for clearing previews on UI entry, but it can remain > 0 after button clicks
    // while the cursor is already back over the world, which would block hover highlight updates until a click occurs.
    if (!IsLocalPlayerController() || CurrentTargetingMode != ECombatTargetingMode::EnemyUnit || IsPointerOverTacticalHUD())
    {
        SetHoveredCombatTarget(nullptr);
        return;
    }

    SetHoveredCombatTarget(ResolveUnitUnderCursor());
}

void ACRPGProjectPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearCombatTargetingMode();
    ClearPendingTacticalMovePreviewRequest();
    if (TacticalMovementControllerComponent)
    {
        TacticalMovementControllerComponent->ClearTacticalPathTraversal(true);
    }

    UnbindFromEventBus();
    UnbindFromCameraModeSubsystem();
    SetControlledPawnTacticalInputSuppressed(false);
    Super::EndPlay(EndPlayReason);
}

void ACRPGProjectPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ACRPGProjectPlayerController::ToggleDebugCameraMode);
        InputComponent->BindKey(EKeys::W, IE_Pressed, this, &ACRPGProjectPlayerController::HandleTacticalMoveForwardPressed);
        InputComponent->BindKey(EKeys::W, IE_Released, this, &ACRPGProjectPlayerController::HandleTacticalMoveForwardReleased);
        InputComponent->BindKey(EKeys::S, IE_Pressed, this, &ACRPGProjectPlayerController::HandleTacticalMoveBackwardPressed);
        InputComponent->BindKey(EKeys::S, IE_Released, this, &ACRPGProjectPlayerController::HandleTacticalMoveBackwardReleased);
        InputComponent->BindKey(EKeys::D, IE_Pressed, this, &ACRPGProjectPlayerController::HandleTacticalMoveRightPressed);
        InputComponent->BindKey(EKeys::D, IE_Released, this, &ACRPGProjectPlayerController::HandleTacticalMoveRightReleased);
        InputComponent->BindKey(EKeys::A, IE_Pressed, this, &ACRPGProjectPlayerController::HandleTacticalMoveLeftPressed);
        InputComponent->BindKey(EKeys::A, IE_Released, this, &ACRPGProjectPlayerController::HandleTacticalMoveLeftReleased);
        InputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &ACRPGProjectPlayerController::HandleTacticalZoom);
        InputComponent->BindAxisKey(EKeys::MouseX, this, &ACRPGProjectPlayerController::HandleTacticalMouseX);
        InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &ACRPGProjectPlayerController::HandleTacticalRightMousePressed);
        InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &ACRPGProjectPlayerController::HandleTacticalRightMouseReleased);
        if (TacticalOutlineOverlayComponent)
        {
            InputComponent->BindKey(EKeys::Tilde, IE_Pressed, TacticalOutlineOverlayComponent.Get(), &UTacticalOutlineOverlayComponent::HandleUnitOutlineOverlayPressed);
            InputComponent->BindKey(EKeys::Tilde, IE_Released, TacticalOutlineOverlayComponent.Get(), &UTacticalOutlineOverlayComponent::HandleUnitOutlineOverlayReleased);
            InputComponent->BindKey(EKeys::LeftAlt, IE_Pressed, TacticalOutlineOverlayComponent.Get(), &UTacticalOutlineOverlayComponent::HandleInteractableOutlineOverlayPressed);
            InputComponent->BindKey(EKeys::LeftAlt, IE_Released, TacticalOutlineOverlayComponent.Get(), &UTacticalOutlineOverlayComponent::HandleInteractableOutlineOverlayReleased);
            InputComponent->BindKey(EKeys::RightAlt, IE_Pressed, TacticalOutlineOverlayComponent.Get(), &UTacticalOutlineOverlayComponent::HandleInteractableOutlineOverlayPressed);
            InputComponent->BindKey(EKeys::RightAlt, IE_Released, TacticalOutlineOverlayComponent.Get(), &UTacticalOutlineOverlayComponent::HandleInteractableOutlineOverlayReleased);
        }
        InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ACRPGProjectPlayerController::HandleTacticalLeftMousePressed);
    }

    if (IsLocalPlayerController())
    {
        if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
        {
            for (UInputMappingContext *CurrentContext : DefaultMappingContexts)
            {
                Subsystem->AddMappingContext(CurrentContext, 0);
            }

            if (!ShouldUseTouchControls())
            {
                for (UInputMappingContext *CurrentContext : MobileExcludedMappingContexts)
                {
                    Subsystem->AddMappingContext(CurrentContext, 0);
                }
            }
        }
    }
}

void ACRPGProjectPlayerController::ToggleDebugCameraMode()
{
    if (!IsLocalPlayerController())
    {
        return;
    }

    if (const UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        if (TacticalTurnSubsystem->IsTurnModeActive())
        {
            return;
        }
    }

    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UCameraModeSubsystem *Subsystem = GameInstance->GetSubsystem<UCameraModeSubsystem>())
        {
            Subsystem->RequestToggleCameraMode();
        }
    }
}

void ACRPGProjectPlayerController::HandleTacticalMoveForwardPressed()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    TacticalMoveForwardInput = 1.0f;
    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalMoveForwardReleased()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    if (TacticalMoveForwardInput > 0.0f)
    {
        TacticalMoveForwardInput = 0.0f;
    }

    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalMoveBackwardPressed()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    TacticalMoveForwardInput = -1.0f;
    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalMoveBackwardReleased()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    if (TacticalMoveForwardInput < 0.0f)
    {
        TacticalMoveForwardInput = 0.0f;
    }

    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalMoveRightPressed()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    TacticalMoveRightInput = 1.0f;
    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalMoveRightReleased()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    if (TacticalMoveRightInput > 0.0f)
    {
        TacticalMoveRightInput = 0.0f;
    }

    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalMoveLeftPressed()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    TacticalMoveRightInput = -1.0f;
    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalMoveLeftReleased()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    if (TacticalMoveRightInput < 0.0f)
    {
        TacticalMoveRightInput = 0.0f;
    }

    UpdateTacticalRoamInput();
}

void ACRPGProjectPlayerController::HandleTacticalZoom(float AxisValue)
{
    if (!IsTacticalModeActive() || !CameraControllerComponent)
    {
        return;
    }

    CameraControllerComponent->AdjustTacticalZoom(AxisValue);
}

void ACRPGProjectPlayerController::HandleTacticalMouseX(float AxisValue)
{
    if (!IsTacticalModeActive() || !bIsTacticalRotateHeld || !CameraControllerComponent)
    {
        return;
    }

    CameraControllerComponent->AddTacticalYawInput(AxisValue);
}

void ACRPGProjectPlayerController::HandleTacticalRightMousePressed()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    if (CurrentTargetingMode != ECombatTargetingMode::None)
    {
        ClearCombatTargetingMode();
        RefreshTacticalCombatHUD();
        return;
    }

    if (TacticalMovementControllerComponent && TacticalMovementControllerComponent->HasActiveTacticalPath())
    {
        TacticalMovementControllerComponent->StopActiveTacticalMove(true);
        return;
    }

    bIsTacticalRotateHeld = true;
}

void ACRPGProjectPlayerController::HandleTacticalRightMouseReleased()
{
    bIsTacticalRotateHeld = false;
}

void ACRPGProjectPlayerController::HandleTacticalLeftMousePressed()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    HandleTacticalLeftClick();
}

void ACRPGProjectPlayerController::HandleTacticalLeftClick()
{
    if (CurrentTargetingMode != ECombatTargetingMode::None)
    {
        SetHoveredCombatTarget(ResolveUnitUnderCursor());
        if (TryExecutePendingCombatAction(HoveredTargetUnit.Get()))
        {
            ClearCombatTargetingMode();
        }

        RefreshTacticalCombatHUD();
        return;
    }

    if (TacticalMovementControllerComponent)
    {
        TacticalMovementControllerComponent->HandleTacticalLeftClick();
    }
}

float ACRPGProjectPlayerController::GetAvailableMovementBudgetForPlanning() const
{
    return TacticalMovementControllerComponent ? TacticalMovementControllerComponent->GetAvailableMovementBudgetForPlanning() : 0.0f;
}

bool ACRPGProjectPlayerController::SelectPlayerPartyUnit(ACRPGBaseCharacter *RequestedUnit)
{
    if (!IsLocalPlayerController() || !IsValid(RequestedUnit))
    {
        return false;
    }

    UTacticalUnitComponent *TacticalUnitComponent = RequestedUnit->GetTacticalUnitComponent();
    if (!TacticalUnitComponent || !TacticalUnitComponent->IsPlayerControlled() || !TacticalUnitComponent->IsAlive())
    {
        return false;
    }

    // Party selection is authoritative possession, not just camera focus, so the old pawn's transient
    // path state has to be discarded before we hand control to the newly selected ally.
    ClearCommittedTraversalControlPoints();
    ClearPendingTacticalMovePreviewRequest();

    if (TacticalMovementControllerComponent)
    {
        TacticalMovementControllerComponent->ClearTacticalPathTraversal(true);
    }

    if (GetPawn() != RequestedUnit)
    {
        StopMovement();
        Possess(RequestedUnit);
    }

    RefreshTacticalCombatHUD();
    return true;
}

bool ACRPGProjectPlayerController::FocusCameraOnTacticalUnit(ACRPGBaseCharacter *RequestedUnit)
{
    if (!IsLocalPlayerController() || !IsValid(RequestedUnit))
    {
        return false;
    }

    // Camera-only focus still invalidates the current preview because the hover ray origin changes.
    ClearPendingTacticalMovePreviewRequest();

    if (CameraControllerComponent && IsTacticalModeActive())
    {
        CameraControllerComponent->FocusTacticalPawn(RequestedUnit);
        return true;
    }

    SetViewTargetWithBlend(RequestedUnit, 0.0f);
    return true;
}

bool ACRPGProjectPlayerController::SelectOrFocusTacticalUnit(ACRPGBaseCharacter *RequestedUnit)
{
    if (!IsLocalPlayerController() || !IsValid(RequestedUnit))
    {
        return false;
    }

    const UTacticalUnitComponent *TacticalUnitComponent = RequestedUnit->GetTacticalUnitComponent();
    if (TacticalUnitComponent && TacticalUnitComponent->IsPlayerControlled() && TacticalUnitComponent->IsAlive())
    {
        // Allies should become the selected/possessed pawn so the HUD highlight tracks player intent.
        return SelectPlayerPartyUnit(RequestedUnit);
    }

    if (bAllowControllingNonPlayerTacticalUnits)
    {
        if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
        {
            ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit();
            if (RequestedUnit == ActiveUnit && TacticalTurnSubsystem->IsTurnModeActive() && TacticalUnitComponent && TacticalUnitComponent->IsAlive())
            {
                // Debug enemy control is intentionally narrow: only the current active non-player unit can be
                // repossessed, which lets testing continue without turning initiative order into free selection.
                ClearCommittedTraversalControlPoints();
                ClearPendingTacticalMovePreviewRequest();

                if (TacticalMovementControllerComponent)
                {
                    TacticalMovementControllerComponent->ClearTacticalPathTraversal(true);
                }

                if (GetPawn() != RequestedUnit)
                {
                    StopMovement();
                    Possess(RequestedUnit);
                }

                RefreshTacticalCombatHUD();
                return true;
            }
        }
    }

    return FocusCameraOnTacticalUnit(RequestedUnit);
}

bool ACRPGProjectPlayerController::IsPointerOverTacticalHUD() const
{
    // The controller does not infer UI hover itself; it delegates to the HUD, which knows which widgets are actionable.
    return TacticalCombatHUDWidget && TacticalCombatHUDWidget->IsPointerOverBlockingUI();
}

void ACRPGProjectPlayerController::NotifyTacticalUIHoverBegin()
{
    ++TacticalUIHoverDepth;
    ClearPendingTacticalMovePreviewRequest();
}

void ACRPGProjectPlayerController::NotifyTacticalUIHoverEnd()
{
    TacticalUIHoverDepth = FMath::Max(0, TacticalUIHoverDepth - 1);
}

bool ACRPGProjectPlayerController::IsHoveringTacticalUI() const
{
    return IsPointerOverTacticalHUD();
}

void ACRPGProjectPlayerController::BindToCameraModeSubsystem()
{
    if (!IsLocalPlayerController())
    {
        return;
    }

    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UCameraModeSubsystem *Subsystem = GameInstance->GetSubsystem<UCameraModeSubsystem>())
        {
            Subsystem->OnCameraModeChanged.RemoveAll(this);
            Subsystem->OnCameraModeChanged.AddDynamic(this, &ACRPGProjectPlayerController::HandleCameraModeChanged);
        }
    }
}

void ACRPGProjectPlayerController::BindToEventBus()
{
    if (!IsLocalPlayerController())
    {
        return;
    }

    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UEventBusSubsystem *EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
        {
            EventBusSubsystem->OnNamedGameEvent.RemoveAll(this);
            EventBusSubsystem->OnNamedGameEvent.AddUObject(this, &ACRPGProjectPlayerController::HandleGameEvent);
        }
    }
}

void ACRPGProjectPlayerController::UnbindFromEventBus()
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UEventBusSubsystem *EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
        {
            EventBusSubsystem->OnNamedGameEvent.RemoveAll(this);
        }
    }
}

void ACRPGProjectPlayerController::UnbindFromCameraModeSubsystem()
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        if (UCameraModeSubsystem *Subsystem = GameInstance->GetSubsystem<UCameraModeSubsystem>())
        {
            Subsystem->OnCameraModeChanged.RemoveAll(this);
        }
    }
}

void ACRPGProjectPlayerController::ApplyCameraModeInputState(ECameraMode CameraMode)
{
    const bool bIsTacticalMode = CameraMode == ECameraMode::Tactical;

    bShowMouseCursor = bIsTacticalMode;
    bEnableClickEvents = bIsTacticalMode;
    bEnableMouseOverEvents = bIsTacticalMode;

    if (bIsTacticalMode)
    {
        FInputModeGameAndUI InputMode;
        InputMode.SetHideCursorDuringCapture(false);
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        SetInputMode(InputMode);
        return;
    }

    SetInputMode(FInputModeGameOnly());
}

UTacticalTurnSubsystem *ACRPGProjectPlayerController::GetTacticalTurnSubsystem() const
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        return GameInstance->GetSubsystem<UTacticalTurnSubsystem>();
    }

    return nullptr;
}

void ACRPGProjectPlayerController::RefreshTacticalCombatHUD() const
{
    if (TacticalCombatHUDWidget)
    {
        TacticalCombatHUDWidget->RefreshFromSubsystem();
    }
}

void ACRPGProjectPlayerController::SpawnTacticalCombatHUD()
{
    if (!IsLocalPlayerController() || TacticalCombatHUDWidget || !TacticalCombatHUDWidgetClass)
    {
        return;
    }

    TacticalCombatHUDWidget = CreateWidget<UTacticalCombatHUDWidget>(this, TacticalCombatHUDWidgetClass);
    if (TacticalCombatHUDWidget)
    {
        TacticalCombatHUDWidget->AddToPlayerScreen(1);
    }
}

void ACRPGProjectPlayerController::HandleCameraModeChanged(const FCameraModeTransition &Transition)
{
    // Camera mode changes invalidate any in-progress preview or traversal because the input model changes.
    bIsTacticalRotateHeld = false;
    ClearCommittedTraversalControlPoints();
    ClearPendingTacticalMovePreviewRequest();
    ApplyCameraModeInputState(Transition.NewMode);
    SetControlledPawnTacticalInputSuppressed(Transition.NewMode == ECameraMode::Tactical);
    RefreshTacticalCombatHUD();

    if (TacticalMovementControllerComponent)
    {
        TacticalMovementControllerComponent->ClearTacticalPathTraversal(true);
    }

    if (Transition.NewMode == ECameraMode::Tactical)
    {
        // Entering tactical mode should leave the current pawn stationary before click-to-move takes over.
        StopMovement();
        StopTacticalPrototypeMovement();
        return;
    }

    if (Transition.NewMode != ECameraMode::Tactical)
    {
        // Leaving tactical mode clears cached roam axes so exploration input resumes from a neutral state.
        TacticalMoveForwardInput = 0.0f;
        TacticalMoveRightInput = 0.0f;

        if (CameraControllerComponent)
        {
            CameraControllerComponent->ClearTacticalRoamInput();
        }
    }
}

void ACRPGProjectPlayerController::HandleGameEvent(const FString &EventName, const FString &Payload)
{
    if (TacticalTurnSyncComponent)
    {
        TacticalTurnSyncComponent->HandleGameEvent(EventName, Payload);
    }

    if (EventName == TacticalHUDActionEvents::MeleeAttackRequested)
    {
        EnterCombatTargetingMode(ECombatActionType::MeleeAttack);
        RefreshTacticalCombatHUD();
        return;
    }

    if (EventName == TacticalHUDActionEvents::RangedAttackRequested)
    {
        EnterCombatTargetingMode(ECombatActionType::RangedAttack);
        RefreshTacticalCombatHUD();
        return;
    }

    if (TacticalOutlineOverlayComponent)
    {
        TacticalOutlineOverlayComponent->HandleGameEvent(EventName, Payload);
    }

    if (EventName == TacticalTurnEventNames::TacticalTurnEnded || EventName == TacticalTurnEventNames::TacticalActiveUnitChanged)
    {
        ClearCombatTargetingMode();
        RefreshTacticalCombatHUD();
        return;
    }
}

void ACRPGProjectPlayerController::SyncPossessionToActiveTacticalUnit()
{
    if (TacticalTurnSyncComponent)
    {
        TacticalTurnSyncComponent->SyncPossessionToActiveTacticalUnit();
    }
}

void ACRPGProjectPlayerController::RestorePossessionAfterTacticalTurn()
{
    if (TacticalTurnSyncComponent)
    {
        TacticalTurnSyncComponent->RestorePossessionAfterTacticalTurn();
    }
}

void ACRPGProjectPlayerController::OnPossess(APawn *InPawn)
{
    Super::OnPossess(InPawn);
    ClearCombatTargetingMode();
    ClearCommittedTraversalControlPoints();
    ClearPendingTacticalMovePreviewRequest();
    if (TacticalOutlineOverlayComponent)
    {
        TacticalOutlineOverlayComponent->RefreshWorldOutlineOverlay();
    }
    RefreshTacticalCombatHUD();
}

void ACRPGProjectPlayerController::OnUnPossess()
{
    ClearCombatTargetingMode();
    ClearCommittedTraversalControlPoints();
    ClearPendingTacticalMovePreviewRequest();
    if (TacticalOutlineOverlayComponent)
    {
        TacticalOutlineOverlayComponent->RefreshWorldOutlineOverlay();
    }
    Super::OnUnPossess();
    RefreshTacticalCombatHUD();
}

void ACRPGProjectPlayerController::UpdateTacticalRoamInput()
{
    if (!CameraControllerComponent)
    {
        return;
    }

    const FVector2D RoamInput(TacticalMoveRightInput, TacticalMoveForwardInput);
    if (RoamInput.IsNearlyZero())
    {
        CameraControllerComponent->ClearTacticalRoamInput();
        return;
    }

    CameraControllerComponent->AddTacticalRoamInput(RoamInput);
}

void ACRPGProjectPlayerController::SetControlledPawnTacticalInputSuppressed(bool bSuppressInput)
{
    if (bSuppressInput)
    {
        if (ACharacter *ControlledCharacter = Cast<ACharacter>(GetPawn()))
        {
            ControlledCharacter->StopJumping();
            ControlledCharacter->GetCharacterMovement()->StopMovementImmediately();
        }
    }
}

void ACRPGProjectPlayerController::StopTacticalPrototypeMovement()
{
    if (ACharacter *ControlledCharacter = Cast<ACharacter>(GetPawn()))
    {
        ControlledCharacter->StopJumping();
        ControlledCharacter->GetCharacterMovement()->StopMovementImmediately();
    }

    StopMovement();
}

bool ACRPGProjectPlayerController::IsTacticalModeActive() const
{
    return CameraControllerComponent && CameraControllerComponent->GetActiveCameraMode() == ECameraMode::Tactical;
}

TArray<FVector> ACRPGProjectPlayerController::BuildDenseResampledPath(const TArray<FVector> &RawNavPoints, float SampleSpacing) const
{
    TArray<FVector> DensePath;
    if (RawNavPoints.Num() == 0)
    {
        return DensePath;
    }

    DensePath.Reserve(RawNavPoints.Num());
    DensePath.Add(RawNavPoints[0]);

    const float EffectiveSampleSpacing = FMath::Max(1.0f, SampleSpacing);
    for (int32 PointIndex = 1; PointIndex < RawNavPoints.Num(); ++PointIndex)
    {
        const FVector SegmentStart = RawNavPoints[PointIndex - 1];
        const FVector SegmentEnd = RawNavPoints[PointIndex];
        const float SegmentLength = FVector::Distance(SegmentStart, SegmentEnd);

        if (SegmentLength <= KINDA_SMALL_NUMBER)
        {
            if (!DensePath.Last().Equals(SegmentEnd, KINDA_SMALL_NUMBER))
            {
                DensePath.Add(SegmentEnd);
            }

            continue;
        }

        const FVector SegmentDirection = (SegmentEnd - SegmentStart) / SegmentLength;
        for (float DistanceAlongSegment = EffectiveSampleSpacing; DistanceAlongSegment < SegmentLength; DistanceAlongSegment += EffectiveSampleSpacing)
        {
            DensePath.Add(SegmentStart + (SegmentDirection * DistanceAlongSegment));
        }

        DensePath.Add(SegmentEnd);
    }

    return DensePath;
}

FVector ACRPGProjectPlayerController::RelaxPreviewPointAgainstUnits(const FVector &SamplePoint, const FVector &PathDirection, const ACRPGBaseCharacter *MovingCharacter) const
{
    const UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    if (!TacticalTurnSubsystem)
    {
        return SamplePoint;
    }

    FVector FlattenedDirection = PathDirection;
    FlattenedDirection.Z = 0.0f;
    if (!FlattenedDirection.Normalize())
    {
        return SamplePoint;
    }

    // Occupancy is resolved from tactical unit data rather than physics capsules so movement rules stay logical.
    struct FOccupiedUnitData
    {
        FVector Location;
        float Radius = 0.0f;
    };

    TArray<FOccupiedUnitData> OccupiedUnits;
    OccupiedUnits.Reserve(TacticalTurnSubsystem->GetInitiativeOrder().Num());

    for (UTacticalUnitComponent *TacticalUnitComponent : TacticalTurnSubsystem->GetInitiativeOrder())
    {
        if (!TacticalUnitComponent || !TacticalUnitComponent->IsOccupyingTacticalSpace())
        {
            continue;
        }

        const ACRPGBaseCharacter *OtherCharacter = Cast<ACRPGBaseCharacter>(TacticalUnitComponent->GetOwner());
        if (!OtherCharacter || OtherCharacter == MovingCharacter)
        {
            continue;
        }

        FOccupiedUnitData &OccupiedUnit = OccupiedUnits.AddDefaulted_GetRef();
        OccupiedUnit.Location = TacticalUnitComponent->GetOccupiedLocation();
        OccupiedUnit.Radius = TacticalUnitComponent->GetOccupancyRadiusCm() + GetTacticalAvoidMargin();
    }

    if (OccupiedUnits.Num() == 0)
    {
        return SamplePoint;
    }

    const auto IsOutsideOccupiedUnits = [&OccupiedUnits](const FVector &CandidatePoint)
    {
        for (const FOccupiedUnitData &OccupiedUnit : OccupiedUnits)
        {
            if (FVector::Dist2D(CandidatePoint, OccupiedUnit.Location) < OccupiedUnit.Radius)
            {
                return false;
            }
        }

        return true;
    };

    if (IsOutsideOccupiedUnits(SamplePoint))
    {
        return SamplePoint;
    }

    // Try a small set of perpendicular offsets and keep the first point that exits all occupied footprints.
    const FVector LeftPerpendicular = FVector::CrossProduct(FVector::UpVector, FlattenedDirection).GetSafeNormal();
    const FVector RightPerpendicular = -LeftPerpendicular;
    const float CandidateOffsetsCm[] = {
        GetTacticalAvoidTestOffsetSmall(),
        GetTacticalAvoidTestOffsetMedium(),
        GetTacticalAvoidTestOffsetLarge()};

    for (float CandidateOffsetCm : CandidateOffsetsCm)
    {
        const FVector LeftCandidate = SamplePoint + (LeftPerpendicular * CandidateOffsetCm);
        if (IsOutsideOccupiedUnits(LeftCandidate))
        {
            return LeftCandidate;
        }

        const FVector RightCandidate = SamplePoint + (RightPerpendicular * CandidateOffsetCm);
        if (IsOutsideOccupiedUnits(RightCandidate))
        {
            return RightCandidate;
        }
    }

    return SamplePoint;
}

TArray<FVector> ACRPGProjectPlayerController::ApplyOccupancyRelaxationToPath(const TArray<FVector> &DensePath, const ACRPGBaseCharacter *MovingCharacter) const
{
    if (DensePath.Num() < 3 || !MovingCharacter)
    {
        return DensePath;
    }

    // Preserve endpoints from navigation and only relax interior samples, which keeps start and destination stable.
    TArray<FVector> AdjustedPath = DensePath;
    for (int32 PointIndex = 1; PointIndex < AdjustedPath.Num() - 1; ++PointIndex)
    {
        FVector PathDirection = AdjustedPath[PointIndex + 1] - AdjustedPath[PointIndex - 1];
        PathDirection.Z = 0.0f;

        if (PathDirection.IsNearlyZero())
        {
            PathDirection = DensePath[PointIndex + 1] - DensePath[PointIndex];
            PathDirection.Z = 0.0f;
        }

        AdjustedPath[PointIndex] = RelaxPreviewPointAgainstUnits(AdjustedPath[PointIndex], PathDirection, MovingCharacter);
    }

    return AdjustedPath;
}

TArray<FVector> ACRPGProjectPlayerController::BuildSmoothedPreviewPath(const TArray<FVector> &RelaxedPath, int32 SubdivisionsPerSegment) const
{
    if (RelaxedPath.Num() < 2)
    {
        return RelaxedPath;
    }

    const int32 EffectiveSubdivisions = FMath::Max(0, SubdivisionsPerSegment);
    if (EffectiveSubdivisions == 0)
    {
        return RelaxedPath;
    }

    // Catmull-Rom gives a readable preview without changing the committed traversal control path structure.
    auto EvaluateCatmullRomPoint = [](const FVector &Point0, const FVector &Point1, const FVector &Point2, const FVector &Point3, float T)
    {
        const float T2 = T * T;
        const float T3 = T2 * T;

        return 0.5f * ((2.0f * Point1) + ((-Point0 + Point2) * T) + ((2.0f * Point0) - (5.0f * Point1) + (4.0f * Point2) - Point3) * T2 + ((-Point0) + (3.0f * Point1) - (3.0f * Point2) + Point3) * T3);
    };

    TArray<FVector> SmoothedPath;
    SmoothedPath.Reserve((RelaxedPath.Num() - 1) * (EffectiveSubdivisions + 1) + 1);
    SmoothedPath.Add(RelaxedPath[0]);

    for (int32 SegmentIndex = 0; SegmentIndex < RelaxedPath.Num() - 1; ++SegmentIndex)
    {
        const FVector &Point0 = RelaxedPath[FMath::Max(0, SegmentIndex - 1)];
        const FVector &Point1 = RelaxedPath[SegmentIndex];
        const FVector &Point2 = RelaxedPath[SegmentIndex + 1];
        const FVector &Point3 = RelaxedPath[FMath::Min(RelaxedPath.Num() - 1, SegmentIndex + 2)];

        for (int32 SubdivisionIndex = 1; SubdivisionIndex <= EffectiveSubdivisions; ++SubdivisionIndex)
        {
            const float T = static_cast<float>(SubdivisionIndex) / static_cast<float>(EffectiveSubdivisions + 1);
            FVector SmoothedPoint = EvaluateCatmullRomPoint(Point0, Point1, Point2, Point3, T);

            const FVector NeighborhoodMin(
                FMath::Min(FMath::Min(Point0.X, Point1.X), FMath::Min(Point2.X, Point3.X)),
                FMath::Min(FMath::Min(Point0.Y, Point1.Y), FMath::Min(Point2.Y, Point3.Y)),
                FMath::Min(FMath::Min(Point0.Z, Point1.Z), FMath::Min(Point2.Z, Point3.Z)));
            const FVector NeighborhoodMax(
                FMath::Max(FMath::Max(Point0.X, Point1.X), FMath::Max(Point2.X, Point3.X)),
                FMath::Max(FMath::Max(Point0.Y, Point1.Y), FMath::Max(Point2.Y, Point3.Y)),
                FMath::Max(FMath::Max(Point0.Z, Point1.Z), FMath::Max(Point2.Z, Point3.Z)));

            SmoothedPoint.X = FMath::Clamp(SmoothedPoint.X, NeighborhoodMin.X, NeighborhoodMax.X);
            SmoothedPoint.Y = FMath::Clamp(SmoothedPoint.Y, NeighborhoodMin.Y, NeighborhoodMax.Y);
            SmoothedPoint.Z = FMath::Clamp(SmoothedPoint.Z, NeighborhoodMin.Z, NeighborhoodMax.Z);
            SmoothedPath.Add(SmoothedPoint);
        }

        SmoothedPath.Add(Point2);
    }

    return SmoothedPath;
}

TArray<FVector> ACRPGProjectPlayerController::BuildTraversalControlPath(const TArray<FVector> &VisualPreviewPath, float ControlPointSpacing) const
{
    // Traversal uses a sparser path than the debug preview to avoid jitter from tiny visual subdivisions.
    TArray<FVector> TraversalControlPath;
    if (VisualPreviewPath.Num() == 0)
    {
        return TraversalControlPath;
    }

    TraversalControlPath.Add(VisualPreviewPath[0]);
    if (VisualPreviewPath.Num() == 1)
    {
        return TraversalControlPath;
    }

    const float EffectiveControlPointSpacing = FMath::Max(1.0f, ControlPointSpacing);
    float AccumulatedDistanceCm = 0.0f;

    for (int32 PointIndex = 1; PointIndex < VisualPreviewPath.Num(); ++PointIndex)
    {
        const float SegmentDistanceCm = FVector::Distance(VisualPreviewPath[PointIndex - 1], VisualPreviewPath[PointIndex]);
        AccumulatedDistanceCm += SegmentDistanceCm;

        if (AccumulatedDistanceCm >= EffectiveControlPointSpacing)
        {
            TraversalControlPath.Add(VisualPreviewPath[PointIndex]);
            AccumulatedDistanceCm = 0.0f;
        }
    }

    if (!TraversalControlPath.Last().Equals(VisualPreviewPath.Last(), KINDA_SMALL_NUMBER))
    {
        TraversalControlPath.Add(VisualPreviewPath.Last());
    }

    return TraversalControlPath;
}

FVector ACRPGProjectPlayerController::GetTraversalLookAheadTarget(const FVector &CurrentLocation, int32 CurrentPathIndex) const
{
    // Look ahead beyond the current node so the pawn keeps flowing through corners instead of stopping at every point.
    if (!CommittedTraversalControlPoints.IsValidIndex(CurrentPathIndex))
    {
        return CommittedTraversalControlPoints.Num() > 0 ? CommittedTraversalControlPoints.Last() : CurrentLocation;
    }

    const float RequiredLookAheadDistance = FMath::Max(1.0f, GetTraversalLookAheadDistance());
    for (int32 PointIndex = CurrentPathIndex; PointIndex < CommittedTraversalControlPoints.Num(); ++PointIndex)
    {
        if (FVector::Dist2D(CurrentLocation, CommittedTraversalControlPoints[PointIndex]) >= RequiredLookAheadDistance)
        {
            return CommittedTraversalControlPoints[PointIndex];
        }
    }

    return CommittedTraversalControlPoints.Last();
}

void ACRPGProjectPlayerController::SetCommittedTraversalControlPoints(const TArray<FVector> &PathPoints)
{
    CommittedTraversalControlPoints = PathPoints;
}

void ACRPGProjectPlayerController::ClearCommittedTraversalControlPoints()
{
    CommittedTraversalControlPoints.Reset();
}

const TArray<FVector> &ACRPGProjectPlayerController::GetCommittedTraversalControlPoints() const
{
    return CommittedTraversalControlPoints;
}

const FTacticalMovePreviewData &ACRPGProjectPlayerController::GetPendingMovePreview() const
{
    static const FTacticalMovePreviewData EmptyPreview;
    return TacticalMovementControllerComponent ? TacticalMovementControllerComponent->GetPendingMovePreview() : EmptyPreview;
}

void ACRPGProjectPlayerController::CommitPendingTacticalMoveRequest()
{
    if (TacticalMovementControllerComponent)
    {
        TacticalMovementControllerComponent->CommitPendingTacticalMove();
    }
}

void ACRPGProjectPlayerController::ClearPendingTacticalMovePreviewRequest()
{
    if (TacticalMovementControllerComponent)
    {
        TacticalMovementControllerComponent->ClearPendingTacticalMovePreview();
    }
}

bool ACRPGProjectPlayerController::IsTurnModeMovementEnabled() const
{
    return TacticalTurnSyncComponent ? TacticalTurnSyncComponent->IsTurnModeMovementEnabled() : true;
}

void ACRPGProjectPlayerController::SetTurnModeMovementEnabled(bool bEnabled)
{
    if (TacticalTurnSyncComponent)
    {
        TacticalTurnSyncComponent->SetTurnModeMovementEnabled(bEnabled);
    }
}

void ACRPGProjectPlayerController::ToggleTurnModeMovementEnabled()
{
    if (TacticalTurnSyncComponent)
    {
        TacticalTurnSyncComponent->ToggleTurnModeMovementEnabled();
    }
}

float ACRPGProjectPlayerController::GetAvailableMovementBudgetForPlanningRequest() const
{
    return GetAvailableMovementBudgetForPlanning();
}

bool ACRPGProjectPlayerController::ShouldUseTouchControls() const
{
    return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

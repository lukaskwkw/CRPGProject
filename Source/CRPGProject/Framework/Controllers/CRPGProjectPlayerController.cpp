// Copyright Epic Games, Inc. All Rights Reserved.

#include "CRPGProjectPlayerController.h"

#include "CameraControllerComponent.h"
#include "CameraModeSubsystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Events/Subsystems/EventBusSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Blueprint/UserWidget.h"
#include "CRPGProject.h"
#include "Tactical/Components/TacticalPathPreviewComponent.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"
#include "UI/HUD/TacticalCombatHUDWidget.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace TacticalCombatEvents
{
	static const FString TacticalTurnStarted = TEXT("tactical_turn_started");
	static const FString TacticalTurnEnded = TEXT("tactical_turn_ended");
	static const FString TacticalEncounterStarted = TEXT("tactical_encounter_started");
	static const FString TacticalRoundStarted = TEXT("tactical_round_started");
	static const FString TacticalActiveUnitChanged = TEXT("tactical_active_unit_changed");
	static const FString TacticalUnitMovementConsumed = TEXT("tactical_unit_movement_consumed");
	static const FString CameraModeChanged = TEXT("camera_mode_changed");
}

ACRPGProjectPlayerController::ACRPGProjectPlayerController()
{
	CameraControllerComponent = CreateDefaultSubobject<UCameraControllerComponent>(TEXT("CameraControllerComponent"));
	TacticalPathPreviewComponent = CreateDefaultSubobject<UTacticalPathPreviewComponent>(TEXT("TacticalPathPreviewComponent"));
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
	RefreshTacticalCombatHUD();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
		}
		else
		{

			UE_LOG(LogCRPGProject, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}
}

void ACRPGProjectPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearPendingTacticalMovePreview();
	ClearTacticalPathTraversal(true);
	UnbindFromEventBus();
	UnbindFromCameraModeSubsystem();
	SetControlledPawnTacticalInputSuppressed(false);
	Super::EndPlay(EndPlayReason);
}

void ACRPGProjectPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTacticalPathTraversal(DeltaTime);
	UpdateTacticalMovePreviewFromHover();
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
		InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &ACRPGProjectPlayerController::HandleTacticalLeftMousePressed);
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem *Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext *CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
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

	if (bHasActiveTacticalPath)
	{
		StopActiveTacticalMove(true);
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
	FHitResult HitResult;
	if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult) || !HitResult.bBlockingHit)
	{
		return;
	}

	APawn *ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector ClickedLocation = HitResult.ImpactPoint;

	if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
	{
		if (TacticalTurnSubsystem->IsTurnModeActive())
		{
			if (!bTurnModeMovementEnabled)
			{
				return;
			}

			if (bHasActiveTacticalPath)
			{
				StopActiveTacticalMove(true);
			}

			if (!PendingMovePreview.bHasPreview || !IsClickNearPendingDestination(ClickedLocation))
			{
				BuildTacticalMovePreview(ClickedLocation);
			}

			CommitPendingTacticalMove();
			return;
		}
	}

	UNavigationSystemV1 *NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		return;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(ClickedLocation, ProjectedLocation))
	{
		ClearTacticalPathTraversal(true);
		return;
	}

	UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Destination chosen: %s"), *ProjectedLocation.Location.ToString());

	UNavigationPath *NavigationPath = NavigationSystem->FindPathToLocationSynchronously(GetWorld(), ControlledPawn->GetActorLocation(), ProjectedLocation.Location, ControlledPawn);
	if (!NavigationPath || !NavigationPath->IsValid() || NavigationPath->PathPoints.Num() == 0)
	{
		ClearTacticalPathTraversal(true);
		return;
	}

	UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Path point count: %d"), NavigationPath->PathPoints.Num());
	StartTacticalPathTraversal(NavigationPath->PathPoints);
}

void ACRPGProjectPlayerController::BuildTacticalMovePreview(const FVector &Destination)
{
	if (!bTurnModeMovementEnabled)
	{
		ClearPendingTacticalMovePreview();
		return;
	}

	APawn *ControlledPawn = GetPawn();
	UNavigationSystemV1 *NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!ControlledPawn || !NavigationSystem)
	{
		ClearPendingTacticalMovePreview();
		return;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(Destination, ProjectedLocation))
	{
		ClearPendingTacticalMovePreview();
		return;
	}

	UNavigationPath *NavigationPath = NavigationSystem->FindPathToLocationSynchronously(GetWorld(), ControlledPawn->GetActorLocation(), ProjectedLocation.Location, ControlledPawn);
	if (!NavigationPath || !NavigationPath->IsValid() || NavigationPath->PathPoints.Num() == 0)
	{
		ClearPendingTacticalMovePreview();
		return;
	}

	// Keep the raw nav path and its affordable subset together so the HUD and renderer stay in sync.
	PendingMovePreview = FTacticalMovePreviewData();
	PendingMovePreview.bHasPreview = true;
	PendingMovePreview.Destination = ProjectedLocation.Location;
	PendingMovePreview.PathPoints = NavigationPath->PathPoints;
	PendingMovePreview.PathDistanceCm = CalculatePathDistance(PendingMovePreview.PathPoints);
	PendingMovePreview.bIsAffordable = true;
	PendingMovePreview.AffordablePathPoints = PendingMovePreview.PathPoints;
	PendingMovePreview.AffordablePathDistanceCm = PendingMovePreview.PathDistanceCm;
	PendingMovePreview.bHasOverBudgetSegment = false;

	if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
	{
		if (TacticalTurnSubsystem->IsTurnModeActive())
		{
			const ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit();
			const UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit ? ActiveUnit->GetTacticalUnitComponent() : nullptr;

			if (!TacticalUnitComponent)
			{
				ClearPendingTacticalMovePreview();
				return;
			}

			const float RemainingMovementRangeCm = GetAvailableMovementBudgetForPlanning();
			PendingMovePreview.bIsAffordable = TacticalUnitComponent->HasMovementBudget(PendingMovePreview.PathDistanceCm);
			PendingMovePreview.bIsAffordable = PendingMovePreview.PathDistanceCm <= RemainingMovementRangeCm + KINDA_SMALL_NUMBER;
			PendingMovePreview.bHasOverBudgetSegment = !PendingMovePreview.bIsAffordable;
			// Clamp the rendered path to the budget, but keep the full nav path for over-budget visualization.
			BuildClampedTacticalPath(PendingMovePreview.PathPoints, RemainingMovementRangeCm, PendingMovePreview.AffordablePathPoints, PendingMovePreview.AffordablePathDistanceCm);
		}
	}

	if (TacticalPathPreviewComponent)
	{
		// The preview component only draws cached world-space points; all path math is finalized here.
		TacticalPathPreviewComponent->RenderPath(PendingMovePreview.PathPoints, PendingMovePreview.AffordablePathDistanceCm);
	}

	RefreshTacticalCombatHUD();
}

void ACRPGProjectPlayerController::BuildClampedTacticalPath(const TArray<FVector> &SourcePathPoints, float MaxDistanceCm, TArray<FVector> &OutPathPoints, float &OutDistanceCm) const
{
	OutPathPoints.Reset();
	OutDistanceCm = 0.0f;

	if (SourcePathPoints.Num() == 0)
	{
		return;
	}

	OutPathPoints.Add(SourcePathPoints[0]);

	const float ClampedMaxDistanceCm = FMath::Max(0.0f, MaxDistanceCm);
	for (int32 PointIndex = 1; PointIndex < SourcePathPoints.Num(); ++PointIndex)
	{
		const FVector SegmentStart = SourcePathPoints[PointIndex - 1];
		const FVector SegmentEnd = SourcePathPoints[PointIndex];
		const float SegmentDistanceCm = FVector::Distance(SegmentStart, SegmentEnd);

		if (OutDistanceCm + SegmentDistanceCm <= ClampedMaxDistanceCm + KINDA_SMALL_NUMBER)
		{
			OutPathPoints.Add(SegmentEnd);
			OutDistanceCm += SegmentDistanceCm;
			continue;
		}

		const float RemainingDistanceCm = ClampedMaxDistanceCm - OutDistanceCm;
		if (RemainingDistanceCm > KINDA_SMALL_NUMBER && SegmentDistanceCm > KINDA_SMALL_NUMBER)
		{
			const float Alpha = FMath::Clamp(RemainingDistanceCm / SegmentDistanceCm, 0.0f, 1.0f);
			OutPathPoints.Add(FMath::Lerp(SegmentStart, SegmentEnd, Alpha));
			OutDistanceCm = ClampedMaxDistanceCm;
		}

		break;
	}
}

void ACRPGProjectPlayerController::CommitPendingTacticalMove()
{
	if (!PendingMovePreview.bHasPreview || PendingMovePreview.AffordablePathPoints.Num() < 2 || PendingMovePreview.AffordablePathDistanceCm <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (PendingMovePreview.AffordablePathDistanceCm <= TacticalMinimumCommittedMoveDistance)
	{
		ClearPendingTacticalMovePreview();
		return;
	}

	// Freeze the current preview into a traversal request before clearing the transient preview state.
	const TArray<FVector> PathPoints = PendingMovePreview.AffordablePathPoints;
	const float PathDistanceCm = PendingMovePreview.AffordablePathDistanceCm;

	if (bHasActiveTacticalPath)
	{
		StopActiveTacticalMove(true);
	}

	if (TacticalPathPreviewComponent)
	{
		TacticalPathPreviewComponent->ClearPath();
	}

	PendingMovePreview = FTacticalMovePreviewData();
	StartTacticalPathTraversal(PathPoints, PathDistanceCm);
	RefreshTacticalCombatHUD();
}

void ACRPGProjectPlayerController::ClearPendingTacticalMovePreview()
{
	// Clearing both the cached preview data and the debug renderer keeps hover previews stateless.
	PendingMovePreview = FTacticalMovePreviewData();

	if (TacticalPathPreviewComponent)
	{
		TacticalPathPreviewComponent->ClearPath();
	}

	RefreshTacticalCombatHUD();
}

bool ACRPGProjectPlayerController::IsClickNearPendingDestination(const FVector &ClickedLocation) const
{
	return PendingMovePreview.bHasPreview && FVector::Dist2D(PendingMovePreview.Destination, ClickedLocation) <= 100.0f;
}

bool ACRPGProjectPlayerController::TryGetTacticalCursorWorldLocation(FVector &OutWorldLocation) const
{
	FHitResult HitResult;
	if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult) || !HitResult.bBlockingHit)
	{
		return false;
	}

	OutWorldLocation = HitResult.ImpactPoint;
	return true;
}

void ACRPGProjectPlayerController::UpdateTacticalMovePreviewFromHover()
{
	if (!IsTacticalModeActive())
	{
		return;
	}

	UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
	if (!TacticalTurnSubsystem || !TacticalTurnSubsystem->IsTurnModeActive() || !bTurnModeMovementEnabled)
	{
		if (PendingMovePreview.bHasPreview)
		{
			ClearPendingTacticalMovePreview();
		}

		return;
	}

	FVector HoveredWorldLocation = FVector::ZeroVector;
	if (!TryGetTacticalCursorWorldLocation(HoveredWorldLocation))
	{
		if (PendingMovePreview.bHasPreview)
		{
			ClearPendingTacticalMovePreview();
		}

		return;
	}

	if (PendingMovePreview.bHasPreview && FVector::Dist2D(PendingMovePreview.Destination, HoveredWorldLocation) <= TacticalHoverPreviewRefreshTolerance)
	{
		// Avoid rebuilding the same nav path every tick while the cursor is effectively stationary.
		if (GetWorld() && GetWorld()->GetTimeSeconds() < NextTacticalHoverPreviewRefreshTime)
		{
			return;
		}
	}

	BuildTacticalMovePreview(HoveredWorldLocation);

	if (GetWorld())
	{
		NextTacticalHoverPreviewRefreshTime = GetWorld()->GetTimeSeconds() + TacticalHoverPreviewRefreshInterval;
	}
}

void ACRPGProjectPlayerController::StopActiveTacticalMove(bool bConsumeTravelledDistance)
{
	if (!bHasActiveTacticalPath)
	{
		return;
	}

	if (const APawn *ControlledPawn = GetPawn())
	{
		ActiveTraversalTravelledDistanceCm = FMath::Min(
			PendingPathDistanceConsumption,
			ActiveTraversalTravelledDistanceCm + FVector::Distance(LastTraversalPawnLocation, ControlledPawn->GetActorLocation()));
		LastTraversalPawnLocation = ControlledPawn->GetActorLocation();
	}

	if (bConsumeTravelledDistance && ActiveTraversalTravelledDistanceCm > KINDA_SMALL_NUMBER)
	{
		PendingPathDistanceConsumption = FMath::Min(PendingPathDistanceConsumption, ActiveTraversalTravelledDistanceCm);
		HandleTacticalPathTraversalCompleted();
	}

	ClearTacticalPathTraversal(true);
}

float ACRPGProjectPlayerController::GetReservedMovementDistanceCm() const
{
	return bHasActiveTacticalPath ? FMath::Max(0.0f, ActiveTraversalTravelledDistanceCm) : 0.0f;
}

float ACRPGProjectPlayerController::GetAvailableMovementBudgetForPlanning() const
{
	if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
	{
		if (TacticalTurnSubsystem->IsTurnModeActive())
		{
			if (const ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit())
			{
				if (const UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit->GetTacticalUnitComponent())
				{
					return FMath::Max(0.0f, TacticalUnitComponent->GetRemainingMovementRange() - GetReservedMovementDistanceCm());
				}
			}
		}
	}

	return 0.0f;
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

void ACRPGProjectPlayerController::StartTacticalPathTraversal(const TArray<FVector> &PathPoints, float PendingDistanceConsumption)
{
	ActiveTacticalPathPoints = PathPoints;
	bHasActiveTacticalPath = ActiveTacticalPathPoints.Num() > 0;
	CurrentPathIndex = ActiveTacticalPathPoints.Num() > 1 ? 1 : 0;
	PendingPathDistanceConsumption = FMath::Max(0.0f, PendingDistanceConsumption);
	ActiveTraversalTravelledDistanceCm = 0.0f;
	LastTraversalPawnLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;

	UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Path traversal started. Active points: %d, starting index: %d"), ActiveTacticalPathPoints.Num(), CurrentPathIndex);
}

void ACRPGProjectPlayerController::UpdateTacticalPathTraversal(float DeltaTime)
{
	if (!bHasActiveTacticalPath || !IsTacticalModeActive())
	{
		return;
	}

	ACharacter *ControlledCharacter = Cast<ACharacter>(GetPawn());
	if (!ControlledCharacter || !ActiveTacticalPathPoints.IsValidIndex(CurrentPathIndex))
	{
		ClearTacticalPathTraversal(false);
		return;
	}

	const FVector PawnLocation = ControlledCharacter->GetActorLocation();
	ActiveTraversalTravelledDistanceCm = FMath::Min(
		PendingPathDistanceConsumption,
		ActiveTraversalTravelledDistanceCm + FVector::Distance(LastTraversalPawnLocation, PawnLocation));
	LastTraversalPawnLocation = PawnLocation;

	const FVector TargetPoint = ActiveTacticalPathPoints[CurrentPathIndex];
	FVector ToTarget = TargetPoint - PawnLocation;
	ToTarget.Z = 0.0f;
	const bool bIsFinalPathPoint = CurrentPathIndex == ActiveTacticalPathPoints.Num() - 1;
	const float CurrentAcceptanceRadius = bIsFinalPathPoint ? FMath::Min(TacticalAcceptanceRadius, TacticalFinalAcceptanceRadius) : TacticalAcceptanceRadius;

	if (ToTarget.SizeSquared() <= FMath::Square(CurrentAcceptanceRadius))
	{
		UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Reached path node %d at %s"), CurrentPathIndex, *TargetPoint.ToString());
		++CurrentPathIndex;

		if (!ActiveTacticalPathPoints.IsValidIndex(CurrentPathIndex))
		{
			UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Tactical traversal completed"));
			HandleTacticalPathTraversalCompleted();
			ClearTacticalPathTraversal(true);
		}

		return;
	}

	const FVector MoveDirection = ToTarget.GetSafeNormal();
	RotatePawnTowardDirection(ControlledCharacter, MoveDirection, DeltaTime);
	ControlledCharacter->AddMovementInput(MoveDirection, 1.0f);
}

void ACRPGProjectPlayerController::ClearTacticalPathTraversal(bool bStopPawnMovement)
{
	bHasActiveTacticalPath = false;
	ActiveTacticalPathPoints.Reset();
	CurrentPathIndex = INDEX_NONE;
	PendingPathDistanceConsumption = 0.0f;
	ActiveTraversalTravelledDistanceCm = 0.0f;
	LastTraversalPawnLocation = FVector::ZeroVector;

	if (bStopPawnMovement)
	{
		StopTacticalPrototypeMovement();
	}
}

void ACRPGProjectPlayerController::RotatePawnTowardDirection(APawn *ControlledPawn, const FVector &MoveDirection, float DeltaTime)
{
	if (!ControlledPawn || MoveDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator DesiredRotation = MoveDirection.Rotation();
	const FRotator NewRotation = FMath::RInterpTo(ControlledPawn->GetActorRotation(), DesiredRotation, DeltaTime, TacticalPathRotationInterpSpeed);
	ControlledPawn->SetActorRotation(NewRotation);
}

float ACRPGProjectPlayerController::CalculatePathDistance(const TArray<FVector> &PathPoints) const
{
	float TotalDistance = 0.0f;

	for (int32 PointIndex = 1; PointIndex < PathPoints.Num(); ++PointIndex)
	{
		TotalDistance += FVector::Distance(PathPoints[PointIndex - 1], PathPoints[PointIndex]);
	}

	return TotalDistance;
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

void ACRPGProjectPlayerController::HandleTacticalPathTraversalCompleted()
{
	// On a completed traversal, spend the full committed path budget. Interrupted moves already clamp
	// PendingPathDistanceConsumption down to the travelled portion before this method is called.
	float ConsumedDistanceCm = PendingPathDistanceConsumption;
	if (ConsumedDistanceCm <= TacticalMinimumCommittedMoveDistance)
	{
		return;
	}

	if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
	{
		if (TacticalTurnSubsystem->IsTurnModeActive())
		{
			if (ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit())
			{
				if (UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit->GetTacticalUnitComponent())
				{
					const float RemainingMovementRangeCm = TacticalUnitComponent->GetRemainingMovementRange();

					// If the approved move would leave only an unusable micro-budget, consume the remainder.
					if (RemainingMovementRangeCm - PendingPathDistanceConsumption <= TacticalMinimumCommittedMoveDistance + KINDA_SMALL_NUMBER)
					{
						ConsumedDistanceCm = RemainingMovementRangeCm;
					}

					TacticalUnitComponent->ConsumeMovement(ConsumedDistanceCm);

					if (UGameInstance *GameInstance = GetGameInstance())
					{
						if (UEventBusSubsystem *EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
						{
							EventBusSubsystem->PublishEvent(
								TacticalCombatEvents::TacticalUnitMovementConsumed,
								FString::Printf(
									TEXT("unit=%s;distance_cm=%.2f;remaining_cm=%.2f;round=%d"),
									*ActiveUnit->GetName(),
									ConsumedDistanceCm,
									TacticalUnitComponent->GetRemainingMovementRange(),
									TacticalTurnSubsystem->GetCurrentRound()));
						}
					}
				}
			}
		}
	}

	RefreshTacticalCombatHUD();
}

void ACRPGProjectPlayerController::HandleCameraModeChanged(const FCameraModeTransition &Transition)
{
	bIsTacticalRotateHeld = false;
	ClearPendingTacticalMovePreview();
	ApplyCameraModeInputState(Transition.NewMode);
	SetControlledPawnTacticalInputSuppressed(Transition.NewMode == ECameraMode::Tactical);
	RefreshTacticalCombatHUD();

	if (Transition.NewMode == ECameraMode::Tactical)
	{
		ClearTacticalPathTraversal(true);
		StopMovement();
		StopTacticalPrototypeMovement();
		return;
	}

	if (Transition.NewMode != ECameraMode::Tactical)
	{
		ClearTacticalPathTraversal(true);
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
	if (EventName == TacticalCombatEvents::TacticalTurnStarted || EventName == TacticalCombatEvents::TacticalTurnEnded || EventName == TacticalCombatEvents::TacticalEncounterStarted || EventName == TacticalCombatEvents::TacticalRoundStarted || EventName == TacticalCombatEvents::TacticalActiveUnitChanged || EventName == TacticalCombatEvents::TacticalUnitMovementConsumed)
	{
		if (EventName == TacticalCombatEvents::TacticalTurnStarted)
		{
			if (!ExplorationPawnBeforeTacticalTurn.IsValid())
			{
				ExplorationPawnBeforeTacticalTurn = GetPawn();
			}

			SyncPossessionToActiveTacticalUnit();
		}

		if (EventName == TacticalCombatEvents::TacticalTurnStarted || EventName == TacticalCombatEvents::TacticalActiveUnitChanged)
		{
			SyncPossessionToActiveTacticalUnit();

			const UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
			const ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
			const UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit ? ActiveUnit->GetTacticalUnitComponent() : nullptr;
			bTurnModeMovementEnabled = TacticalUnitComponent && (bAllowControllingNonPlayerTacticalUnits || TacticalUnitComponent->IsPlayerControlled());
		}

		if (EventName == TacticalCombatEvents::TacticalTurnEnded)
		{
			RestorePossessionAfterTacticalTurn();
			bTurnModeMovementEnabled = true;
			ClearPendingTacticalMovePreview();
		}

		RefreshTacticalCombatHUD();
	}
}

void ACRPGProjectPlayerController::SyncPossessionToActiveTacticalUnit()
{
	UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
	ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
	UTacticalUnitComponent *TacticalUnitComponent = ActiveUnit ? ActiveUnit->GetTacticalUnitComponent() : nullptr;
	if (!IsValid(ActiveUnit) || !TacticalUnitComponent || (!bAllowControllingNonPlayerTacticalUnits && !TacticalUnitComponent->IsPlayerControlled()) || GetPawn() == ActiveUnit)
	{
		return;
	}

	ClearPendingTacticalMovePreview();
	ClearTacticalPathTraversal(true);
	StopMovement();
	SetControlledPawnTacticalInputSuppressed(true);
	Possess(ActiveUnit);
	SetControlledPawnTacticalInputSuppressed(true);
}

void ACRPGProjectPlayerController::RestorePossessionAfterTacticalTurn()
{
	APawn *ExplorationPawn = ExplorationPawnBeforeTacticalTurn.Get();
	ExplorationPawnBeforeTacticalTurn.Reset();

	if (!IsValid(ExplorationPawn) || GetPawn() == ExplorationPawn)
	{
		return;
	}

	ClearPendingTacticalMovePreview();
	ClearTacticalPathTraversal(true);
	StopMovement();
	Possess(ExplorationPawn);
	SetControlledPawnTacticalInputSuppressed(false);
}

void ACRPGProjectPlayerController::OnPossess(APawn *InPawn)
{
	Super::OnPossess(InPawn);
	ClearPendingTacticalMovePreview();
	RefreshTacticalCombatHUD();
}

void ACRPGProjectPlayerController::OnUnPossess()
{
	ClearPendingTacticalMovePreview();
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

void ACRPGProjectPlayerController::CommitPendingTacticalMoveRequest()
{
	CommitPendingTacticalMove();
}

void ACRPGProjectPlayerController::ClearPendingTacticalMovePreviewRequest()
{
	ClearPendingTacticalMovePreview();
}

bool ACRPGProjectPlayerController::IsTurnModeMovementEnabled() const
{
	return bTurnModeMovementEnabled;
}

void ACRPGProjectPlayerController::SetTurnModeMovementEnabled(bool bEnabled)
{
	if (bTurnModeMovementEnabled == bEnabled)
	{
		return;
	}

	bTurnModeMovementEnabled = bEnabled;

	if (!bTurnModeMovementEnabled)
	{
		ClearPendingTacticalMovePreview();
	}
	else
	{
		RefreshTacticalCombatHUD();
	}
}

void ACRPGProjectPlayerController::ToggleTurnModeMovementEnabled()
{
	SetTurnModeMovementEnabled(!bTurnModeMovementEnabled);
}

float ACRPGProjectPlayerController::GetAvailableMovementBudgetForPlanningRequest() const
{
	return GetAvailableMovementBudgetForPlanning();
}

bool ACRPGProjectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

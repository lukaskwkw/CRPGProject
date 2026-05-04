// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraControllerComponent.h"
#include "CameraModeSubsystem.h"
#include "CRPGProjectPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Blueprint/UserWidget.h"
#include "CRPGProject.h"
#include "Widgets/Input/SVirtualJoystick.h"

ACRPGProjectPlayerController::ACRPGProjectPlayerController()
{
	CameraControllerComponent = CreateDefaultSubobject<UCameraControllerComponent>(TEXT("CameraControllerComponent"));
	PrimaryActorTick.bCanEverTick = true;
}

void ACRPGProjectPlayerController::BeginPlay()
{
	Super::BeginPlay();
	BindToCameraModeSubsystem();
	SetControlledPawnTacticalInputSuppressed(IsTacticalModeActive());
	ApplyCameraModeInputState(IsTacticalModeActive() ? ECameraMode::Tactical : ECameraMode::Exploration);

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
	ClearTacticalPathTraversal(true);
	UnbindFromCameraModeSubsystem();
	SetControlledPawnTacticalInputSuppressed(false);
	Super::EndPlay(EndPlayReason);
}

void ACRPGProjectPlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateTacticalPathTraversal(DeltaTime);
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

	UNavigationSystemV1 *NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavigationSystem)
	{
		return;
	}

	FNavLocation ProjectedLocation;
	if (!NavigationSystem->ProjectPointToNavigation(HitResult.ImpactPoint, ProjectedLocation))
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

void ACRPGProjectPlayerController::StartTacticalPathTraversal(const TArray<FVector> &PathPoints)
{
	ActiveTacticalPathPoints = PathPoints;
	bHasActiveTacticalPath = ActiveTacticalPathPoints.Num() > 0;
	CurrentPathIndex = ActiveTacticalPathPoints.Num() > 1 ? 1 : 0;

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
	const FVector TargetPoint = ActiveTacticalPathPoints[CurrentPathIndex];
	FVector ToTarget = TargetPoint - PawnLocation;
	ToTarget.Z = 0.0f;

	if (ToTarget.SizeSquared() <= FMath::Square(TacticalAcceptanceRadius))
	{
		UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Reached path node %d at %s"), CurrentPathIndex, *TargetPoint.ToString());
		++CurrentPathIndex;

		if (!ActiveTacticalPathPoints.IsValidIndex(CurrentPathIndex))
		{
			UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Tactical traversal completed"));
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

void ACRPGProjectPlayerController::HandleCameraModeChanged(const FCameraModeTransition &Transition)
{
	bIsTacticalRotateHeld = false;
	ApplyCameraModeInputState(Transition.NewMode);
	SetControlledPawnTacticalInputSuppressed(Transition.NewMode == ECameraMode::Tactical);

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
	if (APawn *ControlledPawn = GetPawn())
	{
		if (bSuppressInput)
		{
			ControlledPawn->DisableInput(this);
		}
		else
		{
			ControlledPawn->EnableInput(this);
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

bool ACRPGProjectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

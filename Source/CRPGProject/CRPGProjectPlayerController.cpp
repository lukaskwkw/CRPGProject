// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraControllerComponent.h"
#include "CameraModeSubsystem.h"
#include "CRPGProjectPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "NavigationSystem.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/UserWidget.h"
#include "CRPGProject.h"
#include "Widgets/Input/SVirtualJoystick.h"

ACRPGProjectPlayerController::ACRPGProjectPlayerController()
{
	CameraControllerComponent = CreateDefaultSubobject<UCameraControllerComponent>(TEXT("CameraControllerComponent"));
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
		return;
	}

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, ProjectedLocation.Location);
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

void ACRPGProjectPlayerController::HandleCameraModeChanged(const FCameraModeTransition &Transition)
{
	bIsTacticalRotateHeld = false;
	ApplyCameraModeInputState(Transition.NewMode);
	SetControlledPawnTacticalInputSuppressed(Transition.NewMode == ECameraMode::Tactical);

	if (Transition.NewMode == ECameraMode::Tactical)
	{
		StopMovement();
		StopTacticalPrototypeMovement();
		return;
	}

	if (Transition.NewMode != ECameraMode::Tactical)
	{
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

// Copyright Epic Games, Inc. All Rights Reserved.


#include "CameraControllerComponent.h"
#include "CameraModeSubsystem.h"
#include "CRPGProjectPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
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

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogCRPGProject, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ACRPGProjectPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::V, IE_Pressed, this, &ACRPGProjectPlayerController::ToggleDebugCameraMode);
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
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

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UCameraModeSubsystem* Subsystem = GameInstance->GetSubsystem<UCameraModeSubsystem>())
		{
			Subsystem->RequestToggleCameraMode();
		}
	}
}

bool ACRPGProjectPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

#include "CameraControllerComponent.h"

#include "CRPGProject.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

UCameraControllerComponent::UCameraControllerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    ExplorationSettings.ArmLength = 400.0f;
    ExplorationSettings.TargetOffset = FVector::ZeroVector;
    ExplorationSettings.SocketOffset = FVector::ZeroVector;
    ExplorationSettings.Rotation = FRotator(-10.0f, 0.0f, 0.0f);
    ExplorationSettings.FieldOfView = 90.0f;

    TacticalSettings.ArmLength = 900.0f;
    TacticalSettings.TargetOffset = FVector(0.0f, 0.0f, 180.0f);
    TacticalSettings.SocketOffset = FVector::ZeroVector;
    TacticalSettings.Rotation = FRotator(-60.0f, 0.0f, 0.0f);
    TacticalSettings.FieldOfView = 75.0f;
    UpdateTacticalPitchFromZoom();

    CinematicSettings = TacticalSettings;
}

void UCameraControllerComponent::BeginPlay()
{
    Super::BeginPlay();

    // The component always derives state from the owning controller/pawn pair instead of assuming startup order.
    OwningPlayerController = Cast<APlayerController>(GetOwner());
    HandleControlledPawnChanged();
    BindToCameraModeSubsystem();
    InitializeForCurrentMode();
}

void UCameraControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnbindFromCameraModeSubsystem();

    if (TacticalCameraActor)
    {
        TacticalCameraActor->Destroy();
        TacticalCameraActor = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void UCameraControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    HandleControlledPawnChanged();

    if (!OwningPlayerController)
    {
        return;
    }

    switch (ActiveCameraMode)
    {
    case ECameraMode::Tactical:
        UpdateTacticalCamera(DeltaTime);
        break;

    case ECameraMode::Cinematic:
        break;

    case ECameraMode::Exploration:
    default:
        UpdateExplorationCamera(DeltaTime);
        break;
    }
}

void UCameraControllerComponent::AddTacticalRoamInput(const FVector2D &InputAxis)
{
    TacticalRoamInput = InputAxis;
}

void UCameraControllerComponent::ClearTacticalRoamInput()
{
    TacticalRoamInput = FVector2D::ZeroVector;
}

void UCameraControllerComponent::RecenterOnActivePawn()
{
    if (APawn *Pawn = ActiveCameraMode == ECameraMode::Tactical ? GetTacticalFocusPawn() : GetControlledPawn())
    {
        // Recenter updates the desired anchor first and only snaps the live anchor if one was never established.
        const FVector FocusLocation = GetPawnFocusLocation(Pawn);
        TargetTacticalAnchorLocation = FocusLocation;

        if (!bHasTacticalAnchor)
        {
            TacticalAnchorLocation = FocusLocation;
            bHasTacticalAnchor = true;
        }
    }
}

void UCameraControllerComponent::FocusTacticalPawn(APawn *Pawn)
{
    if (!OwningPlayerController || !IsValid(Pawn))
    {
        return;
    }

    EnsureTacticalCamera();
    ClearTacticalRoamInput();

    // Focus requests only move the tactical anchor; the real camera position still glides there in Tick.
    const FVector FocusLocation = GetPawnFocusLocation(Pawn);
    TargetTacticalAnchorLocation = FocusLocation;

    if (!bHasTacticalAnchor)
    {
        TacticalAnchorLocation = FocusLocation;
        bHasTacticalAnchor = true;
    }

    if (ActiveCameraMode == ECameraMode::Tactical && TacticalCameraActor && OwningPlayerController->GetViewTarget() != TacticalCameraActor)
    {
        OwningPlayerController->SetViewTargetWithBlend(TacticalCameraActor, ActiveTransition.BlendTime);
    }
}

void UCameraControllerComponent::AdjustTacticalZoom(float ZoomAxisValue)
{
    if (ActiveCameraMode != ECameraMode::Tactical || FMath::IsNearlyZero(ZoomAxisValue))
    {
        return;
    }

    TacticalSettings.ArmLength = FMath::Clamp(
        TacticalSettings.ArmLength - (ZoomAxisValue * TacticalZoomStep),
        TacticalMinimumArmLength,
        TacticalMaximumArmLength);

    // Pitch is derived from zoom so wider tactical views automatically flatten toward a more top-down angle.
    UpdateTacticalPitchFromZoom();
}

void UCameraControllerComponent::AddTacticalYawInput(float YawAxisValue)
{
    if (ActiveCameraMode != ECameraMode::Tactical || FMath::IsNearlyZero(YawAxisValue))
    {
        return;
    }

    TacticalYaw = FRotator::NormalizeAxis(TacticalYaw + (YawAxisValue * TacticalYawRotationSpeed));
    TacticalSettings.Rotation.Yaw = TacticalYaw;
}

ECameraMode UCameraControllerComponent::GetActiveCameraMode() const
{
    return ActiveCameraMode;
}

void UCameraControllerComponent::HandleCameraModeChanged(const FCameraModeTransition &Transition)
{
    ActiveTransition = Transition;
    ActiveCameraMode = Transition.NewMode;

    // Each mode entry function owns its own view-target transition and camera state bootstrap.
    switch (Transition.NewMode)
    {
    case ECameraMode::Tactical:
        EnterTacticalMode(Transition);
        break;

    case ECameraMode::Cinematic:
        EnterCinematicMode(Transition);
        break;

    case ECameraMode::Exploration:
    default:
        EnterExplorationMode(Transition);
        break;
    }
}

void UCameraControllerComponent::UpdateTacticalPitchFromZoom()
{
    const float ZoomAlpha = TacticalMaximumArmLength > TacticalMinimumArmLength
                                ? FMath::GetRangePct(TacticalMinimumArmLength, TacticalMaximumArmLength, TacticalSettings.ArmLength)
                                : 0.0f;

    TacticalSettings.Rotation.Pitch = FMath::Lerp(TacticalClosestPitch, TacticalFarthestPitch, ZoomAlpha);
}

void UCameraControllerComponent::InitializeForCurrentMode()
{
    if (!CameraModeSubsystem)
    {
        // No subsystem means exploration defaults win so template camera behavior still works in isolation.
        ActiveCameraMode = ECameraMode::Exploration;
        EnterExplorationMode(ActiveTransition);
        return;
    }

    ActiveTransition = CameraModeSubsystem->GetCurrentTransition();
    ActiveCameraMode = CameraModeSubsystem->GetCurrentCameraMode();
    HandleCameraModeChanged(ActiveTransition);
}

void UCameraControllerComponent::BindToCameraModeSubsystem()
{
    if (!OwningPlayerController)
    {
        return;
    }

    UGameInstance *GameInstance = OwningPlayerController->GetGameInstance();
    if (!GameInstance)
    {
        return;
    }

    CameraModeSubsystem = GameInstance->GetSubsystem<UCameraModeSubsystem>();
    if (CameraModeSubsystem)
    {
        CameraModeSubsystem->OnCameraModeChanged.AddDynamic(this, &UCameraControllerComponent::HandleCameraModeChanged);
    }
}

void UCameraControllerComponent::UnbindFromCameraModeSubsystem()
{
    if (CameraModeSubsystem)
    {
        CameraModeSubsystem->OnCameraModeChanged.RemoveAll(this);
        CameraModeSubsystem = nullptr;
    }
}

void UCameraControllerComponent::EnsureTacticalCamera()
{
    if (TacticalCameraActor || !OwningPlayerController || !GetWorld())
    {
        return;
    }

    // Tactical view uses a transient detached camera actor so exploration spring-arm settings remain untouched.
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = OwningPlayerController;
    SpawnParameters.ObjectFlags |= RF_Transient;

    TacticalCameraActor = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, SpawnParameters);
    if (TacticalCameraActor)
    {
        TacticalCameraActor->SetActorHiddenInGame(true);
        TacticalCameraActor->SetActorEnableCollision(false);
    }
}

void UCameraControllerComponent::HandleControlledPawnChanged()
{
    APawn *CurrentPawn = GetControlledPawn();
    if (CachedPawn == CurrentPawn)
    {
        return;
    }

    CachedPawn = CurrentPawn;
    ClearTacticalRoamInput();

    if (ActiveCameraMode == ECameraMode::Exploration)
    {
        // Exploration should always snap back to the newly possessed pawn and forget the tactical anchor entirely.
        bHasTacticalAnchor = false;
        RecenterOnActivePawn();
        EnterExplorationMode(ActiveTransition);
    }
    else if (ActiveCameraMode == ECameraMode::Tactical)
    {
        // In tactical mode keep the existing camera yaw and current anchor, then glide toward the newly selected focus pawn.
        RecenterOnActivePawn();
        EnsureTacticalCamera();

        if (TacticalCameraActor && OwningPlayerController && OwningPlayerController->GetViewTarget() != TacticalCameraActor)
        {
            OwningPlayerController->SetViewTargetWithBlend(TacticalCameraActor, ActiveTransition.BlendTime);
        }
    }
}

void UCameraControllerComponent::EnterExplorationMode(const FCameraModeTransition &Transition)
{
    if (!OwningPlayerController)
    {
        return;
    }

    ClearTacticalRoamInput();
    TacticalYaw = 0.0f;
    TacticalSettings.Rotation.Yaw = TacticalYaw;

    if (APawn *Pawn = GetControlledPawn())
    {
        OwningPlayerController->SetViewTargetWithBlend(Pawn, Transition.BlendTime);
    }
}

void UCameraControllerComponent::EnterTacticalMode(const FCameraModeTransition &Transition)
{
    if (!OwningPlayerController)
    {
        return;
    }

    EnsureTacticalCamera();
    RecenterOnActivePawn();
    // Tactical mode inherits the controller yaw at entry so orbit direction stays consistent with what the player sees.
    TacticalYaw = FRotator::NormalizeAxis(OwningPlayerController->GetControlRotation().Yaw);
    TacticalSettings.Rotation.Yaw = TacticalYaw;
    TacticalAnchorLocation = TargetTacticalAnchorLocation;
    bHasTacticalAnchor = true;

    if (TacticalCameraActor)
    {
        const FVector DesiredCameraLocation = TacticalAnchorLocation - TacticalSettings.Rotation.Vector() * TacticalSettings.ArmLength + TacticalSettings.TargetOffset + TacticalSettings.SocketOffset;

        TacticalCameraActor->SetActorLocationAndRotation(DesiredCameraLocation, TacticalSettings.Rotation);
        TacticalCameraActor->GetCameraComponent()->SetFieldOfView(TacticalSettings.FieldOfView);
        OwningPlayerController->SetViewTargetWithBlend(TacticalCameraActor, Transition.BlendTime);
    }
}

void UCameraControllerComponent::EnterCinematicMode(const FCameraModeTransition &Transition)
{
    if (!OwningPlayerController)
    {
        return;
    }

    if (APawn *Pawn = GetControlledPawn())
    {
        OwningPlayerController->SetViewTargetWithBlend(Pawn, Transition.BlendTime);
    }
}

void UCameraControllerComponent::UpdateExplorationCamera(float DeltaTime)
{
    USpringArmComponent *SpringArm = FindControlledSpringArm();
    UCameraComponent *Camera = FindControlledCamera();
    if (!SpringArm || !Camera)
    {
        return;
    }

    const FCameraModeViewSettings &Settings = GetModeViewSettings(ECameraMode::Exploration);
    const float InterpSpeed = GetModeInterpolationSpeed();

    SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, Settings.ArmLength, DeltaTime, InterpSpeed);
    SpringArm->TargetOffset = FMath::VInterpTo(SpringArm->TargetOffset, Settings.TargetOffset, DeltaTime, InterpSpeed);
    SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, Settings.SocketOffset, DeltaTime, InterpSpeed);
    Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, Settings.FieldOfView, DeltaTime, InterpSpeed));
    SpringArm->bUsePawnControlRotation = true;
}

void UCameraControllerComponent::UpdateTacticalCamera(float DeltaTime)
{
    if (!OwningPlayerController)
    {
        return;
    }

    EnsureTacticalCamera();
    APawn *Pawn = GetTacticalFocusPawn();
    if (!Pawn || !TacticalCameraActor)
    {
        return;
    }

    const FCameraModeViewSettings &Settings = GetModeViewSettings(ECameraMode::Tactical);
    const float InterpSpeed = GetModeInterpolationSpeed();
    const float AnchorInterpSpeed = GetAnchorInterpolationSpeed();
    const FVector FocusLocation = GetPawnFocusLocation(Pawn);
    const FRotator TacticalYawRotation(0.0f, Settings.Rotation.Yaw, 0.0f);
    const FVector ForwardDirection = FRotationMatrix(TacticalYawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(TacticalYawRotation).GetUnitAxis(EAxis::Y);

    // Roam input moves the desired anchor in camera-relative space; the actual anchor then eases toward it.
    TargetTacticalAnchorLocation += ((ForwardDirection * TacticalRoamInput.Y) + (RightDirection * TacticalRoamInput.X)) * TacticalRoamSpeed * DeltaTime;

    if (!bHasTacticalAnchor)
    {
        // First tactical tick snaps anchor setup to the current focus pawn before interpolation begins.
        TargetTacticalAnchorLocation = FocusLocation;
        TacticalAnchorLocation = FocusLocation;
        bHasTacticalAnchor = true;
    }

    // Two layers of interpolation are intentional: anchor smoothing for focus changes, then camera smoothing for presentation.
    TacticalAnchorLocation = FMath::VInterpTo(TacticalAnchorLocation, TargetTacticalAnchorLocation, DeltaTime, AnchorInterpSpeed);

    const FVector DesiredCameraLocation = TacticalAnchorLocation - Settings.Rotation.Vector() * Settings.ArmLength + Settings.TargetOffset + Settings.SocketOffset;
    const FVector NewCameraLocation = FMath::VInterpTo(TacticalCameraActor->GetActorLocation(), DesiredCameraLocation, DeltaTime, InterpSpeed);
    const FRotator NewCameraRotation = FMath::RInterpTo(TacticalCameraActor->GetActorRotation(), Settings.Rotation, DeltaTime, InterpSpeed);

    TacticalCameraActor->SetActorLocationAndRotation(NewCameraLocation, NewCameraRotation);
    TacticalCameraActor->GetCameraComponent()->SetFieldOfView(FMath::FInterpTo(TacticalCameraActor->GetCameraComponent()->FieldOfView, Settings.FieldOfView, DeltaTime, InterpSpeed));
}

const FCameraModeViewSettings &UCameraControllerComponent::GetModeViewSettings(ECameraMode CameraMode) const
{
    switch (CameraMode)
    {
    case ECameraMode::Tactical:
        return TacticalSettings;

    case ECameraMode::Cinematic:
        return CinematicSettings;

    case ECameraMode::Exploration:
    default:
        return ExplorationSettings;
    }
}

APawn *UCameraControllerComponent::GetControlledPawn() const
{
    return OwningPlayerController ? OwningPlayerController->GetPawn() : nullptr;
}

APawn *UCameraControllerComponent::GetTacticalFocusPawn() const
{
    if (!OwningPlayerController)
    {
        return nullptr;
    }

    // Controlled pawn wins so ally selection updates the tactical camera immediately; active unit is only a fallback.
    if (APawn *ControlledPawn = GetControlledPawn())
    {
        return ControlledPawn;
    }

    if (UGameInstance *GameInstance = OwningPlayerController->GetGameInstance())
    {
        if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GameInstance->GetSubsystem<UTacticalTurnSubsystem>())
        {
            if (ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem->GetActiveUnit())
            {
                return ActiveUnit;
            }
        }
    }

    return GetControlledPawn();
}

USpringArmComponent *UCameraControllerComponent::FindControlledSpringArm() const
{
    if (APawn *Pawn = GetControlledPawn())
    {
        return Pawn->FindComponentByClass<USpringArmComponent>();
    }

    return nullptr;
}

UCameraComponent *UCameraControllerComponent::FindControlledCamera() const
{
    if (APawn *Pawn = GetControlledPawn())
    {
        return Pawn->FindComponentByClass<UCameraComponent>();
    }

    return nullptr;
}

FVector UCameraControllerComponent::GetPawnFocusLocation(APawn *Pawn) const
{
    // Camera focus can intentionally sit above the pawn origin so tactical framing is centered on the character body.
    return Pawn ? Pawn->GetActorLocation() + PawnFocusOffset : FVector::ZeroVector;
}

float UCameraControllerComponent::GetModeInterpolationSpeed() const
{
    return ActiveTransition.BlendTime > KINDA_SMALL_NUMBER
               ? FMath::Clamp(1.0f / ActiveTransition.BlendTime, MinimumInterpolationSpeed, MaximumInterpolationSpeed)
               : MaximumInterpolationSpeed;
}

float UCameraControllerComponent::GetAnchorInterpolationSpeed() const
{
    return FMath::Max(MinimumInterpolationSpeed, GetModeInterpolationSpeed());
}

#include "CameraControllerComponent.h"

#include "CRPGProject.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"

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

    CinematicSettings = TacticalSettings;
}

void UCameraControllerComponent::BeginPlay()
{
    Super::BeginPlay();

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

void UCameraControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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

void UCameraControllerComponent::AddTacticalRoamInput(const FVector2D& InputAxis)
{
    TacticalRoamInput = InputAxis;
}

void UCameraControllerComponent::ClearTacticalRoamInput()
{
    TacticalRoamInput = FVector2D::ZeroVector;
}

void UCameraControllerComponent::RecenterOnActivePawn()
{
    if (APawn* Pawn = GetControlledPawn())
    {
        const FVector FocusLocation = GetPawnFocusLocation(Pawn);
        TargetTacticalAnchorLocation = FocusLocation;

        if (!bHasTacticalAnchor)
        {
            TacticalAnchorLocation = FocusLocation;
            bHasTacticalAnchor = true;
        }
    }
}

ECameraMode UCameraControllerComponent::GetActiveCameraMode() const
{
    return ActiveCameraMode;
}

void UCameraControllerComponent::HandleCameraModeChanged(const FCameraModeTransition& Transition)
{
    ActiveTransition = Transition;
    ActiveCameraMode = Transition.NewMode;

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

void UCameraControllerComponent::InitializeForCurrentMode()
{
    if (!CameraModeSubsystem)
    {
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

    UGameInstance* GameInstance = OwningPlayerController->GetGameInstance();
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
    APawn* CurrentPawn = GetControlledPawn();
    if (CachedPawn == CurrentPawn)
    {
        return;
    }

    CachedPawn = CurrentPawn;
    bHasTacticalAnchor = false;
    ClearTacticalRoamInput();
    RecenterOnActivePawn();

    if (ActiveCameraMode == ECameraMode::Exploration)
    {
        EnterExplorationMode(ActiveTransition);
    }
    else if (ActiveCameraMode == ECameraMode::Tactical)
    {
        EnterTacticalMode(ActiveTransition);
    }
}

void UCameraControllerComponent::EnterExplorationMode(const FCameraModeTransition& Transition)
{
    if (!OwningPlayerController)
    {
        return;
    }

    ClearTacticalRoamInput();

    if (APawn* Pawn = GetControlledPawn())
    {
        OwningPlayerController->SetViewTargetWithBlend(Pawn, Transition.BlendTime);
    }
}

void UCameraControllerComponent::EnterTacticalMode(const FCameraModeTransition& Transition)
{
    if (!OwningPlayerController)
    {
        return;
    }

    EnsureTacticalCamera();
    RecenterOnActivePawn();

    if (TacticalCameraActor)
    {
        OwningPlayerController->SetViewTargetWithBlend(TacticalCameraActor, Transition.BlendTime);
    }
}

void UCameraControllerComponent::EnterCinematicMode(const FCameraModeTransition& Transition)
{
    if (!OwningPlayerController)
    {
        return;
    }

    if (APawn* Pawn = GetControlledPawn())
    {
        OwningPlayerController->SetViewTargetWithBlend(Pawn, Transition.BlendTime);
    }
}

void UCameraControllerComponent::UpdateExplorationCamera(float DeltaTime)
{
    USpringArmComponent* SpringArm = FindControlledSpringArm();
    UCameraComponent* Camera = FindControlledCamera();
    if (!SpringArm || !Camera)
    {
        return;
    }

    const FCameraModeViewSettings& Settings = GetModeViewSettings(ECameraMode::Exploration);
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
    APawn* Pawn = GetControlledPawn();
    if (!Pawn || !TacticalCameraActor)
    {
        return;
    }

    const FCameraModeViewSettings& Settings = GetModeViewSettings(ECameraMode::Tactical);
    const float InterpSpeed = GetModeInterpolationSpeed();
    const float AnchorInterpSpeed = GetAnchorInterpolationSpeed();
    const FVector FocusLocation = GetPawnFocusLocation(Pawn);
    const FVector ForwardDirection = FRotationMatrix(FRotator(0.0f, OwningPlayerController->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(FRotator(0.0f, OwningPlayerController->GetControlRotation().Yaw, 0.0f)).GetUnitAxis(EAxis::Y);

    TargetTacticalAnchorLocation += ((ForwardDirection * TacticalRoamInput.Y) + (RightDirection * TacticalRoamInput.X)) * TacticalRoamSpeed * DeltaTime;

    if (!bHasTacticalAnchor)
    {
        TargetTacticalAnchorLocation = FocusLocation;
        TacticalAnchorLocation = FocusLocation;
        bHasTacticalAnchor = true;
    }

    TacticalAnchorLocation = FMath::VInterpTo(TacticalAnchorLocation, TargetTacticalAnchorLocation, DeltaTime, AnchorInterpSpeed);

    const FVector DesiredCameraLocation = TacticalAnchorLocation - Settings.Rotation.Vector() * Settings.ArmLength + Settings.TargetOffset + Settings.SocketOffset;
    const FVector NewCameraLocation = FMath::VInterpTo(TacticalCameraActor->GetActorLocation(), DesiredCameraLocation, DeltaTime, InterpSpeed);
    const FRotator NewCameraRotation = FMath::RInterpTo(TacticalCameraActor->GetActorRotation(), Settings.Rotation, DeltaTime, InterpSpeed);

    TacticalCameraActor->SetActorLocationAndRotation(NewCameraLocation, NewCameraRotation);
    TacticalCameraActor->GetCameraComponent()->SetFieldOfView(FMath::FInterpTo(TacticalCameraActor->GetCameraComponent()->FieldOfView, Settings.FieldOfView, DeltaTime, InterpSpeed));

    USpringArmComponent* SpringArm = FindControlledSpringArm();
    UCameraComponent* Camera = FindControlledCamera();
    if (SpringArm && Camera)
    {
        SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, Settings.ArmLength, DeltaTime, InterpSpeed);
        SpringArm->TargetOffset = FMath::VInterpTo(SpringArm->TargetOffset, Settings.TargetOffset, DeltaTime, InterpSpeed);
        SpringArm->SocketOffset = FMath::VInterpTo(SpringArm->SocketOffset, Settings.SocketOffset, DeltaTime, InterpSpeed);
        SpringArm->bUsePawnControlRotation = false;
        Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, Settings.FieldOfView, DeltaTime, InterpSpeed));
    }

    OwningPlayerController->SetControlRotation(FMath::RInterpTo(OwningPlayerController->GetControlRotation(), Settings.Rotation, DeltaTime, InterpSpeed));
}

const FCameraModeViewSettings& UCameraControllerComponent::GetModeViewSettings(ECameraMode CameraMode) const
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

APawn* UCameraControllerComponent::GetControlledPawn() const
{
    return OwningPlayerController ? OwningPlayerController->GetPawn() : nullptr;
}

USpringArmComponent* UCameraControllerComponent::FindControlledSpringArm() const
{
  if (APawn* Pawn = GetControlledPawn())
    {
        return Pawn->FindComponentByClass<USpringArmComponent>();
    }

    return nullptr;
}

UCameraComponent* UCameraControllerComponent::FindControlledCamera() const
{
  if (APawn* Pawn = GetControlledPawn())
    {
        return Pawn->FindComponentByClass<UCameraComponent>();
    }

    return nullptr;
}

FVector UCameraControllerComponent::GetPawnFocusLocation(APawn* Pawn) const
{
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

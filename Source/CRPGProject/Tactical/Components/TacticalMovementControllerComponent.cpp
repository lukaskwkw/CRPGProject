#include "TacticalMovementControllerComponent.h"

#include "CRPGProject.h"
#include "CameraControllerComponent.h"
#include "Components/CapsuleComponent.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Framework/Controllers/CRPGProjectPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Tactical/Components/TacticalPathPreviewComponent.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

namespace TacticalMovementEvents
{
    static const FString TacticalUnitMovementConsumed = TEXT("tactical_unit_movement_consumed");
}

UTacticalMovementControllerComponent::UTacticalMovementControllerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UTacticalMovementControllerComponent::BeginPlay()
{
    Super::BeginPlay();
    OwningController = Cast<ACRPGProjectPlayerController>(GetOwner());
}

void UTacticalMovementControllerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearPendingTacticalMovePreview();
    ClearTacticalPathTraversal(true);
    OwningController = nullptr;
    Super::EndPlay(EndPlayReason);
}

void UTacticalMovementControllerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Preview generation and traversal updates intentionally coexist here so hover feedback and active motion
    // stay in sync with the same controller/camera state each frame.
    UpdateTacticalPathTraversal(DeltaTime);
    UpdateTacticalMovePreviewFromHover();
}

void UTacticalMovementControllerComponent::HandleTacticalLeftClick()
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller)
    {
        return;
    }

    FHitResult HitResult;
    if (!Controller->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult) || !HitResult.bBlockingHit)
    {
        return;
    }

    APawn *ControlledPawn = Controller->GetPawn();
    if (!ControlledPawn)
    {
        return;
    }

    const FVector ClickedLocation = HitResult.ImpactPoint;

    // In turn mode a click only confirms the currently hovered preview; outside turn mode it starts immediate traversal.
    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem())
    {
        if (TacticalTurnSubsystem->IsTurnModeActive())
        {
            if (!Controller->IsTurnModeMovementEnabled())
            {
                return;
            }

            if (bHasActiveTacticalPath)
            {
                StopActiveTacticalMove(true);
            }

            if (!PendingMovePreview.bHasPreview || !IsClickNearPendingDestination(ClickedLocation))
            {
                return;
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

void UTacticalMovementControllerComponent::CommitPendingTacticalMove()
{
    if (!PendingMovePreview.bHasPreview || PendingMovePreview.AffordablePathPoints.Num() < 2 || PendingMovePreview.AffordablePathDistanceCm <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    if (!OwningController || PendingMovePreview.AffordablePathDistanceCm <= OwningController->GetTacticalMinimumCommittedMoveDistance())
    {
        ClearPendingTacticalMovePreview();
        return;
    }

    // Only the affordable segment is committed. Over-budget tail remains visual feedback only.
    const TArray<FVector> PathPoints = PendingMovePreview.AffordablePathPoints;
    const float PathDistanceCm = PendingMovePreview.AffordablePathDistanceCm;
    const TArray<FVector> TraversalControlPath = OwningController->BuildTraversalControlPath(PathPoints, OwningController->GetTraversalControlPointSpacing());

    if (bHasActiveTacticalPath)
    {
        StopActiveTacticalMove(true);
    }

    if (UTacticalPathPreviewComponent *PathPreviewComponent = GetPathPreviewComponent())
    {
        PathPreviewComponent->ClearPath();
    }

    OwningController->SetCommittedTraversalControlPoints(TraversalControlPath);
    PendingMovePreview = FTacticalMovePreviewData();
    StartTacticalPathTraversal(TraversalControlPath, PathDistanceCm);
    RefreshTacticalCombatHUD();
}

void UTacticalMovementControllerComponent::ClearPendingTacticalMovePreview()
{
    PendingMovePreview = FTacticalMovePreviewData();

    if (UTacticalPathPreviewComponent *PathPreviewComponent = GetPathPreviewComponent())
    {
        PathPreviewComponent->ClearPath();
    }

    RefreshTacticalCombatHUD();
}

void UTacticalMovementControllerComponent::StopActiveTacticalMove(bool bConsumeTravelledDistance)
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller || !bHasActiveTacticalPath)
    {
        return;
    }

    if (const APawn *ControlledPawn = Controller->GetPawn())
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

void UTacticalMovementControllerComponent::ClearTacticalPathTraversal(bool bStopPawnMovement)
{
    if (OwningController)
    {
        OwningController->ClearCommittedTraversalControlPoints();
    }

    // Traversal temporarily changes both local steering and pawn collision, so cleanup must restore both.
    RestoreTraversalAvoidance();
    RestoreTraversalPawnCollision();
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

bool UTacticalMovementControllerComponent::HasActiveTacticalPath() const
{
    return bHasActiveTacticalPath;
}

const TArray<FVector> &UTacticalMovementControllerComponent::GetActiveTacticalPathPoints() const
{
    return ActiveTacticalPathPoints;
}

float UTacticalMovementControllerComponent::GetAvailableMovementBudgetForPlanning() const
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

const FTacticalMovePreviewData &UTacticalMovementControllerComponent::GetPendingMovePreview() const
{
    return PendingMovePreview;
}

ACRPGProjectPlayerController *UTacticalMovementControllerComponent::GetOwnerController() const
{
    return OwningController.Get();
}

UTacticalTurnSubsystem *UTacticalMovementControllerComponent::GetTacticalTurnSubsystem() const
{
    if (ACRPGProjectPlayerController *Controller = GetOwnerController())
    {
        if (UGameInstance *GameInstance = Controller->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UTacticalTurnSubsystem>();
        }
    }

    return nullptr;
}

UTacticalPathPreviewComponent *UTacticalMovementControllerComponent::GetPathPreviewComponent() const
{
    if (const ACRPGProjectPlayerController *Controller = GetOwnerController())
    {
        return Controller->GetTacticalPathPreviewComponent();
    }

    return nullptr;
}

void UTacticalMovementControllerComponent::DisableTraversalAvoidance(ACharacter *ControlledCharacter)
{
    RestoreTraversalAvoidance();

    if (!ControlledCharacter)
    {
        return;
    }

    UCharacterMovementComponent *CharacterMovement = ControlledCharacter->GetCharacterMovement();
    if (!CharacterMovement)
    {
        return;
    }

    TraversalAvoidanceCharacter = ControlledCharacter;
    bTraversalAvoidanceWasEnabled = CharacterMovement->bUseRVOAvoidance;
    CharacterMovement->SetAvoidanceEnabled(false);
}

void UTacticalMovementControllerComponent::RestoreTraversalAvoidance()
{
    if (!TraversalAvoidanceCharacter)
    {
        bTraversalAvoidanceWasEnabled = false;
        return;
    }

    if (UCharacterMovementComponent *CharacterMovement = TraversalAvoidanceCharacter->GetCharacterMovement())
    {
        CharacterMovement->SetAvoidanceEnabled(bTraversalAvoidanceWasEnabled);
    }

    TraversalAvoidanceCharacter = nullptr;
    bTraversalAvoidanceWasEnabled = false;
}

void UTacticalMovementControllerComponent::DisableTraversalPawnCollision(ACharacter *ControlledCharacter)
{
    RestoreTraversalPawnCollision();

    const UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    if (!TacticalTurnSubsystem || !TacticalTurnSubsystem->IsTurnModeActive())
    {
        return;
    }

    if (!ControlledCharacter)
    {
        return;
    }

    UCapsuleComponent *CapsuleComponent = ControlledCharacter->GetCapsuleComponent();
    if (!CapsuleComponent)
    {
        return;
    }

    TraversalPawnCollisionCharacter = ControlledCharacter;
    TraversalPawnCollisionResponse = CapsuleComponent->GetCollisionResponseToChannel(ECC_Pawn);
    CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

    // While moving, the pawn ignores other pawns physically, but logical occupancy still prevents ending on them.
    if (ACRPGBaseCharacter *ControlledBaseCharacter = Cast<ACRPGBaseCharacter>(ControlledCharacter))
    {
        if (UTacticalUnitComponent *TacticalUnitComponent = ControlledBaseCharacter->GetTacticalUnitComponent())
        {
            TacticalUnitComponent->SetOccupancySuppressed(true);
        }
    }
}

void UTacticalMovementControllerComponent::RestoreTraversalPawnCollision()
{
    if (!TraversalPawnCollisionCharacter)
    {
        TraversalPawnCollisionResponse = ECR_Block;
        return;
    }

    if (UCapsuleComponent *CapsuleComponent = TraversalPawnCollisionCharacter->GetCapsuleComponent())
    {
        CapsuleComponent->SetCollisionResponseToChannel(ECC_Pawn, TraversalPawnCollisionResponse);
    }

    if (ACRPGBaseCharacter *ControlledBaseCharacter = Cast<ACRPGBaseCharacter>(TraversalPawnCollisionCharacter.Get()))
    {
        if (UTacticalUnitComponent *TacticalUnitComponent = ControlledBaseCharacter->GetTacticalUnitComponent())
        {
            TacticalUnitComponent->SetOccupancySuppressed(false);
        }
    }

    TraversalPawnCollisionCharacter = nullptr;
    TraversalPawnCollisionResponse = ECR_Block;
}

bool UTacticalMovementControllerComponent::IsTacticalModeActive() const
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    const UCameraControllerComponent *CameraController = Controller ? Controller->GetCameraControllerComponent() : nullptr;
    return CameraController && CameraController->GetActiveCameraMode() == ECameraMode::Tactical;
}

bool UTacticalMovementControllerComponent::TryGetTacticalCursorWorldLocation(FVector &OutWorldLocation) const
{
    const ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller)
    {
        return false;
    }

    FHitResult HitResult;
    if (!Controller->GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult) || !HitResult.bBlockingHit)
    {
        return false;
    }

    OutWorldLocation = HitResult.ImpactPoint;
    return true;
}

bool UTacticalMovementControllerComponent::IsClickNearPendingDestination(const FVector &ClickedLocation) const
{
    return PendingMovePreview.bHasPreview && FVector::Dist2D(PendingMovePreview.Destination, ClickedLocation) <= 100.0f;
}

float UTacticalMovementControllerComponent::GetReservedMovementDistanceCm() const
{
    return bHasActiveTacticalPath ? FMath::Max(0.0f, ActiveTraversalTravelledDistanceCm) : 0.0f;
}

float UTacticalMovementControllerComponent::CalculatePathDistance(const TArray<FVector> &PathPoints) const
{
    float TotalDistance = 0.0f;

    for (int32 PointIndex = 1; PointIndex < PathPoints.Num(); ++PointIndex)
    {
        TotalDistance += FVector::Distance(PathPoints[PointIndex - 1], PathPoints[PointIndex]);
    }

    return TotalDistance;
}

void UTacticalMovementControllerComponent::BuildTacticalMovePreview(const FVector &Destination)
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller || !Controller->IsTurnModeMovementEnabled())
    {
        ClearPendingTacticalMovePreview();
        return;
    }

    APawn *ControlledPawn = Controller->GetPawn();
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

    PendingMovePreview = FTacticalMovePreviewData();
    PendingMovePreview.bHasPreview = true;
    PendingMovePreview.Destination = ProjectedLocation.Location;
    UNavigationPath *NavigationPath = NavigationSystem->FindPathToLocationSynchronously(
        GetWorld(),
        ControlledPawn->GetActorLocation(),
        ProjectedLocation.Location,
        ControlledPawn);
    if (!NavigationPath || !NavigationPath->IsValid() || NavigationPath->PathPoints.Num() == 0)
    {
        ClearPendingTacticalMovePreview();
        return;
    }

    // Preserve dense resampling for readable previews and precise budget clamping while keeping navmesh as the route authority.
    PendingMovePreview.PathPoints = Controller->BuildDenseResampledPath(NavigationPath->PathPoints, Controller->GetDenseSampleSpacing());
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
            PendingMovePreview.bIsAffordable = PendingMovePreview.PathDistanceCm <= RemainingMovementRangeCm + KINDA_SMALL_NUMBER;
            PendingMovePreview.bHasOverBudgetSegment = !PendingMovePreview.bIsAffordable;
            BuildClampedTacticalPath(PendingMovePreview.PathPoints, RemainingMovementRangeCm, PendingMovePreview.AffordablePathPoints, PendingMovePreview.AffordablePathDistanceCm);
        }
    }

    if (UTacticalPathPreviewComponent *PathPreviewComponent = GetPathPreviewComponent())
    {
        PathPreviewComponent->RenderPath(PendingMovePreview.PathPoints, PendingMovePreview.AffordablePathDistanceCm);
    }

    RefreshTacticalCombatHUD();
}

void UTacticalMovementControllerComponent::BuildClampedTacticalPath(const TArray<FVector> &SourcePathPoints, float MaxDistanceCm, TArray<FVector> &OutPathPoints, float &OutDistanceCm) const
{
    // Clamp by exact traveled distance so the affordable preview stops at the true movement budget breakpoint.
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

void UTacticalMovementControllerComponent::UpdateTacticalMovePreviewFromHover()
{
    if (!IsTacticalModeActive())
    {
        return;
    }

    ACRPGProjectPlayerController *Controller = GetOwnerController();
    UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    if (!Controller || !TacticalTurnSubsystem || !TacticalTurnSubsystem->IsTurnModeActive() || !Controller->IsTurnModeMovementEnabled())
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

    // Hover refresh is throttled to avoid doing a nav query every frame when the cursor barely moved.
    if (PendingMovePreview.bHasPreview && FVector::Dist2D(PendingMovePreview.Destination, HoveredWorldLocation) <= Controller->GetTacticalHoverPreviewRefreshTolerance())
    {
        if (GetWorld() && GetWorld()->GetTimeSeconds() < NextTacticalHoverPreviewRefreshTime)
        {
            return;
        }
    }

    BuildTacticalMovePreview(HoveredWorldLocation);

    if (GetWorld())
    {
        NextTacticalHoverPreviewRefreshTime = GetWorld()->GetTimeSeconds() + Controller->GetTacticalHoverPreviewRefreshInterval();
    }
}

void UTacticalMovementControllerComponent::StartTacticalPathTraversal(const TArray<FVector> &PathPoints, float PendingDistanceConsumption)
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    ACharacter *ControlledCharacter = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
    ActiveTacticalPathPoints = PathPoints;
    bHasActiveTacticalPath = ActiveTacticalPathPoints.Num() > 0;
    CurrentPathIndex = ActiveTacticalPathPoints.Num() > 1 ? 1 : 0;
    PendingPathDistanceConsumption = FMath::Max(0.0f, PendingDistanceConsumption);
    ActiveTraversalTravelledDistanceCm = 0.0f;
    LastTraversalPawnLocation = Controller && Controller->GetPawn() ? Controller->GetPawn()->GetActorLocation() : FVector::ZeroVector;

    // Traversal switches the pawn into a deterministic locomotion mode driven by AddMovementInput each tick.
    DisableTraversalAvoidance(ControlledCharacter);
    DisableTraversalPawnCollision(ControlledCharacter);

    UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Path traversal started. Active points: %d, starting index: %d"), ActiveTacticalPathPoints.Num(), CurrentPathIndex);
}

void UTacticalMovementControllerComponent::UpdateTacticalPathTraversal(float DeltaTime)
{
    if (!bHasActiveTacticalPath || !IsTacticalModeActive())
    {
        return;
    }

    ACRPGProjectPlayerController *Controller = GetOwnerController();
    ACharacter *ControlledCharacter = Controller ? Cast<ACharacter>(Controller->GetPawn()) : nullptr;
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

    const FVector IndexedTargetPoint = ActiveTacticalPathPoints[CurrentPathIndex];
    FVector ToIndexedTarget = IndexedTargetPoint - PawnLocation;
    ToIndexedTarget.Z = 0.0f;
    const bool bIsFinalPathPoint = CurrentPathIndex == ActiveTacticalPathPoints.Num() - 1;
    const float CurrentAcceptanceRadius = bIsFinalPathPoint
                                              ? FMath::Min(Controller->GetTacticalAcceptanceRadius(), Controller->GetTacticalFinalAcceptanceRadius())
                                              : Controller->GetTacticalAcceptanceRadius();

    if (ToIndexedTarget.SizeSquared() <= FMath::Square(CurrentAcceptanceRadius))
    {
        UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Reached path node %d at %s"), CurrentPathIndex, *IndexedTargetPoint.ToString());
        ++CurrentPathIndex;

        if (!ActiveTacticalPathPoints.IsValidIndex(CurrentPathIndex))
        {
            UE_LOG(LogCRPGProject, Verbose, TEXT("[TacticalMove] Tactical traversal completed"));
            HandleTacticalPathTraversalCompleted();
            ClearTacticalPathTraversal(true);
        }

        return;
    }

    FVector LookAheadTarget = Controller->GetTraversalLookAheadTarget(PawnLocation, CurrentPathIndex);
    FVector ToLookAheadTarget = LookAheadTarget - PawnLocation;
    ToLookAheadTarget.Z = 0.0f;

    if (ToLookAheadTarget.IsNearlyZero())
    {
        ToLookAheadTarget = ToIndexedTarget;
    }

    // Manual locomotion keeps the possessed unit under character movement instead of AI path following.
    const FVector MoveDirection = ToLookAheadTarget.GetSafeNormal();
    RotatePawnTowardDirection(ControlledCharacter, MoveDirection, DeltaTime);
    ControlledCharacter->AddMovementInput(MoveDirection, 1.0f);
}

void UTacticalMovementControllerComponent::RotatePawnTowardDirection(APawn *ControlledPawn, const FVector &MoveDirection, float DeltaTime) const
{
    if (!OwningController || !ControlledPawn || MoveDirection.IsNearlyZero())
    {
        return;
    }

    const FRotator DesiredRotation = MoveDirection.Rotation();
    const FRotator NewRotation = FMath::RInterpTo(ControlledPawn->GetActorRotation(), DesiredRotation, DeltaTime, OwningController->GetTacticalPathRotationInterpSpeed());
    ControlledPawn->SetActorRotation(NewRotation);
}

void UTacticalMovementControllerComponent::HandleTacticalPathTraversalCompleted()
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    float ConsumedDistanceCm = PendingPathDistanceConsumption;
    if (!Controller || ConsumedDistanceCm <= Controller->GetTacticalMinimumCommittedMoveDistance())
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

                    if (RemainingMovementRangeCm - PendingPathDistanceConsumption <= Controller->GetTacticalMinimumCommittedMoveDistance() + KINDA_SMALL_NUMBER)
                    {
                        ConsumedDistanceCm = RemainingMovementRangeCm;
                    }

                    // Movement budget is consumed only after traversal finishes or is explicitly interrupted and committed.
                    TacticalUnitComponent->ConsumeMovement(ConsumedDistanceCm);

                    if (UGameInstance *GameInstance = Controller->GetGameInstance())
                    {
                        if (UEventBusSubsystem *EventBusSubsystem = GameInstance->GetSubsystem<UEventBusSubsystem>())
                        {
                            EventBusSubsystem->PublishEvent(
                                TacticalMovementEvents::TacticalUnitMovementConsumed,
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

void UTacticalMovementControllerComponent::RefreshTacticalCombatHUD() const
{
    if (const ACRPGProjectPlayerController *Controller = GetOwnerController())
    {
        Controller->RefreshTacticalCombatHUD();
    }
}

void UTacticalMovementControllerComponent::StopTacticalPrototypeMovement() const
{
    ACRPGProjectPlayerController *Controller = GetOwnerController();
    if (!Controller)
    {
        return;
    }

    if (ACharacter *ControlledCharacter = Cast<ACharacter>(Controller->GetPawn()))
    {
        ControlledCharacter->StopJumping();
        ControlledCharacter->GetCharacterMovement()->StopMovementImmediately();
    }

    Controller->StopMovement();
}
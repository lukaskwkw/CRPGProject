#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Tactical/Movement/TacticalPathTypes.h"
#include "TacticalMovementControllerComponent.generated.h"

class ACRPGProjectPlayerController;
struct FHitResult;
class APawn;
class ACharacter;
class ACRPGBaseCharacter;
class UTacticalPathPreviewComponent;
class UTacticalTurnSubsystem;

UCLASS(ClassGroup = (Tactical), meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UTacticalMovementControllerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTacticalMovementControllerComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

    void HandleTacticalLeftClick();
    void CommitPendingTacticalMove();
    void ClearPendingTacticalMovePreview();
    void StopActiveTacticalMove(bool bConsumeTravelledDistance);
    // Clears movement state, restores any temporarily disabled collision/avoidance and optionally stops the pawn.
    void ClearTacticalPathTraversal(bool bStopPawnMovement);

    bool HasActiveTacticalPath() const;
    const TArray<FVector> &GetActiveTacticalPathPoints() const;
    float GetAvailableMovementBudgetForPlanning() const;
    const FTacticalMovePreviewData &GetPendingMovePreview() const;

private:
    // Private helpers split into four concerns: controller access, preview building, traversal, and cleanup/restoration.
    ACRPGProjectPlayerController *GetOwnerController() const;
    UTacticalTurnSubsystem *GetTacticalTurnSubsystem() const;
    UTacticalPathPreviewComponent *GetPathPreviewComponent() const;
    void DisableTraversalAvoidance(ACharacter *ControlledCharacter);
    void RestoreTraversalAvoidance();
    void DisableTraversalPawnCollision(ACharacter *ControlledCharacter);
    void RestoreTraversalPawnCollision();
    bool IsTacticalModeActive() const;
    bool TryGetTacticalCursorWorldLocation(FVector &OutWorldLocation) const;
    bool IsClickNearPendingDestination(const FVector &ClickedLocation) const;
    bool IsClickOnHoveredInteractable(const FHitResult &HitResult) const;
    float GetReservedMovementDistanceCm() const;
    float CalculatePathDistance(const TArray<FVector> &PathPoints) const;
    void BuildTacticalMovePreview(const FVector &Destination);
    // Reuses the regular movement-preview pipeline, but feeds it a controller-computed destination that would
    // move the active unit into melee or ranged attack range of the hovered enemy.
    bool TryBuildCombatTargetingPreview();
    // Reuses the regular preview pipeline to stop at the hovered interactable's configured use distance.
    bool TryBuildInteractableHoverPreview();
    void BuildClampedTacticalPath(const TArray<FVector> &SourcePathPoints, float MaxDistanceCm, TArray<FVector> &OutPathPoints, float &OutDistanceCm) const;
    void UpdateTacticalMovePreviewFromHover();
    void StartTacticalPathTraversal(const TArray<FVector> &PathPoints, float PendingDistanceConsumption = 0.0f);
    void UpdateTacticalPathTraversal(float DeltaTime);
    void RotatePawnTowardDirection(APawn *ControlledPawn, const FVector &MoveDirection, float DeltaTime) const;
    void HandleTacticalPathTraversalCompleted();
    void RefreshTacticalCombatHUD() const;
    void StopTacticalPrototypeMovement() const;

private:
    UPROPERTY(Transient)
    TObjectPtr<ACRPGProjectPlayerController> OwningController;

    UPROPERTY(Transient)
    bool bHasActiveTacticalPath = false;

    UPROPERTY(Transient)
    TArray<FVector> ActiveTacticalPathPoints;

    UPROPERTY(Transient)
    int32 CurrentPathIndex = INDEX_NONE;

    UPROPERTY(Transient)
    float PendingPathDistanceConsumption = 0.0f;

    UPROPERTY(Transient)
    float ActiveTraversalTravelledDistanceCm = 0.0f;

    UPROPERTY(Transient)
    float NextTacticalHoverPreviewRefreshTime = 0.0f;

    UPROPERTY(Transient)
    FVector LastTraversalPawnLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    TObjectPtr<ACharacter> TraversalAvoidanceCharacter = nullptr;

    UPROPERTY(Transient)
    bool bTraversalAvoidanceWasEnabled = false;

    UPROPERTY(Transient)
    TObjectPtr<ACharacter> TraversalPawnCollisionCharacter = nullptr;

    UPROPERTY(Transient)
    TEnumAsByte<ECollisionResponse> TraversalPawnCollisionResponse = ECR_Block;

    UPROPERTY(Transient)
    FTacticalMovePreviewData PendingMovePreview;
};
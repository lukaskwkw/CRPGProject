// Copyright Epic Games, Inc. All Rights Reserved.

#include "CRPGProjectPlayerController.h"

#include "Combat/Subsystems/CombatResolverSubsystem.h"
#include "Engine/EngineTypes.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Components/TacticalMovementControllerComponent.h"
#include "Tactical/Components/TacticalUnitComponent.h"
#include "Tactical/Subsystems/TacticalTurnSubsystem.h"

void ACRPGProjectPlayerController::EnterCombatTargetingMode(ECombatActionType ActionType)
{
    UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    ACRPGBaseCharacter *SelectedUnit = Cast<ACRPGBaseCharacter>(GetPawn());
    ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
    UTacticalUnitComponent *SelectedUnitComponent = SelectedUnit ? SelectedUnit->GetTacticalUnitComponent() : nullptr;
    const bool bIsTurnModeActive = TacticalTurnSubsystem && TacticalTurnSubsystem->IsTurnModeActive();

    if (!SelectedUnit || !SelectedUnitComponent || !SelectedUnitComponent->IsAlive() || (bIsTurnModeActive && SelectedUnit != ActiveUnit))
    {
        ClearCombatTargetingMode();
        return;
    }

    if (PendingCombatAction == ActionType && CurrentTargetingMode == ECombatTargetingMode::EnemyUnit)
    {
        ClearCombatTargetingMode();
        RefreshTacticalCombatHUD();
        return;
    }

    // Preserve the user's original movement-enabled state only on the first entry into targeting.
    // Switching from melee to ranged should not overwrite this snapshot with "false".
    const bool bWasAlreadyTargeting = CurrentTargetingMode == ECombatTargetingMode::EnemyUnit;
    if (!bWasAlreadyTargeting && bIsTurnModeActive)
    {
        bMovementEnabledBeforeCombatTargeting = IsTurnModeMovementEnabled();
    }

    PendingCombatAction = ActionType;
    CurrentTargetingMode = ActionType == ECombatActionType::None ? ECombatTargetingMode::None : ECombatTargetingMode::EnemyUnit;
    SetHoveredCombatTarget(nullptr);
    if (bIsTurnModeActive)
    {
        SetTurnModeMovementEnabled(false);
    }
    ClearPendingTacticalMovePreviewRequest();
}

void ACRPGProjectPlayerController::ClearCombatTargetingMode()
{
    PendingCombatAction = ECombatActionType::None;
    CurrentTargetingMode = ECombatTargetingMode::None;
    SetHoveredCombatTarget(nullptr);

    if (UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem(); TacticalTurnSubsystem && TacticalTurnSubsystem->IsTurnModeActive())
    {
        SetTurnModeMovementEnabled(bMovementEnabledBeforeCombatTargeting);
    }
}

bool ACRPGProjectPlayerController::IsCombatTargetingPreviewActive() const
{
    const UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    return TacticalTurnSubsystem && TacticalTurnSubsystem->IsTurnModeActive() &&
           CurrentTargetingMode == ECombatTargetingMode::EnemyUnit &&
           (PendingCombatAction == ECombatActionType::MeleeAttack || PendingCombatAction == ECombatActionType::RangedAttack);
}

bool ACRPGProjectPlayerController::TryGetCombatTargetingPreviewDestination(FVector &OutDestination) const
{
    if (!IsCombatTargetingPreviewActive())
    {
        return false;
    }

    const ACRPGBaseCharacter *SelectedUnit = Cast<ACRPGBaseCharacter>(GetPawn());
    const UTacticalUnitComponent *Attacker = SelectedUnit ? SelectedUnit->GetTacticalUnitComponent() : nullptr;
    UTacticalUnitComponent *TargetUnit = ResolveUnitUnderCursor();
    if (!IsValidCombatTarget(Attacker, TargetUnit))
    {
        return false;
    }

    if (IsTargetInCombatRange(Attacker, TargetUnit, PendingCombatAction))
    {
        return false;
    }

    const FVector AttackerLocation = Attacker->GetOccupiedLocation();
    const FVector TargetLocation = TargetUnit->GetOccupiedLocation();
    FVector ApproachDirection = AttackerLocation - TargetLocation;
    ApproachDirection.Z = 0.0f;

    if (!ApproachDirection.Normalize())
    {
        return false;
    }

    // The destination is not the enemy location itself. It is the closest straight-line point that would place
    // the attacker safely inside the relevant legal range once occupied radii, nav projection, and traversal acceptance
    // are taken into account.
    const float AttackRangeCm = PendingCombatAction == ECombatActionType::MeleeAttack ? Attacker->GetMeleeRangeCm() : Attacker->GetRangedRangeCm();
    const float DesiredCenterDistanceCm = FMath::Max(
        0.0f,
        AttackRangeCm + Attacker->GetOccupancyRadiusCm() + TargetUnit->GetOccupancyRadiusCm() - CombatApproachBufferCm);
    OutDestination = TargetLocation + (ApproachDirection * DesiredCenterDistanceCm);
    return true;
}

UTacticalUnitComponent *ACRPGProjectPlayerController::ResolveUnitUnderCursor() const
{
    // Combat targeting should prefer an actual pawn under the cursor over the floor hit used by movement preview.
    // Without this ordering, hovering an enemy often resolves to the ground beneath them and targeting appears inert.
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    FHitResult PawnHitResult;
    if (GetHitResultUnderCursorForObjects(ObjectTypes, true, PawnHitResult) && PawnHitResult.bBlockingHit)
    {
        AActor *PawnHitActor = PawnHitResult.GetActor();
        if (!PawnHitActor && PawnHitResult.Component.IsValid())
        {
            PawnHitActor = PawnHitResult.Component->GetOwner();
        }

        if (ACRPGBaseCharacter *PawnHitCharacter = Cast<ACRPGBaseCharacter>(PawnHitActor))
        {
            return PawnHitCharacter->GetTacticalUnitComponent();
        }
    }

    FHitResult HitResult;
    if (!GetHitResultUnderCursorByChannel(UEngineTypes::ConvertToTraceType(ECC_Visibility), true, HitResult) || !HitResult.bBlockingHit)
    {
        return nullptr;
    }

    AActor *HitActor = HitResult.GetActor();
    if (!HitActor && HitResult.Component.IsValid())
    {
        HitActor = HitResult.Component->GetOwner();
    }

    ACRPGBaseCharacter *HitCharacter = Cast<ACRPGBaseCharacter>(HitActor);
    return HitCharacter ? HitCharacter->GetTacticalUnitComponent() : nullptr;
}

void ACRPGProjectPlayerController::SetHoveredCombatTarget(UTacticalUnitComponent *NewTargetUnit)
{
    const ACRPGBaseCharacter *SelectedUnit = Cast<ACRPGBaseCharacter>(GetPawn());
    const UTacticalUnitComponent *Attacker = SelectedUnit ? SelectedUnit->GetTacticalUnitComponent() : nullptr;
    if (CurrentTargetingMode != ECombatTargetingMode::EnemyUnit || !IsValidCombatTarget(Attacker, NewTargetUnit))
    {
        NewTargetUnit = nullptr;
    }

    UTacticalUnitComponent *PreviousTargetUnit = HoveredTargetUnit.Get();
    if (PreviousTargetUnit == NewTargetUnit)
    {
        return;
    }

    SetCombatTargetHighlight(PreviousTargetUnit, false);
    HoveredTargetUnit = NewTargetUnit;
    SetCombatTargetHighlight(NewTargetUnit, true);
    RefreshTacticalCombatHUD();
}

void ACRPGProjectPlayerController::SetCombatTargetHighlight(UTacticalUnitComponent *TargetUnit, bool bEnableHighlight) const
{
    if (ACRPGBaseCharacter *TargetCharacter = TargetUnit ? Cast<ACRPGBaseCharacter>(TargetUnit->GetOwner()) : nullptr)
    {
        TargetCharacter->SetCombatTargetHighlightEnabled(bEnableHighlight);
    }
}

bool ACRPGProjectPlayerController::IsValidCombatTarget(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender) const
{
    return Attacker && Defender && Attacker != Defender && Attacker->IsAlive() && Defender->IsAlive() && Attacker->IsEnemyTo(Defender);
}

float ACRPGProjectPlayerController::GetCombatGapToTargetCm(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender) const
{
    if (!IsValidCombatTarget(Attacker, Defender))
    {
        return TNumericLimits<float>::Max();
    }

    const float CenterDistanceCm = FVector::Dist2D(Attacker->GetOccupiedLocation(), Defender->GetOccupiedLocation());
    return FMath::Max(0.0f, CenterDistanceCm - Attacker->GetOccupancyRadiusCm() - Defender->GetOccupancyRadiusCm());
}

bool ACRPGProjectPlayerController::IsTargetInCombatRange(const UTacticalUnitComponent *Attacker, const UTacticalUnitComponent *Defender, ECombatActionType ActionType) const
{
    if (!IsValidCombatTarget(Attacker, Defender))
    {
        return false;
    }

    const float RequiredRangeCm = ActionType == ECombatActionType::MeleeAttack ? Attacker->GetMeleeRangeCm() : Attacker->GetRangedRangeCm();
    return GetCombatGapToTargetCm(Attacker, Defender) <= RequiredRangeCm + CombatRangeToleranceCm;
}

bool ACRPGProjectPlayerController::TryExecutePendingCombatAction(UTacticalUnitComponent *TargetUnit)
{
    UTacticalTurnSubsystem *TacticalTurnSubsystem = GetTacticalTurnSubsystem();
    ACRPGBaseCharacter *SelectedUnit = Cast<ACRPGBaseCharacter>(GetPawn());
    ACRPGBaseCharacter *ActiveUnit = TacticalTurnSubsystem ? TacticalTurnSubsystem->GetActiveUnit() : nullptr;
    UTacticalUnitComponent *Attacker = SelectedUnit ? SelectedUnit->GetTacticalUnitComponent() : nullptr;
    const bool bIsTurnModeActive = TacticalTurnSubsystem && TacticalTurnSubsystem->IsTurnModeActive();

    if (!Attacker || !SelectedUnit || (bIsTurnModeActive && SelectedUnit != ActiveUnit))
    {
        return false;
    }

    if ((bIsTurnModeActive && !Attacker->HasAnyActionPoints()) || !IsValidCombatTarget(Attacker, TargetUnit))
    {
        return false;
    }

    if (!IsTargetInCombatRange(Attacker, TargetUnit, PendingCombatAction))
    {
        if (bIsTurnModeActive && TacticalMovementControllerComponent)
        {
            const FTacticalMovePreviewData &PendingMovePreview = GetPendingMovePreview();
            if (PendingMovePreview.bHasPreview && PendingMovePreview.AffordablePathPoints.Num() >= 2 && PendingMovePreview.AffordablePathDistanceCm > GetTacticalMinimumCommittedMoveDistance())
            {
                // Melee outside range uses move-then-attack. Ranged outside range only moves into range and stops.
                if (PendingCombatAction == ECombatActionType::MeleeAttack)
                {
                    DeferredCombatActionAfterTraversal = PendingCombatAction;
                    DeferredCombatTargetAfterTraversal = TargetUnit;
                }
                else
                {
                    DeferredCombatActionAfterTraversal = ECombatActionType::None;
                    DeferredCombatTargetAfterTraversal.Reset();
                }

                TacticalMovementControllerComponent->CommitPendingTacticalMove();
                ClearCombatTargetingMode();
            }
        }

        return false;
    }

    if (SelectedUnit && TargetUnit)
    {
        FVector LookDirection = TargetUnit->GetOccupiedLocation() - SelectedUnit->GetActorLocation();
        LookDirection.Z = 0.0f;
        if (!LookDirection.IsNearlyZero())
        {
            SelectedUnit->SetActorRotation(LookDirection.Rotation());
        }
    }

    UCombatResolverSubsystem *CombatResolverSubsystem = GetCombatResolverSubsystem();
    if (!CombatResolverSubsystem)
    {
        return false;
    }

    switch (PendingCombatAction)
    {
    case ECombatActionType::MeleeAttack:
        CombatResolverSubsystem->ResolveMeleeAttack(Attacker, TargetUnit);
        break;
    case ECombatActionType::RangedAttack:
        CombatResolverSubsystem->ResolveRangedAttack(Attacker, TargetUnit);
        break;
    default:
        return false;
    }

    RefreshTacticalCombatHUD();
    return true;
}

void ACRPGProjectPlayerController::HandleTacticalTraversalCompleted()
{
    // This callback is the seam between deterministic locomotion and combat resolution.
    // If melee used move-then-attack, we restore the pending action just long enough to re-run the normal validation path.
    UTacticalUnitComponent *DeferredTargetUnit = DeferredCombatTargetAfterTraversal.Get();
    const ECombatActionType DeferredAction = DeferredCombatActionAfterTraversal;
    DeferredCombatTargetAfterTraversal.Reset();
    DeferredCombatActionAfterTraversal = ECombatActionType::None;

    if (DeferredAction == ECombatActionType::None || !DeferredTargetUnit)
    {
        return;
    }

    const ECombatActionType PreviousPendingAction = PendingCombatAction;
    const ECombatTargetingMode PreviousTargetingMode = CurrentTargetingMode;
    PendingCombatAction = DeferredAction;
    CurrentTargetingMode = ECombatTargetingMode::EnemyUnit;
    TryExecutePendingCombatAction(DeferredTargetUnit);
    PendingCombatAction = PreviousPendingAction;
    CurrentTargetingMode = PreviousTargetingMode;
    RefreshTacticalCombatHUD();
}

UCombatResolverSubsystem *ACRPGProjectPlayerController::GetCombatResolverSubsystem() const
{
    if (UGameInstance *GameInstance = GetGameInstance())
    {
        return GameInstance->GetSubsystem<UCombatResolverSubsystem>();
    }

    return nullptr;
}
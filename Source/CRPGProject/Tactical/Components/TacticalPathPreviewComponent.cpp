#include "Tactical/Components/TacticalPathPreviewComponent.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

namespace
{
    constexpr float TacticalPathDebugSphereRadius = 4.0f;
    constexpr float TacticalPathDebugLineThickness = 1.0f;
}

UTacticalPathPreviewComponent::UTacticalPathPreviewComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTacticalPathPreviewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bHasPathToRender || CachedPathPoints.Num() == 0 || !GetWorld())
    {
        return;
    }

    float TraversedDistanceCm = 0.0f;

    const FVector PathOffset(0.0f, 0.0f, PathDebugVerticalOffset);

    DrawDebugSphere(GetWorld(), CachedPathPoints[0] + PathOffset, TacticalPathDebugSphereRadius, 8, CachedAffordableDistanceCm > 0.0f ? FColor::Green : FColor::Red, false, 0.0f);

    for (int32 PointIndex = 1; PointIndex < CachedPathPoints.Num(); ++PointIndex)
    {
        const FVector SegmentStart = CachedPathPoints[PointIndex - 1];
        const FVector SegmentEnd = CachedPathPoints[PointIndex];
        const float SegmentDistanceCm = FVector::Distance(SegmentStart, SegmentEnd);
        const float SegmentEndDistanceCm = TraversedDistanceCm + SegmentDistanceCm;

        if (SegmentEndDistanceCm <= CachedAffordableDistanceCm + KINDA_SMALL_NUMBER)
        {
            DrawPathSegment(SegmentStart, SegmentEnd, FColor::Green);
            DrawDebugSphere(GetWorld(), SegmentEnd + PathOffset, TacticalPathDebugSphereRadius, 8, FColor::Green, false, 0.0f);
        }
        else if (TraversedDistanceCm >= CachedAffordableDistanceCm - KINDA_SMALL_NUMBER)
        {
            DrawPathSegment(SegmentStart, SegmentEnd, FColor::Red);
            DrawDebugSphere(GetWorld(), SegmentEnd + PathOffset, TacticalPathDebugSphereRadius, 8, FColor::Red, false, 0.0f);
        }
        else
        {
            const float RemainingAffordableDistanceCm = CachedAffordableDistanceCm - TraversedDistanceCm;
            const float Alpha = SegmentDistanceCm > KINDA_SMALL_NUMBER ? RemainingAffordableDistanceCm / SegmentDistanceCm : 0.0f;
            const FVector BreakPoint = FMath::Lerp(SegmentStart, SegmentEnd, FMath::Clamp(Alpha, 0.0f, 1.0f));

            DrawPathSegment(SegmentStart, BreakPoint, FColor::Green);
            DrawDebugSphere(GetWorld(), BreakPoint + PathOffset, TacticalPathDebugSphereRadius, 8, FColor::Red, false, 0.0f);
            DrawPathSegment(BreakPoint, SegmentEnd, FColor::Red);
            DrawDebugSphere(GetWorld(), SegmentEnd + PathOffset, TacticalPathDebugSphereRadius, 8, FColor::Red, false, 0.0f);
        }

        TraversedDistanceCm = SegmentEndDistanceCm;
    }
}

void UTacticalPathPreviewComponent::RenderPath(const TArray<FVector> &PathPoints, float AffordableDistanceCm)
{
    CachedPathPoints = PathPoints;
    bHasPathToRender = CachedPathPoints.Num() > 0;
    CachedAffordableDistanceCm = FMath::Max(0.0f, AffordableDistanceCm);
}

void UTacticalPathPreviewComponent::ClearPath()
{
    bHasPathToRender = false;
    CachedPathPoints.Reset();
    CachedAffordableDistanceCm = 0.0f;
}

void UTacticalPathPreviewComponent::DrawPathSegment(const FVector &SegmentStart, const FVector &SegmentEnd, const FColor &SegmentColor) const
{
    if (!GetWorld())
    {
        return;
    }

    const FVector PathOffset(0.0f, 0.0f, PathDebugVerticalOffset);
    DrawDebugLine(GetWorld(), SegmentStart + PathOffset, SegmentEnd + PathOffset, SegmentColor, false, 0.0f, 0, TacticalPathDebugLineThickness);
}

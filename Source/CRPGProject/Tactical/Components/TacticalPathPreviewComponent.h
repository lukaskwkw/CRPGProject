#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TacticalPathPreviewComponent.generated.h"

UCLASS(ClassGroup = (Tactical), meta = (BlueprintSpawnableComponent))
class CRPGPROJECT_API UTacticalPathPreviewComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTacticalPathPreviewComponent();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
    void RenderPath(const TArray<FVector>& PathPoints, float AffordableDistanceCm);

    UFUNCTION(BlueprintCallable, Category = "Tactical|Movement")
    void ClearPath();

private:
    void DrawPathSegment(const FVector& SegmentStart, const FVector& SegmentEnd, const FColor& SegmentColor) const;

  UPROPERTY(EditAnywhere, Category = "Tactical|Movement|Debug")
    FLinearColor ValidPathColor = FLinearColor(0.1f, 1.0f, 0.1f, 0.45f);

    UPROPERTY(EditAnywhere, Category = "Tactical|Movement|Debug")
    FLinearColor InvalidPathColor = FLinearColor(1.0f, 0.15f, 0.15f, 0.35f);

    UPROPERTY(EditAnywhere, Category = "Tactical|Movement|Debug", meta = (ClampMin = "0.0", Units = "cm"))
    float PathDebugVerticalOffset = 5.0f;

    UPROPERTY(EditAnywhere, Category = "Tactical|Movement|Debug", meta = (ClampMin = "0.0", Units = "cm"))
    float PathDebugSphereRadius = 10.0f;

    UPROPERTY(EditAnywhere, Category = "Tactical|Movement|Debug", meta = (ClampMin = "4", ClampMax = "32"))
    int32 PathDebugSphereSegments = 8;

    UPROPERTY(EditAnywhere, Category = "Tactical|Movement|Debug", meta = (ClampMin = "0.0"))
    float PathDebugLineThickness = 1.0f;

    UPROPERTY()
    bool bHasPathToRender = false;

    UPROPERTY()
    float CachedAffordableDistanceCm = 0.0f;

    UPROPERTY()
    TArray<FVector> CachedPathPoints;
};

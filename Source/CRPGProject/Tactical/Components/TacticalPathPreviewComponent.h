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

    UPROPERTY(EditAnywhere, Category = "Tactical|Movement|Debug", meta = (ClampMin = "0.0", Units = "cm"))
    float PathDebugVerticalOffset = 5.0f;

    UPROPERTY()
    bool bHasPathToRender = false;

    UPROPERTY()
    float CachedAffordableDistanceCm = 0.0f;

    UPROPERTY()
    TArray<FVector> CachedPathPoints;
};

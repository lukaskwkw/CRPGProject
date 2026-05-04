#pragma once

#include "CoreMinimal.h"
#include "TacticalPathTypes.generated.h"

USTRUCT(BlueprintType)
struct CRPGPROJECT_API FTacticalMovePreviewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement")
    bool bHasPreview = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement")
    bool bIsAffordable = false;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement")
    FVector Destination = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement")
    TArray<FVector> PathPoints;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement")
    TArray<FVector> AffordablePathPoints;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement", meta = (Units = "cm"))
    float PathDistanceCm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement", meta = (Units = "cm"))
    float AffordablePathDistanceCm = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|Movement")
    bool bHasOverBudgetSegment = false;
};

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TacticalOutlineInteractable.generated.h"

UINTERFACE(BlueprintType)
class CRPGPROJECT_API UTacticalOutlineInteractable : public UInterface
{
    GENERATED_BODY()
};

class CRPGPROJECT_API ITacticalOutlineInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tactical|Outline")
    void SetInteractableOutlineEnabled(bool bEnabled);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tactical|Outline")
    bool IsInteractableOutlineExcluded() const;
};
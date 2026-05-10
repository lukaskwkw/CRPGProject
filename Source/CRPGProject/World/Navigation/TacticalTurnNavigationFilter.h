#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "TacticalTurnNavigationFilter.generated.h"

UCLASS()
class CRPGPROJECT_API UTacticalTurnNavigationFilter : public UNavigationQueryFilter
{
    GENERATED_BODY()

public:
    UTacticalTurnNavigationFilter(const FObjectInitializer &ObjectInitializer);
};

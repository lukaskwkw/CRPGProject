#pragma once

#include "CoreMinimal.h"
#include "NavAreas/NavArea_Null.h"
#include "TacticalTurnNavAreas.generated.h"

UCLASS()
class CRPGPROJECT_API UTacticalTurnNavArea_AllyOccupied : public UNavArea_Null
{
    GENERATED_BODY()

public:
    UTacticalTurnNavArea_AllyOccupied(const FObjectInitializer &ObjectInitializer);
};

UCLASS()
class CRPGPROJECT_API UTacticalTurnNavArea_EnemyOccupied : public UNavArea_Null
{
    GENERATED_BODY()

public:
    UTacticalTurnNavArea_EnemyOccupied(const FObjectInitializer &ObjectInitializer);
};

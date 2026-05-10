#include "World/Navigation/TacticalTurnNavigationFilter.h"

#include "World/Navigation/TacticalTurnNavAreas.h"

UTacticalTurnNavigationFilter::UTacticalTurnNavigationFilter(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    FNavigationFilterArea &AllyArea = Areas.AddDefaulted_GetRef();
    AllyArea.AreaClass = UTacticalTurnNavArea_AllyOccupied::StaticClass();
    AllyArea.bIsExcluded = true;

    FNavigationFilterArea &EnemyArea = Areas.AddDefaulted_GetRef();
    EnemyArea.AreaClass = UTacticalTurnNavArea_EnemyOccupied::StaticClass();
    EnemyArea.bIsExcluded = true;
}

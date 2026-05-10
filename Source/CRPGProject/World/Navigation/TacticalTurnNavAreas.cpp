#include "World/Navigation/TacticalTurnNavAreas.h"

UTacticalTurnNavArea_AllyOccupied::UTacticalTurnNavArea_AllyOccupied(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    DrawColor = FColor(64, 160, 255);
}

UTacticalTurnNavArea_EnemyOccupied::UTacticalTurnNavArea_EnemyOccupied(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer)
{
    DrawColor = FColor(255, 80, 80);
}

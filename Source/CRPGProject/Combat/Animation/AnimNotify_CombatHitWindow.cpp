#include "Combat/Animation/AnimNotify_CombatHitWindow.h"

#include "Combat/Subsystems/CombatExecutionSubsystem.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Components/TacticalUnitComponent.h"

void UAnimNotify_CombatHitWindow::Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation, const FAnimNotifyEventReference &EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    ACRPGBaseCharacter *OwnerCharacter = MeshComp ? Cast<ACRPGBaseCharacter>(MeshComp->GetOwner()) : nullptr;
    UTacticalUnitComponent *TacticalUnitComponent = OwnerCharacter ? OwnerCharacter->GetTacticalUnitComponent() : nullptr;
    UGameInstance *GameInstance = OwnerCharacter ? OwnerCharacter->GetGameInstance() : nullptr;
    UCombatExecutionSubsystem *CombatExecutionSubsystem = GameInstance ? GameInstance->GetSubsystem<UCombatExecutionSubsystem>() : nullptr;
    if (CombatExecutionSubsystem && TacticalUnitComponent)
    {
        CombatExecutionSubsystem->ExecutePendingAttackAtHitWindow(TacticalUnitComponent);
    }
}
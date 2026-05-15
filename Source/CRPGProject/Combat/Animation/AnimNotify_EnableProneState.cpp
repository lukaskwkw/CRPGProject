#include "Combat/Animation/AnimNotify_EnableProneState.h"

#include "Combat/Types/CombatTypes.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Components/TacticalUnitComponent.h"

void UAnimNotify_EnableProneState::Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation, const FAnimNotifyEventReference &EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    ACRPGBaseCharacter *OwnerCharacter = MeshComp ? Cast<ACRPGBaseCharacter>(MeshComp->GetOwner()) : nullptr;
    UTacticalUnitComponent *TacticalUnitComponent = OwnerCharacter ? OwnerCharacter->GetTacticalUnitComponent() : nullptr;
    if (!TacticalUnitComponent)
    {
        return;
    }

    if (TacticalUnitComponent->CanEnterProneState())
    {
        TacticalUnitComponent->SetCombatState(ECombatUnitState::Prone);
        return;
    }

    if (TacticalUnitComponent->IsProne())
    {
        OwnerCharacter->EnterTacticalProneState();
    }
}
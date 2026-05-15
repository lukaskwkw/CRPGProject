#include "Combat/Animation/AnimNotify_EnableRagdoll.h"

#include "Framework/Characters/CRPGBaseCharacter.h"

void UAnimNotify_EnableRagdoll::Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation, const FAnimNotifyEventReference &EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (ACRPGBaseCharacter *OwnerCharacter = MeshComp ? Cast<ACRPGBaseCharacter>(MeshComp->GetOwner()) : nullptr)
    {
        OwnerCharacter->EnableCombatRagdoll();
    }
}
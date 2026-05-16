#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CombatMissReactionWindow.generated.h"

UCLASS(meta = (DisplayName = "Combat Miss Reaction Window"))
class CRPGPROJECT_API UAnimNotify_CombatMissReactionWindow : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation, const FAnimNotifyEventReference &EventReference) override;
};
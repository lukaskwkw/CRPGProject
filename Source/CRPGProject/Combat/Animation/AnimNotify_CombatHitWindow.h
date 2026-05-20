#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_CombatHitWindow.generated.h"

UCLASS(meta = (DisplayName = "Combat Hit Window"))
class CRPGPROJECT_API UAnimNotify_CombatHitWindow : public UAnimNotify
{
    GENERATED_BODY()

public:
    virtual void Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation, const FAnimNotifyEventReference &EventReference) override;
};
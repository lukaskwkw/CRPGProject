#include "World/Actors/OutlineInteractableActor.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"

AOutlineInteractableActor::AOutlineInteractableActor()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
}

void AOutlineInteractableActor::OnConstruction(const FTransform &Transform)
{
    Super::OnConstruction(Transform);
    RefreshOutlinePresentation();
}

void AOutlineInteractableActor::SetInteractableOutlineCategory(ECRPGOutlineCategory NewCategory)
{
    if (InteractableOutlineCategory == NewCategory)
    {
        return;
    }

    InteractableOutlineCategory = NewCategory;
    RefreshOutlinePresentation();
}

void AOutlineInteractableActor::SetInteractableOutlineCategoryToInteractable()
{
    SetInteractableOutlineCategory(ECRPGOutlineCategory::Interactable);
}

void AOutlineInteractableActor::ClearInteractableOutlineCategory()
{
    SetInteractableOutlineCategory(ECRPGOutlineCategory::None);
}

void AOutlineInteractableActor::SetInteractableOutlineExcluded(bool bExcluded)
{
    if (bInteractableOutlineExcluded == bExcluded)
    {
        return;
    }

    bInteractableOutlineExcluded = bExcluded;
    RefreshOutlinePresentation();
}

void AOutlineInteractableActor::SetInteractableOutlineEnabled_Implementation(bool bEnabled)
{
    if (bInteractableOutlineEnabled == bEnabled)
    {
        return;
    }

    bInteractableOutlineEnabled = bEnabled;
    RefreshOutlinePresentation();
}

bool AOutlineInteractableActor::IsInteractableOutlineExcluded_Implementation() const
{
    return bInteractableOutlineExcluded;
}

void AOutlineInteractableActor::RefreshOutlinePresentation()
{
    const ECRPGOutlineCategory IdleCategory = bShowOutlineWhenIdle ? InteractableOutlineCategory : ECRPGOutlineCategory::None;
    const ECRPGOutlineCategory EffectiveCategory = bInteractableOutlineExcluded
                                                       ? ECRPGOutlineCategory::None
                                                       : (bInteractableOutlineEnabled ? InteractableOutlineCategory : IdleCategory);
    const bool bEnableOutline = EffectiveCategory != ECRPGOutlineCategory::None;

    TInlineComponentArray<UPrimitiveComponent *> PrimitiveComponents;
    GetComponents<UPrimitiveComponent>(PrimitiveComponents);
    for (UPrimitiveComponent *PrimitiveComponent : PrimitiveComponents)
    {
        ApplyOutlineToPrimitiveComponent(PrimitiveComponent, bEnableOutline, EffectiveCategory);
    }
}

void AOutlineInteractableActor::ApplyOutlineToPrimitiveComponent(UPrimitiveComponent *PrimitiveComponent, bool bEnableOutline, ECRPGOutlineCategory EffectiveCategory) const
{
    if (!IsValid(PrimitiveComponent) || PrimitiveComponent->bHiddenInGame)
    {
        return;
    }

    PrimitiveComponent->SetRenderCustomDepth(bEnableOutline);
    PrimitiveComponent->SetCustomDepthStencilValue(static_cast<int32>(EffectiveCategory));
}
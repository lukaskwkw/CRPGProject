#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Framework/Characters/CRPGBaseCharacter.h"
#include "Tactical/Interfaces/TacticalOutlineInteractable.h"
#include "OutlineInteractableActor.generated.h"

class UPrimitiveComponent;
class USceneComponent;

UCLASS()
class CRPGPROJECT_API AOutlineInteractableActor : public AActor, public ITacticalOutlineInteractable
{
    GENERATED_BODY()

public:
    AOutlineInteractableActor();

    virtual void OnConstruction(const FTransform &Transform) override;

    UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
    void SetInteractableOutlineCategory(ECRPGOutlineCategory NewCategory);

    UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
    void SetInteractableOutlineCategoryToInteractable();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
    void ClearInteractableOutlineCategory();

    UFUNCTION(BlueprintCallable, Category = "Tactical|Outline")
    void SetInteractableOutlineExcluded(bool bExcluded);

    UFUNCTION(BlueprintPure, Category = "Tactical|Outline")
    ECRPGOutlineCategory GetInteractableOutlineCategory() const { return InteractableOutlineCategory; }

    UFUNCTION(BlueprintPure, Category = "Tactical|Outline")
    bool IsInteractableOutlineEnabled() const { return bInteractableOutlineEnabled; }

    UFUNCTION(BlueprintPure, Category = "Tactical|Outline")
    bool GetShowOutlineWhenIdle() const { return bShowOutlineWhenIdle; }

    virtual void SetInteractableOutlineEnabled_Implementation(bool bEnabled) override;
    virtual bool IsInteractableOutlineExcluded_Implementation() const override;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Outline", meta = (AllowPrivateAccess = "true"))
    ECRPGOutlineCategory InteractableOutlineCategory = ECRPGOutlineCategory::Interactable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Outline", meta = (AllowPrivateAccess = "true"))
    bool bShowOutlineWhenIdle = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tactical|Outline", meta = (AllowPrivateAccess = "true"))
    bool bInteractableOutlineExcluded = false;

    UPROPERTY(Transient)
    bool bInteractableOutlineEnabled = false;

private:
    void RefreshOutlinePresentation();
    void ApplyOutlineToPrimitiveComponent(UPrimitiveComponent *PrimitiveComponent, bool bEnableOutline, ECRPGOutlineCategory EffectiveCategory) const;
};
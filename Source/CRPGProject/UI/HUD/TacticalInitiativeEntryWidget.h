#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TacticalCombatHUDWidget.h"
#include "TacticalInitiativeEntryWidget.generated.h"

UCLASS(Abstract, Blueprintable)
class CRPGPROJECT_API UTacticalInitiativeEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Intercepts clicks so the initiative tracker can request selection or camera focus for that unit. */
    virtual FReply NativeOnMouseButtonDown(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    /** Mirrors hover state into the controller so stale world-space preview is cleared immediately. */
    virtual void NativeOnMouseEnter(const FGeometry &InGeometry, const FPointerEvent &InMouseEvent) override;
    /** Releases transient UI-hover state after leaving the entry. */
    virtual void NativeOnMouseLeave(const FPointerEvent &InMouseEvent) override;

    UFUNCTION(BlueprintCallable, Category = "Tactical|HUD")
    /** Updates the initiative-entry view model and notifies Blueprint to redraw. */
    void SetViewData(const FTacticalInitiativeEntryViewData &InViewData);

    UPROPERTY(BlueprintReadOnly, Category = "Tactical|HUD")
    FTacticalInitiativeEntryViewData ViewData;

    UFUNCTION(BlueprintImplementableEvent, Category = "Tactical|HUD")
    /** Called after SetViewData so Blueprint visuals can refresh from the new data. */
    void OnViewDataChanged();
};